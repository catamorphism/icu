// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/messageformat2.h"
#include "messageformat2_checker.h"
#include "messageformat2_context.h"
#include "messageformat2_expression_context.h"
#include "messageformat2_macros.h"
#include "messageformat2_serializer.h"
#include "uvector.h" // U_ASSERT

#include <algorithm>

U_NAMESPACE_BEGIN

namespace message2 {

using namespace data_model;

#define TEXT_SELECTOR UnicodeString("select")

// ------------------------------------------------------
// Formatting

// The result of formatting a literal is just itself.
static Formattable evalLiteral(const Literal& lit) {
    return Formattable(lit.unquoted());
}

// Assumes that `var` is a message argument; returns the argument's value.
[[nodiscard]] FormattedValue MessageFormatter::evalArgument(const VariableName& var, ExpressionContext& context) const {
    const MessageContext& c = context.messageContext();

    U_ASSERT(c.hasGlobal(var));
    // The fallback for a variable name is itself.
    context.setFallbackTo(var);
    return (FormattedValue(c.getGlobal(var)));
}

// Returns the contents of the literal
[[nodiscard]] FormattedValue MessageFormatter::formatLiteral(const Literal& lit, ExpressionContext& context) const {
    // The fallback for a literal is itself.
    context.setFallbackTo(lit);
    return FormattedValue(evalLiteral(lit));
}

[[nodiscard]] FormattedValue MessageFormatter::formatOperand(const Environment& env, const Operand& rand, ExpressionContext& context, UErrorCode &status) const {
    if (U_FAILURE(status)) {
        return {};
    }

    if (rand.isNull()) {
        return FormattedValue();
    }
    if (rand.isVariable()) {
        // Check if it's local or global
        // TODO: Currently, this code allows name shadowing, but depending on the
        // resolution of:
        //   https://github.com/unicode-org/message-format-wg/issues/310
        // it might need to forbid it.
        const VariableName& var = rand.asVariable();
        // TODO: Currently, this code implements lazy evaluation of locals.
        // That is, the environment binds names to a closure, not a resolved value.
        // Eager vs. lazy evaluation is an open issue:
        // see https://github.com/unicode-org/message-format-wg/issues/299

        // Look up the variable in the environment
        if (env.has(var)) {
          // `var` is a local -- look it up
          const Closure& rhs = env.lookup(var);
          // Format the expression using the environment from the closure
          return formatExpression(rhs.getEnv(), rhs.getExpr(), context, status);
        }
        // Use fallback per
        // https://github.com/unicode-org/message-format-wg/blob/main/spec/formatting.md#fallback-resolution
        context.setFallbackTo(var);
        // Variable wasn't found in locals -- check if it's global
        if (context.messageContext().hasGlobal(var)) {
            return evalArgument(var, context);
        } else {
            // Unbound variable -- set a resolution error
            context.messageContext().getErrors().setUnresolvedVariable(var, status);
            return FormattedValue(context.fallback);
        }
    } else {
        U_ASSERT(rand.isLiteral());
        return formatLiteral(rand.asLiteral(), context);
    }
}

// Resolves a function's options, recording the value of each option in the context
void MessageFormatter::resolveOptions(const Environment& env, const OptionMap& options, ExpressionContext& context, UErrorCode& status) const {
    LocalPointer<UVector> optionsVector(createUVector(status));
    CHECK_ERROR(status);
    LocalPointer<ResolvedFunctionOption> resolvedOpt;
    for (int i = 0; i < options.size(); i++) {
        const Option& opt = options.getOption(i, status);
        CHECK_ERROR(status);
        const UnicodeString& k = opt.getName();
        const Operand& v = opt.getValue();

        // Options are fully evaluated before calling the function
        // Create a new context for formatting the right-hand side of the option
        ExpressionContext rhsContext = context.create();
        // Format the operand in its own context
        FormattedValue rhsVal = formatOperand(env, v, rhsContext, status);
        CHECK_ERROR(status);
        if (!rhsVal.isFallback()) {
            resolvedOpt.adoptInstead(create<ResolvedFunctionOption>(ResolvedFunctionOption(k, rhsVal.asFormattable()), status));
            CHECK_ERROR(status);
            optionsVector->adoptElement(resolvedOpt.orphan(), status);
        }
    }

    context.adoptFunctionOptions(optionsVector.orphan(), status);
}

// Formats an expression using `globalEnv` for the values of variables
[[nodiscard]] FormattedValue MessageFormatter::formatExpression(const Environment& globalEnv, const Expression& expr, ExpressionContext& context, UErrorCode &status) const {
    if (U_FAILURE(status)) {
        return {};
    }

    // Formatting error
    if (expr.isReserved()) {
        context.messageContext().getErrors().setReservedError(status);
        U_ASSERT(context.isFallback());
        return FormattedValue(context.fallback);
    }

    const Operand& rand = expr.getOperand();
    // Format the operand (formatOperand handles the case of a null operand)
    FormattedValue randVal = formatOperand(globalEnv, rand, context, status);

    if (!expr.isFunctionCall()) {
        return randVal;
    }
    const Operator& rator = expr.getOperator();
    const FunctionName& functionName = rator.getFunctionName();
    const OptionMap& options = rator.getOptionsInternal();
    // Resolve the options
    resolveOptions(globalEnv, options, context, status);

    // Don't call the function on error values
    if (randVal.isFallback()) {
        return FormattedValue(context.fallback);
    }

    // Call the formatter function
    context.setContents(std::move(randVal));
    // The fallback for a nullary function call is the function name
    if (rand.isNull()) {
        context.setFallbackTo(functionName);
    }
    FormattedValue returnVal = context.evalFormatterCall(functionName, status);
    // If the call was successful, nothing more to do
    if (U_SUCCESS(status)) {
        return returnVal;
    } else if (!(context.messageContext().getErrors().hasError())) {
        // Set formatting warning if formatting function had no output
        // but didn't set an error or warning
        context.messageContext().getErrors().setFormattingError(functionName.toString(), status);
    }
    // If we reached this point, the formatter is null --
    // must have been a previous unknown function warning
    return FormattedValue(context.fallback);
}

// Formats each text and expression part of a pattern, appending the results to `result`
void MessageFormatter::formatPattern(MessageContext& globalContext, const Environment& globalEnv, const Pattern& pat, UErrorCode &status, UnicodeString& result) const {
    CHECK_ERROR(status);

    for (int32_t i = 0; i < pat.numParts(); i++) {
        const PatternPart& part = pat.getPart(i);
        if (part.isText()) {
            result += part.asText();
        } else {
              // Create a new context to evaluate the expression part
              ExpressionContext context(globalContext, UnicodeString(REPLACEMENT));
	      // Format the expression
	      FormattedValue partVal = formatExpression(globalEnv, part.contents(), context, status);
	      // Force full evaluation, e.g. applying default formatters to
	      // unformatted input (or formatting numbers as strings)
	      result += context.formatToString(locale, partVal, status);
        }
    }
}

// ------------------------------------------------------
// Selection

// See https://github.com/unicode-org/message-format-wg/blob/main/spec/formatting.md#resolve-selectors
// `res` is a vector of ExpressionContexts
void MessageFormatter::resolveSelectors(MessageContext& context, const Environment& env, UErrorCode &status, UVector& res) const {
    CHECK_ERROR(status);
    U_ASSERT(dataModel.hasSelectors());

    const Expression* selectors = dataModel.selectors.getAlias();
    // 1. Let res be a new empty list of resolved values that support selection.
    // (Implicit, since `res` is an out-parameter)
    // 2. For each expression exp of the message's selectors
    for (int32_t i = 0; i < dataModel.numSelectors; i++) {
        ExpressionContext rv(context, UnicodeString(REPLACEMENT));
        // 2i. Let rv be the resolved value of exp.
        formatSelectorExpression(env, selectors[i], rv, status);
        if (rv.hasSelector()) {
            // 2ii. If selection is supported for rv:
            // (True if this code has been reached)
        } else {
            // 2iii. Else:
            // Let nomatch be a resolved value for which selection always fails.
            // Append nomatch as the last element of the list res.
            // Emit a Selection Error.
            // (Note: in this case, rv, being a fallback, serves as `nomatch`)
            #ifdef _DEBUG
            const Errors& err = rv.messageContext().getErrors();
            U_ASSERT(err.hasUnknownFunctionError() || err.hasSelectorError());
            U_ASSERT(rv.isFallback());
            #endif
        }
        // 2ii(a). Append rv as the last element of the list res.
        // (Also fulfills 2iii)
        ExpressionContext* ctx = create<ExpressionContext>(std::move(rv), status);
        CHECK_ERROR(status);
        res.adoptElement(ctx, status);
    }
}

// See https://github.com/unicode-org/message-format-wg/blob/main/spec/formatting.md#resolve-preferences
// `keys` and `matches` are vectors of strings
void MessageFormatter::matchSelectorKeys(const UVector& keys,
					 ExpressionContext& rv,
					 UVector& matches,
					 UErrorCode& status) const {
    CHECK_ERROR(status);

    if (rv.isFallback()) {
        // Return an empty list of matches
        return;
    }
    U_ASSERT(rv.hasSelector());

    rv.evalPendingSelectorCall(keys, matches, status);
}

// See https://github.com/unicode-org/message-format-wg/blob/main/spec/formatting.md#resolve-preferences
// `res` is a vector of ExpressionContexts; `pref` is a vector of vectors of strings
void MessageFormatter::resolvePreferences(UVector& res, UVector& pref, UErrorCode &status) const {
    CHECK_ERROR(status);

    // 1. Let pref be a new empty list of lists of strings.
    // (Implicit, since `pref` is an out-parameter)
    UnicodeString ks;
    LocalPointer<UnicodeString> ksP;
    int32_t numVariants = dataModel.numVariants;
    const Variant* variants = dataModel.variants.getAlias();
    // 2. For each index i in res
    for (int32_t i = 0; i < (int32_t) res.size(); i++) {
        // 2i. Let keys be a new empty list of strings.
        LocalPointer<UVector> keys(createUVector(status));
        CHECK_ERROR(status);
        // 2ii. For each variant `var` of the message
        for (int32_t variantNum = 0; variantNum < numVariants; variantNum++) {
            const SelectorKeys& selectorKeys = variants[variantNum].getKeys();

            // Note: Here, `var` names the key list of `var`,
            // not a Variant itself
            const Key* var = selectorKeys.getKeysInternal();
            int32_t len = selectorKeys.len;
            // 2ii(a). Let `key` be the `var` key at position i.
            U_ASSERT(i < len); // established by semantic check in formatSelectors()
            const Key& key = var[i];
            // 2ii(b). If `key` is not the catch-all key '*'
            if (!key.isWildcard()) {
                // 2ii(b)(a) Assert that key is a literal.
                // (Not needed)
                // 2ii(b)(b) Let `ks` be the resolved value of `key`.
                ks = key.asLiteral().unquoted();
                // 2ii(b)(c) Append `ks` as the last element of the list `keys`.
                ksP.adoptInstead(create<UnicodeString>(std::move(ks), status));
                CHECK_ERROR(status);
                keys->adoptElement(ksP.orphan(), status);
            }
        }
        // 2iii. Let `rv` be the resolved value at index `i` of `res`.
        U_ASSERT(i < res.size());
        ExpressionContext& rv = *(static_cast<ExpressionContext*>(res[i]));
        // 2iv. Let matches be the result of calling the method MatchSelectorKeys(rv, keys)
        LocalPointer<UVector> matches(createUVector(status));
        matchSelectorKeys(*keys, rv, *matches, status);
        // 2v. Append `matches` as the last element of the list `pref`
        pref.adoptElement(matches.orphan(), status);
    }
}

// `v` is assumed to be a vector of strings
static int32_t vectorFind(const UVector& v, const UnicodeString& k) {
    for (int32_t i = 0; i < v.size(); i++) {
        if (*static_cast<UnicodeString*>(v[i]) == k) {
            return i;
        }
    }
    return -1;
}

static UBool vectorContains(const UVector& v, const UnicodeString& k) {
    return (vectorFind(v, k) != -1);
}

// See https://github.com/unicode-org/message-format-wg/blob/main/spec/formatting.md#filter-variants
// `pref` is a vector of vectors of strings. `vars` is a vector of PrioritizedVariants
void MessageFormatter::filterVariants(const UVector& pref, UVector& vars, UErrorCode& status) const {
    const Variant* variants = dataModel.variants.getAlias();

    // 1. Let `vars` be a new empty list of variants.
    // (Not needed since `vars` is an out-parameter)
    // 2. For each variant `var` of the message:
    for (int32_t j = 0; j < dataModel.numVariants; j++) {
        const SelectorKeys& selectorKeys = variants[j].getKeys();
        const Pattern& p = variants[j].getPattern();

        // Note: Here, `var` names the key list of `var`,
        // not a Variant itself
        const Key* var = selectorKeys.getKeysInternal();
        // 2i. For each index `i` in `pref`:
        bool noMatch = false;
        for (int32_t i = 0; i < (int32_t) pref.size(); i++) {
            // 2i(a). Let `key` be the `var` key at position `i`.
            U_ASSERT(i < selectorKeys.len);
            const Key& key = var[i];
            // 2i(b). If key is the catch-all key '*':
            if (key.isWildcard()) {
                // 2i(b)(a). Continue the inner loop on pref.
                continue;
            }
            // 2i(c). Assert that `key` is a literal.
            // (Not needed)
            // 2i(d). Let `ks` be the resolved value of `key`.
            UnicodeString ks = key.asLiteral().unquoted();
            // 2i(e). Let `matches` be the list of strings at index `i` of `pref`.
            const UVector& matches = *(static_cast<UVector*>(pref[i])); // `matches` is a vector of strings
            // 2i(f). If `matches` includes `ks`
            if (vectorContains(matches, ks)) {
                // 2i(f)(a). Continue the inner loop on `pref`.
                continue;
            }
            // 2i(g). Else:
            // 2i(g)(a). Continue the outer loop on message variants.
            noMatch = true;
            break;
        }
        if (!noMatch) {
            // Append `var` as the last element of the list `vars`.
	    PrioritizedVariant* tuple = create<PrioritizedVariant>(PrioritizedVariant(-1, selectorKeys, p), status);
            CHECK_ERROR(status);
            vars.adoptElement(tuple, status);
        }
    }
}

static int32_t comparePrioritizedVariants(UElement left, UElement right) {
    const PrioritizedVariant& tuple1 = *(static_cast<const PrioritizedVariant*>(left.pointer));
    const PrioritizedVariant& tuple2 = *(static_cast<const PrioritizedVariant*>(right.pointer));
    if (tuple1 < tuple2) {
        return -1;
    }
    if (tuple1.priority == tuple2.priority) {
        return 0;
    }
    return 1;
}

// See https://github.com/unicode-org/message-format-wg/blob/main/spec/formatting.md#sort-variants
// Leaves the preferred variant as element 0 in `sortable`
// Note: this sorts in-place, so `sortable` is just `vars`
// `pref` is a vector of vectors of strings; `vars` is a vector of PrioritizedVariants
void MessageFormatter::sortVariants(const UVector& pref, UVector& vars, UErrorCode& status) const {
    CHECK_ERROR(status);

// Note: steps 1 and 2 are omitted since we use `vars` as `sortable` (we sort in-place)
    // 1. Let `sortable` be a new empty list of (integer, variant) tuples.
    // (Not needed since `sortable` is an out-parameter)
    // 2. For each variant `var` of `vars`
    // 2i. Let tuple be a new tuple (-1, var).
    // 2ii. Append `tuple` as the last element of the list `sortable`.

    // 3. Let `len` be the integer count of items in `pref`.
    int32_t len = pref.size();
    // 4. Let `i` be `len` - 1.
    int32_t i = len - 1;
    // 5. While i >= 0:
    while (i >= 0) {
        // 5i. Let `matches` be the list of strings at index `i` of `pref`.
        U_ASSERT(pref[i] != nullptr);
	const UVector& matches = *(static_cast<UVector*>(pref[i])); // `matches` is a vector of strings
        // 5ii. Let `minpref` be the integer count of items in `matches`.
        int32_t minpref = matches.size();
        // 5iii. For each tuple `tuple` of `sortable`:
        for (int32_t j = 0; j < vars.size(); j++) {
            U_ASSERT(vars[j] != nullptr);
            PrioritizedVariant& tuple = *(static_cast<PrioritizedVariant*>(vars[j]));
            // 5iii(a). Let matchpref be an integer with the value minpref.
            int32_t matchpref = minpref;
            // 5iii(b). Let `key` be the tuple variant key at position `i`.
            const Key* tupleVariantKeys = tuple.keys.getKeysInternal();
            U_ASSERT(i < tuple.keys.len); // Given by earlier semantic checking
            const Key& key = tupleVariantKeys[i];
            // 5iii(c) If `key` is not the catch-all key '*':
            if (!key.isWildcard()) {
                // 5iii(c)(a). Assert that `key` is a literal.
                // (Not needed)
                // 5iii(c)(b). Let `ks` be the resolved value of `key`.
                UnicodeString ks = key.asLiteral().unquoted();
                // 5iii(c)(c) Let matchpref be the integer position of ks in `matches`.
                int32_t matchpref = vectorFind(matches, ks);
                U_ASSERT(matchpref >= 0);
            }
            // 5iii(d) Set the `tuple` integer value as matchpref.
            tuple.priority = matchpref;
        }
        // 5iv. Set `sortable` to be the result of calling the method SortVariants(`sortable`)
        vars.sort(comparePrioritizedVariants, status);
        CHECK_ERROR(status);
        // 5v. Set `i` to be `i` - 1.
        i--;
    }
    // The caller is responsible for steps 6 and 7
    // 6. Let `var` be the `variant` element of the first element of `sortable`.
    // 7. Select the pattern of `var`
}


// Evaluate the operand
void MessageFormatter::resolveVariables(const Environment& env, const Operand& rand, ExpressionContext& context, UErrorCode &status) const {
    CHECK_ERROR(status);

    if (rand.isNull()) {
        // Nothing to do
        return;
    } else if (rand.isLiteral()) {
        // If there's already a function name set, this shouldn't have been evaluated
        U_ASSERT(!context.hasFunctionName());
        context.setContents(formatLiteral(rand.asLiteral(), context).asFormattable());
    } else {
        // Must be variable
        const VariableName& var = rand.asVariable();
        // Resolve the variable
        if (env.has(var)) {
            const Closure& referent = env.lookup(var);
            // Resolve the referent
            resolveVariables(referent.getEnv(), referent.getExpr(), context, status);
            return;
        }
        // Either this is a global var or an unbound var --
        // either way, it can't be bound to a function call.
        context.setFallbackTo(var);
        // Check globals
        if (context.messageContext().hasGlobal(var)) {
            context.setContents(evalArgument(var, context).asFormattable());
        } else {
            // Unresolved variable -- could be a previous warning. Nothing to resolve
            U_ASSERT(context.messageContext().getErrors().hasUnresolvedVariableError());
        }
    }
}

// Evaluate the expression except for not performing the top-level function call
// (which is expected to be a selector, but may not be, in error cases)
void MessageFormatter::resolveVariables(const Environment& env, const Expression& expr, ExpressionContext& context, UErrorCode &status) const {
    CHECK_ERROR(status);

    // A `reserved` is an error
    if (expr.isReserved()) {
        context.messageContext().getErrors().setReservedError(status);
        U_ASSERT(context.isFallback());
        return;
    }

    // Function call -- resolve the operand and options
    if (expr.isFunctionCall()) {
        const Operator& rator = expr.getOperator();
        context.setFunctionName(rator.getFunctionName());
        resolveOptions(env, rator.getOptionsInternal(), context, status);
        // Operand may be the null argument, but resolveVariables() handles that
        context.setContents(formatOperand(env, expr.getOperand(), context, status));
    } else {
        resolveVariables(env, expr.getOperand(), context, status);
    }
}

// Leaves `context` either as a fallback with errors,
// or in a state with a pending call to a selector that has been set
void MessageFormatter::formatSelectorExpression(const Environment& globalEnv, const Expression& expr, ExpressionContext& context, UErrorCode &status) const {
    CHECK_ERROR(status);

    // Resolve expression to determine if it's a function call
    resolveVariables(globalEnv, expr, context, status);

    DynamicErrors& err = context.messageContext().getErrors();

    // If there is a selector, then `resolveVariables()` recorded it in the context
    if (context.hasSelector()) {
        // Check if there was an error
        if (context.isFallback()) {
            // Use a null expression if it's a syntax or data model warning;
            // create a valid (non-fallback) formatted placeholder from the
            // fallback string otherwise
            if (err.hasSyntaxError() || err.hasDataModelError()) {
                U_ASSERT(!context.canFormat());
            }
        }
    } else {
        // Determine the type of error to set
        if (context.hasFunctionName()) {
            const FunctionName& fn = context.getFunctionName();
            // A selector used as a formatter is a selector error
            if (context.hasFormatter()) {
                err.setSelectorError(fn, status);
            } else {
                // Otherwise, the error is an unknown function error
                err.setUnknownFunction(fn, status);
            }
        } else {
            // No function name -- this is a missing selector annotation error
            U_ASSERT(err.hasMissingSelectorAnnotationError());
        }
        CHECK_ERROR(status);
        context.clearFunctionName();
        context.clearFunctionOptions();
        context.setFallback();
    }
}

void MessageFormatter::formatSelectors(MessageContext& context, const Environment& env, UErrorCode &status, UnicodeString& result) const {
    CHECK_ERROR(status);

    // See https://github.com/unicode-org/message-format-wg/blob/main/spec/formatting.md#pattern-selection

    // Resolve Selectors
    // res is a vector of ExpressionContexts
    LocalPointer<UVector> res(createUVector(status));
    CHECK_ERROR(status);
    resolveSelectors(context, env, status, *res);

    // Resolve Preferences
    // pref is a vector of vectors of strings
    LocalPointer<UVector> pref(createUVector(status));
    CHECK_ERROR(status);
    resolvePreferences(*res, *pref, status);

    // Filter Variants
    // vars is a vector of PrioritizedVariants
    LocalPointer<UVector> vars(createUVector(status));
    CHECK_ERROR(status);
    filterVariants(*pref, *vars, status);

    // Sort Variants and select the final pattern
    // Note: `sortable` in the spec is just `vars` here,
    // which is sorted in-place
    sortVariants(*pref, *vars, status);

    CHECK_ERROR(status);

    // 6. Let `var` be the `variant` element of the first element of `sortable`.
    U_ASSERT(vars->size() > 0); // This should have been checked earlier (having 0 variants would be a data model error)
    const PrioritizedVariant& var = *(static_cast<PrioritizedVariant*>(vars->elementAt(0)));
    // 7. Select the pattern of `var`
    const Pattern& pat = var.pat;

    // Format the pattern
    formatPattern(context, env, pat, status, result);
}

UBool PrioritizedVariant::operator<(const PrioritizedVariant& other) const {
  if (priority < other.priority) {
      return true;
  }
  return false;
}

PrioritizedVariant::~PrioritizedVariant() {}

UnicodeString MessageFormatter::getPattern() const {
    // Converts the current data model back to a string
    UnicodeString result;
    Serializer serializer(getDataModel(), result);
    serializer.serialize();
    return result;
}

// Precondition: custom function registry exists
FunctionRegistry& MessageFormatter::getCustomFunctionRegistry() const {
    U_ASSERT(hasCustomFunctionRegistry());
    return *customFunctionRegistry;
}

// Note: this is non-const due to the function registry being non-const, which is in turn
// due to the values (`FormatterFactory` objects in the map) having mutable state.
// In other words, formatting a message can mutate the underlying `MessageFormatter` by changing
// state within the factory objects that represent custom formatters.
UnicodeString MessageFormatter::formatToString(const MessageArguments& arguments, UErrorCode &status) {
    EMPTY_ON_ERROR(status);

    // Create a new environment that will store closures for all local variables
    Environment* env = Environment::create(status);
    // Create a new context with the given arguments and the `errors` structure
    MessageContext context(*this, arguments, *errors, status);

    // Check for unresolved variable errors
    checkDeclarations(context, env, status);
    LocalPointer<Environment> globalEnv(env);

    UnicodeString result;
    if (!dataModel.hasSelectors()) {
        formatPattern(context, *globalEnv, dataModel.getPattern(), status, result);
    } else {
        // Check for errors/warnings -- if so, then the result of pattern selection is the fallback value
        // See https://github.com/unicode-org/message-format-wg/blob/main/spec/formatting.md#pattern-selection
        const DynamicErrors& err = context.getErrors();
        if (err.hasSyntaxError() || err.hasDataModelError()) {
            result += REPLACEMENT;
        } else {
            formatSelectors(context, *globalEnv, status, result);
        }
    }
    // Update status according to all errors seen while formatting
    context.checkErrors(status);
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
    // Check local scope
    if (localEnv.has(var)) {
        return;
    }
    // Check global scope
    if (context.hasGlobal(var)) {
        return;
    }
    context.getErrors().setUnresolvedVariable(var, status);
}

void MessageFormatter::check(MessageContext& context, const Environment& localEnv, const Expression& expr, UErrorCode& status) const {
    // Check for unresolved variable errors
    if (expr.isFunctionCall()) {
        const Operator& rator = expr.getOperator();
        const Operand& rand = expr.getOperand();
        check(context, localEnv, rand, status);
        check(context, localEnv, rator.getOptionsInternal(), status);
    }
}

// Check for resolution errors
void MessageFormatter::checkDeclarations(MessageContext& context, Environment*& env, UErrorCode &status) const {
    CHECK_ERROR(status);

    const Binding* decls = getDataModel().getLocalVariablesInternal();
    U_ASSERT(env != nullptr && decls != nullptr);

    for (int32_t i = 0; i < getDataModel().bindingsLen; i++) {
        const Binding& decl = decls[i];
        const Expression& rhs = decl.getValue();
        check(context, *env, rhs, status);

        // Add a closure to the global environment,
        // memoizing the value of localEnv up to this point

        // Add the LHS to the environment for checking the next declaration
        env = Environment::create(decl.getVariable(), Closure(rhs, *env), env, status);
        CHECK_ERROR(status);
    }
}
} // namespace message2

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */
