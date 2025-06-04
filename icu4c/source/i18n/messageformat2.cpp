// © 2024 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#include "unicode/utypes.h"

#if !UCONFIG_NO_NORMALIZATION

#if !UCONFIG_NO_FORMATTING

#if !UCONFIG_NO_MF2

#include "unicode/messageformat2_arguments.h"
#include "unicode/messageformat2_data_model.h"
#include "unicode/messageformat2_formattable.h"
#include "unicode/messageformat2.h"
#include "unicode/normalizer2.h"
#include "unicode/unistr.h"
#include "messageformat2_allocation.h"
#include "messageformat2_checker.h"
#include "messageformat2_evaluation.h"
#include "messageformat2_function_registry_internal.h"
#include "messageformat2_macros.h"


U_NAMESPACE_BEGIN

namespace message2 {

using namespace data_model;

// ------------------------------------------------------
// Formatting

// The result of formatting a literal is just itself.
static Formattable evalLiteral(const Literal& lit) {
    return Formattable(lit.unquoted());
}

// Assumes that `var` is a message argument; returns the argument's value.
[[nodiscard]] FormattedPlaceholder MessageFormatter::evalArgument(const UnicodeString& fallback,
                                                                  const VariableName& var,
                                                                  MessageContext& context,
                                                                  UErrorCode& errorCode) const {
    if (U_SUCCESS(errorCode)) {
        const Formattable* val = context.getGlobal(var, errorCode);
        if (U_SUCCESS(errorCode)) {
            // Note: the fallback string has to be passed in because in a declaration like:
            // .local $foo = {$bar :number}
            // the fallback for $bar is "$foo".
            UnicodeString fallbackToUse = fallback;
            if (fallbackToUse.isEmpty()) {
                fallbackToUse += DOLLAR;
                fallbackToUse += var;
            }
            return (FormattedPlaceholder(*val, fallbackToUse));
        }
    }
    return {};
}

// Helper function to re-escape any escaped-char characters
static UnicodeString reserialize(const UnicodeString& s) {
    UnicodeString result(PIPE);
    for (int32_t i = 0; i < s.length(); i++) {
        switch(s[i]) {
        case BACKSLASH:
        case PIPE:
        case LEFT_CURLY_BRACE:
        case RIGHT_CURLY_BRACE: {
            result += BACKSLASH;
            break;
        }
        default:
            break;
        }
        result += s[i];
    }
    result += PIPE;
    return result;
}

// Returns the contents of the literal
[[nodiscard]] FormattedPlaceholder MessageFormatter::formatLiteral(const UnicodeString& fallback,
                                                                   const Literal& lit) const {
    // The fallback for a literal is itself, unless another fallback is passed in
    // (same reasoning as evalArgument())
    UnicodeString fallbackToUse = fallback.isEmpty() ? reserialize(lit.unquoted()) : fallback;
    return FormattedPlaceholder(evalLiteral(lit), fallbackToUse);
}

[[nodiscard]] InternalValue* MessageFormatter::formatOperand(const UnicodeString& fallback,
                                                             const Environment& env,
                                                             const Operand& rand,
                                                             MessageContext& context,
                                                             UErrorCode &status) const {
    if (U_FAILURE(status)) {
        return {};
    }

    if (rand.isNull()) {
        return create<InternalValue>(InternalValue(FormattedPlaceholder()), status);
    }
    if (rand.isVariable()) {
        // Check if it's local or global
        // Note: there is no name shadowing; this is enforced by the parser
        const VariableName& var = rand.asVariable();
        // TODO: Currently, this code implements lazy evaluation of locals.
        // That is, the environment binds names to a closure, not a resolved value.
        // Eager vs. lazy evaluation is an open issue:
        // see https://github.com/unicode-org/message-format-wg/issues/299

        // NFC-normalize the variable name. See
        // https://github.com/unicode-org/message-format-wg/blob/main/spec/syntax.md#names-and-identifiers
        const VariableName normalized = StandardFunctions::normalizeNFC(var);

        // Look up the variable in the environment
        if (env.has(normalized)) {
          // `var` is a local -- look it up
          const Closure& rhs = env.lookup(normalized);
          // Format the expression using the environment from the closure
          // The name of this local variable is the fallback for its RHS.
          UnicodeString newFallback(DOLLAR);
          newFallback += var;
          return formatExpression(newFallback, rhs.getEnv(), rhs.getExpr(), context, status);
        }
        // Variable wasn't found in locals -- check if it's global
        FormattedPlaceholder result = evalArgument(fallback, normalized, context, status);
        if (status == U_ILLEGAL_ARGUMENT_ERROR) {
            status = U_ZERO_ERROR;
            // Unbound variable -- set a resolution error
            context.getErrors().setUnresolvedVariable(var, status);
            // Use fallback per
            // https://github.com/unicode-org/message-format-wg/blob/main/spec/formatting.md#fallback-resolution
            UnicodeString str(DOLLAR);
            str += var;
            return create<InternalValue>(InternalValue(FormattedPlaceholder(str)), status);
        }
        return create<InternalValue>(InternalValue(std::move(result)), status);
    } else {
        U_ASSERT(rand.isLiteral());
        return create<InternalValue>(InternalValue(formatLiteral(fallback, rand.asLiteral())), status);
    }
}

// Resolves a function's options
FunctionOptions MessageFormatter::resolveOptions(const Environment& env, const OptionMap& options, MessageContext& context, UErrorCode& status) const {
    LocalPointer<UVector> optionsVector(createUVector(status));
    if (U_FAILURE(status)) {
        return {};
    }
    LocalPointer<ResolvedFunctionOption> resolvedOpt;
    for (int i = 0; i < options.size(); i++) {
        const Option& opt = options.getOption(i, status);
        if (U_FAILURE(status)) {
            return {};
        }
        const UnicodeString& k = opt.getName();
        const Operand& v = opt.getValue();

        // Options are fully evaluated before calling the function
        // Format the operand
        LocalPointer<InternalValue> rhsVal(formatOperand({}, env, v, context, status));
        if (U_FAILURE(status)) {
            return {};
        }
        // Note: this means option values are "eagerly" evaluated.
        // Currently, options don't have options. This will be addressed by the
        // full FormattedPlaceholder redesign.
        FormattedPlaceholder optValue = rhsVal->forceFormatting(context.getErrors(), status);
        resolvedOpt.adoptInstead(create<ResolvedFunctionOption>
                                 (ResolvedFunctionOption(k,
                                                         optValue.asFormattable()),
                                  status));
        if (U_FAILURE(status)) {
            return {};
        }
        optionsVector->adoptElement(resolvedOpt.orphan(), status);
    }
    return FunctionOptions(std::move(*optionsVector), status);
}

// Overload that dispatches on argument type. Syntax doesn't provide for options in this case.
[[nodiscard]] InternalValue* MessageFormatter::evalFunctionCall(FormattedPlaceholder&& argument,
                                                                MessageContext& context,
                                                                UErrorCode& status) const {
    if (U_FAILURE(status)) {
        return nullptr;
    }

    // These cases should have been checked for already
    U_ASSERT(!argument.isFallback() && !argument.isNullOperand());

    const Formattable& toFormat = argument.asFormattable();
    switch (toFormat.getType()) {
    case UFMT_OBJECT: {
        const FormattableObject* obj = toFormat.getObject(status);
        U_ASSERT(U_SUCCESS(status));
        U_ASSERT(obj != nullptr);
        const UnicodeString& type = obj->tag();
        FunctionName functionName;
        if (!getDefaultFormatterNameByType(type, functionName)) {
            // No formatter for this type -- follow default behavior
            break;
        }
        return evalFunctionCall(functionName,
                                create<InternalValue>(std::move(argument), status),
                                FunctionOptions(),
                                context,
                                status);
    }
    default: {
        // TODO: The array case isn't handled yet; not sure whether it's desirable
        // to have a default list formatter
        break;
    }
    }
    // No formatter for this type, or it's a primitive type (which will be formatted later)
    // -- just return the argument itself
    return create<InternalValue>(std::move(argument), status);
}

// Overload that dispatches on function name
// Adopts `arg`
[[nodiscard]] InternalValue* MessageFormatter::evalFunctionCall(const FunctionName& functionName,
                                                                InternalValue* arg_,
                                                                FunctionOptions&& options,
                                                                MessageContext& context,
                                                                UErrorCode& status) const {
    if (U_FAILURE(status)) {
        return {};
    }

    LocalPointer<InternalValue> arg(arg_);

    // Look up the formatter or selector
    LocalPointer<Formatter> formatterImpl(nullptr);
    LocalPointer<Selector> selectorImpl(nullptr);
    if (isFormatter(functionName)) {
        formatterImpl.adoptInstead(getFormatter(functionName, status));
        U_ASSERT(U_SUCCESS(status));
    }
    if (isSelector(functionName)) {
        selectorImpl.adoptInstead(getSelector(context, functionName, status));
        U_ASSERT(U_SUCCESS(status));
    }
    if (formatterImpl == nullptr && selectorImpl == nullptr) {
        // Unknown function error
        context.getErrors().setUnknownFunction(functionName, status);

        if (arg->hasNullOperand()) {
            // Non-selector used as selector; an error would have been recorded earlier
            UnicodeString fallback(COLON);
            fallback += functionName;
            return new InternalValue(FormattedPlaceholder(fallback));
        } else {
            return new InternalValue(FormattedPlaceholder(arg->getFallback()));
        }
    }
    return new InternalValue(arg.orphan(),
                             std::move(options),
                             functionName,
                             formatterImpl.isValid() ? formatterImpl.orphan() : nullptr,
                             selectorImpl.isValid() ? selectorImpl.orphan() : nullptr);
}

// Formats an expression using `globalEnv` for the values of variables
[[nodiscard]] InternalValue* MessageFormatter::formatExpression(const UnicodeString& fallback,
                                                                const Environment& globalEnv,
                                                                const Expression& expr,
                                                                MessageContext& context,
                                                                UErrorCode &status) const {
    if (U_FAILURE(status)) {
        return {};
    }

    const Operand& rand = expr.getOperand();
    // Format the operand (formatOperand handles the case of a null operand)
    LocalPointer<InternalValue> randVal(formatOperand(fallback, globalEnv, rand, context, status));

    FormattedPlaceholder maybeRand = randVal->takeArgument(status);

    if (!expr.isFunctionCall() && U_SUCCESS(status)) {
        // Dispatch based on type of `randVal`
         if (maybeRand.isFallback()) {
            return randVal.orphan();
        }
        return evalFunctionCall(std::move(maybeRand), context, status);
    } else if (expr.isFunctionCall()) {
        status = U_ZERO_ERROR;
        const Operator* rator = expr.getOperator(status);
        U_ASSERT(U_SUCCESS(status));
        const FunctionName& functionName = rator->getFunctionName();
        const OptionMap& options = rator->getOptionsInternal();
        // Resolve the options
        FunctionOptions resolvedOptions = resolveOptions(globalEnv, options, context, status);

        // Call the formatter function
        return evalFunctionCall(functionName,
                                randVal.orphan(),
                                std::move(resolvedOptions),
                                context,
                                status);
    } else {
        status = U_ZERO_ERROR;
        return randVal.orphan();
    }
}

// Formats each text and expression part of a pattern, appending the results to `result`
void MessageFormatter::formatPattern(MessageContext& context, const Environment& globalEnv, const Pattern& pat, UErrorCode &status, UnicodeString& result) const {
    CHECK_ERROR(status);

    for (int32_t i = 0; i < pat.numParts(); i++) {
        const PatternPart& part = pat.getPart(i);
        if (part.isText()) {
            result += part.asText();
        } else if (part.isMarkup()) {
            // Markup is ignored
        } else {
	      // Format the expression
              LocalPointer<InternalValue> partVal(
                  formatExpression({}, globalEnv, part.contents(), context, status));
              FormattedPlaceholder partResult = partVal->forceFormatting(context.getErrors(),
                                                                         status);
              // Force full evaluation, e.g. applying default formatters to
	      // unformatted input (or formatting numbers as strings)
              result += partResult.formatToString(locale, status);
              // Handle formatting errors. `formatToString()` can't take a context and thus can't
              // register an error directly
              if (status == U_MF_FORMATTING_ERROR) {
                  status = U_ZERO_ERROR;
                  // TODO: The name of the formatter that failed is unavailable.
                  // Not ideal, but it's hard for `formatToString()`
                  // to pass along more detailed diagnostics
                  context.getErrors().setFormattingError(status);
              }
        }
    }
}

// ------------------------------------------------------
// Selection

// See https://github.com/unicode-org/message-format-wg/blob/main/spec/formatting.md#resolve-selectors
// `res` is a vector of ResolvedSelectors
void MessageFormatter::resolveSelectors(MessageContext& context, const Environment& env, UErrorCode &status, UVector& res) const {
    CHECK_ERROR(status);
    U_ASSERT(!dataModel.hasPattern());

    const VariableName* selectors = dataModel.getSelectorsInternal();
    // 1. Let res be a new empty list of resolved values that support selection.
    // (Implicit, since `res` is an out-parameter)
    // 2. For each expression exp of the message's selectors
    for (int32_t i = 0; i < dataModel.numSelectors(); i++) {
        // 2i. Let rv be the resolved value of exp.
        LocalPointer<InternalValue> rv(formatOperand({}, env, Operand(selectors[i]), context, status));
        if (rv->canSelect()) {
            // 2ii. If selection is supported for rv:
            // (True if this code has been reached)
        } else {
            // 2iii. Else:
            // Let nomatch be a resolved value for which selection always fails.
            // Append nomatch as the last element of the list res.
            // Emit a Selection Error.
            // (Note: in this case, rv, being a fallback, serves as `nomatch`)
            DynamicErrors& err = context.getErrors();
            err.setSelectorError(rv->getFunctionName(), status);
            rv.adoptInstead(new InternalValue(FormattedPlaceholder(rv->getFallback())));
            if (!rv.isValid()) {
                status = U_MEMORY_ALLOCATION_ERROR;
                return;
            }
        }
        // 2ii(a). Append rv as the last element of the list res.
        // (Also fulfills 2iii)
        res.adoptElement(rv.orphan(), status);
    }
}

bool MessageFormatter::selectorsMatch(MessageContext& context, const UVector& selectorList, const SelectorKeys& keys, UErrorCode& errorCode) const {
    if (U_FAILURE(errorCode))
        return false;
    int32_t len = selectorList.size(); // Guaranteed by earlier check to be the same length as keys
    const Key* keyList = keys.getKeysInternal();
    for (int32_t i = 0; i < len; i++) {
        if (keyList[i].isWildcard())
            continue;
        UnicodeString k = StandardFunctions::normalizeNFC(keyList[i].asLiteral().unquoted());
        InternalValue* sel = ((InternalValue*) selectorList[i]);
        bool value = sel->matchSelector(context.getErrors(), k, errorCode);
        if (U_FAILURE(errorCode))
            return false;
        if (!value)
            return false;
    }
    return true;
}

// Returns: Better if keys1 is better than keys2;
// Worse if keys1 is worse than keys2;
// Same otherwise
SelectorCompareResult MessageFormatter::selectorsCompare(MessageContext& context, const UVector& selectorList, const SelectorKeys& keys1, const SelectorKeys& keys2, UErrorCode& errorCode) const {
    if (U_FAILURE(errorCode))
        return { };
    int32_t len = selectorList.size(); // Guaranteed by earlier check to be the same length as keys1 and keys2
    const Key* keyList1 = keys1.getKeysInternal();
    const Key* keyList2 = keys2.getKeysInternal();
    for (int32_t i = 0; i < len; i++) {
        // * is worse than any other key
        if (keyList1[i].isWildcard() && (!keyList2[i].isWildcard()))
            return SelectorCompareResult::Worse;
        // Any other key is better than *
        if (!keyList1[i].isWildcard() && keyList2[i].isWildcard())
            return SelectorCompareResult::Better;
        // If both are wildcards, compare the rest of the keys
        if (keyList1[i].isWildcard())
            continue;
        UnicodeString k1 = StandardFunctions::normalizeNFC(keyList1[i].asLiteral().unquoted());
        UnicodeString k2 = StandardFunctions::normalizeNFC(keyList2[i].asLiteral().unquoted());
        InternalValue* sel = ((InternalValue*) selectorList[i]);
        SelectorCompareResult result = sel->compareSelector(context.getErrors(), k1, k2, errorCode);
        if (U_FAILURE(errorCode))
            return { };
        switch (result) {
        case SelectorCompareResult::Same:
            continue;
        default:
            return result;
        }
    }
    return SelectorCompareResult::Same;
}

void MessageFormatter::formatSelectors(MessageContext& context, const Environment& env, UErrorCode &status, UnicodeString& result) const {
    CHECK_ERROR(status);

    // See https://github.com/unicode-org/message-format-wg/blob/main/spec/formatting.md#pattern-selection

    // Resolve Selectors
    // selectorList is a vector of InternalValues
    LocalPointer<UVector> selectorList(createUVector(status));
    CHECK_ERROR(status);
    resolveSelectors(context, env, status, *selectorList);
    CHECK_ERROR(status);

    // Let bestVariant be undefined.
    const Variant* bestVariant = nullptr;

    // For each variant in the list:
    const Variant* variants = dataModel.getVariantsInternal();
    for (int32_t i = 0; i < dataModel.numVariants(); i++) {
        const SelectorKeys& keys = variants[i].getKeys();
        bool match = selectorsMatch(context, *selectorList, keys, status);
        if (U_FAILURE(status))
            return;
        if (!match)
            continue;
        if (!bestVariant)
            bestVariant = &variants[i];
        else if (selectorsCompare(context, *selectorList, keys, bestVariant->getKeys(), status) == SelectorCompareResult::Better)
            bestVariant = &variants[i];
        if (U_FAILURE(status))
            return;
    }
    U_ASSERT(bestVariant);
    formatPattern(context, env, bestVariant->getPattern(), status, result);
}

// Note: this is non-const due to the function registry being non-const, which is in turn
// due to the values (`FormatterFactory` objects in the map) having mutable state.
// In other words, formatting a message can mutate the underlying `MessageFormatter` by changing
// state within the factory objects that represent custom formatters.
UnicodeString MessageFormatter::formatToString(const MessageArguments& arguments, UErrorCode &status) {
    EMPTY_ON_ERROR(status);

    // Create a new context with the given arguments and the `errors` structure
    MessageContext context(arguments, *errors, status);
    UnicodeString result;

    if (!(errors->hasSyntaxError() || errors->hasDataModelError())) {
        // Create a new environment that will store closures for all local variables
        // Check for unresolved variable errors
        // checkDeclarations needs a reference to the pointer to the environment
        // since it uses its `env` argument as an out-parameter. So it needs to be
        // temporarily not a LocalPointer...
        Environment* env(Environment::create(status));
        checkDeclarations(context, env, status);
        // ...and then it's adopted to avoid leaks
        LocalPointer<Environment> globalEnv(env);

        if (dataModel.hasPattern()) {
            formatPattern(context, *globalEnv, dataModel.getPattern(), status, result);
        } else {
            // Check for errors/warnings -- if so, then the result of pattern selection is the fallback value
            // See https://www.unicode.org/reports/tr35/tr35-messageFormat.html#pattern-selection
            const DynamicErrors& err = context.getErrors();
            if (err.hasSyntaxError() || err.hasDataModelError()) {
                result += REPLACEMENT;
            } else {
                formatSelectors(context, *globalEnv, status, result);
            }
        }
    }

    // Update status according to all errors seen while formatting
    if (signalErrors) {
        context.checkErrors(status);
    }
    if (U_FAILURE(status)) {
        result.remove();
    }
    return result;
}

// ----------------------------------------
// Checking for resolution errors

void MessageFormatter::check(MessageContext& context, const Environment& localEnv, const OptionMap& options, UErrorCode& status) const {
    // Check the RHS of each option
    for (int32_t i = 0; i < options.size(); i++) {
        const Option& opt = options.getOption(i, status);
        CHECK_ERROR(status);
        check(context, localEnv, opt.getValue(), status);
    }
}

void MessageFormatter::check(MessageContext& context, const Environment& localEnv, const Operand& rand, UErrorCode& status) const {
    // Nothing to check for literals
    if (rand.isLiteral() || rand.isNull()) {
        return;
    }

    // Check that variable is in scope
    const VariableName& var = rand.asVariable();
    UnicodeString normalized = StandardFunctions::normalizeNFC(var);

    // Check local scope
    if (localEnv.has(normalized)) {
        return;
    }
    // Check global scope
    context.getGlobal(normalized, status);
    if (status == U_ILLEGAL_ARGUMENT_ERROR) {
        status = U_ZERO_ERROR;
        context.getErrors().setUnresolvedVariable(var, status);
    }
    // Either `var` is a global, or some other error occurred.
    // Nothing more to do either way
    return;
}

void MessageFormatter::check(MessageContext& context, const Environment& localEnv, const Expression& expr, UErrorCode& status) const {
    // Check for unresolved variable errors
    if (expr.isFunctionCall()) {
        const Operator* rator = expr.getOperator(status);
        U_ASSERT(U_SUCCESS(status));
        const Operand& rand = expr.getOperand();
        check(context, localEnv, rand, status);
        check(context, localEnv, rator->getOptionsInternal(), status);
    }
}

// Check for resolution errors
void MessageFormatter::checkDeclarations(MessageContext& context, Environment*& env, UErrorCode &status) const {
    CHECK_ERROR(status);

    const Binding* decls = getDataModel().getLocalVariablesInternal();
    U_ASSERT(env != nullptr && (decls != nullptr || getDataModel().bindingsLen == 0));

    for (int32_t i = 0; i < getDataModel().bindingsLen; i++) {
        const Binding& decl = decls[i];
        const Expression& rhs = decl.getValue();
        check(context, *env, rhs, status);

        // Add a closure to the global environment,
        // memoizing the value of localEnv up to this point

        // Add the LHS to the environment for checking the next declaration
        env = Environment::create(StandardFunctions::normalizeNFC(decl.getVariable()),
                                  Closure(rhs, *env),
                                  env,
                                  status);
        CHECK_ERROR(status);
    }
}
} // namespace message2

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_MF2 */

#endif /* #if !UCONFIG_NO_FORMATTING */

#endif /* #if !UCONFIG_NO_NORMALIZATION */
