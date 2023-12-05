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

// Export an explicit template instantiation of the LocalPointer that is used as a
// data member of various MessageFormatDataModel classes.
// (When building DLLs for Windows this is required.)
// (See messageformat2_data_model_forward_decls.h for similar examples.)
#if U_PF_WINDOWS <= U_PLATFORM && U_PLATFORM <= U_PF_CYGWIN && defined(_MSC_VER)
// Ignore warning 4661 as LocalPointerBase does not use operator== or operator!=
#pragma warning(disable: 4661)
#endif

namespace message2 {

using namespace data_model;

#define TEXT_SELECTOR UnicodeString("select")

// ------------------------------------------------------
// Formatting

// The result of formatting a literal is just itself.
static Formattable evalLiteral(const Literal& lit) {
    return Formattable(lit.unquoted());
}

// Assumes that `var` is a message argument; sets the input in the context
// to the argument's value.
void MessageFormatter::evalArgument(const VariableName& var, ExpressionContext& context) const {
    const MessageContext& c = context.messageContext();

    U_ASSERT(c.hasGlobal(var));
    // The fallback for a variable name is itself.
    context.setFallbackTo(var);
    if (c.hasGlobalAsFormattable(var)) {
        context.setInput(c.getGlobalAsFormattable(var));
    } else {
        context.setInput(c.getGlobalAsObject(var));
    }
}

// Sets the input to the contents of the literal
void MessageFormatter::formatLiteral(const Literal& lit, ExpressionContext& context) const {
    // The fallback for a literal is itself.
    context.setFallbackTo(lit);
    context.setInput(evalLiteral(lit));
}

void MessageFormatter::formatOperand(const Environment& env, const Operand& rand, ExpressionContext& context, UErrorCode &status) const {
    CHECK_ERROR(status);
    if (rand.isNull()) {
        context.setNoOperand();
        return;
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
          formatExpression(rhs.getEnv(), rhs.getExpr(), context, status);
          return;
        }
        // Use fallback per
        // https://github.com/unicode-org/message-format-wg/blob/main/spec/formatting.md#fallback-resolution
        context.setFallbackTo(var);
        // Variable wasn't found in locals -- check if it's global
        if (context.messageContext().hasGlobal(var)) {
            evalArgument(var, context);
            return;
        } else {
            // Unbound variable -- set a resolution error
            context.messageContext().getErrors().setUnresolvedVariable(var);
            return;
        }
    } else if (rand.isLiteral()) {
        formatLiteral(rand.asLiteral(), context);
        return;
    }
}

// Resolves a function's options, recording the value of each option in the context
void MessageFormatter::resolveOptions(const Environment& env, const OptionMap& options, ExpressionContext& context, UErrorCode& status) const {
    CHECK_ERROR(status);

    for (auto iter = options.begin(); iter != options.end(); ++iter) {
        const UnicodeString& k = iter.first();
        const Operand& v = iter.second();

        // Options are fully evaluated before calling the function
        // Create a new context for formatting the right-hand side of the option
        ExpressionContext rhsContext = context.create();
        // Format the operand in its own context
        formatOperand(env, v, rhsContext, status);
        // If formatting succeeded, pass the string
        if (rhsContext.hasStringOutput()) {
            context.setStringOption(k, rhsContext.getStringOutput());
        } else if (rhsContext.hasFormattableInput()) {
            // (Fall back to the input if the result was a formatted number)
            const Formattable& f = rhsContext.getFormattableInput();
            switch (f.getType()) {
                case Formattable::Type::kDate: {
                    context.setDateOption(k, f.getDate());
                    break;
                }
                case Formattable::Type::kDouble: {
                    context.setNumericOption(k, f.getDouble());
                    break;
                }
                case Formattable::Type::kLong: {
                    context.setNumericOption(k, f.getLong());
                    break;
                }
                case Formattable::Type::kInt64: {
                    context.setNumericOption(k, (double) f.getInt64());
                    break;
                }
                case Formattable::Type::kString: {
                    context.setStringOption(k, f.getString());
                    break;
                }
                default: {
                    // Options with array or object types are ignored
                    continue;
                }
            }
        } else if (rhsContext.hasObjectInput()) {
	    context.setObjectOption(k, rhsContext.getObjectInputPointer());
        } else {
            // Ignore fallbacks
            U_ASSERT(rhsContext.isFallback());
        }
    }
}

// Formats an expression using `globalEnv` for the values of variables
void MessageFormatter::formatExpression(const Environment& globalEnv, const Expression& expr, ExpressionContext& context, UErrorCode &status) const {
    CHECK_ERROR(status);

    // Formatting error
    if (expr.isReserved()) {
        context.messageContext().getErrors().setReservedError();
        U_ASSERT(context.isFallback());
        return;
    }

    const Operand& rand = expr.getOperand();
    // Format the operand (formatOperand handles the case of a null operand)
    formatOperand(globalEnv, rand, context, status);

    if (expr.isFunctionCall()) {
        const Operator& rator = expr.getOperator();
        const FunctionName& functionName = rator.getFunctionName();
        const OptionMap& options = rator.getOptions();
        // Resolve the options
        resolveOptions(globalEnv, options, context, status);

        // Don't call the function on error values
        if (context.isFallback()) {
            return;
        }

        // Call the formatter function
        context.evalFormatterCall(functionName, status);
        // If the call was successful, nothing more to do
        if (context.hasOutput() && U_SUCCESS(status)) {
            return;
        } else if (!(context.messageContext().getErrors().hasError())) {
            // Set formatting warning if formatting function had no output
            // but didn't set an error or warning
            context.messageContext().getErrors().setFormattingError(functionName.toString());
        }

        // If we reached this point, the formatter is null --
        // must have been a previous unknown function warning
        if (rand.isNull()) {
            context.setFallbackTo(functionName);
        }
        context.setFallback();
        return;
    }
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
	      ExpressionContext context(globalContext);
	      // Format the expression
	      formatExpression(globalEnv, part.contents(), context, status);
	      // Force full evaluation, e.g. applying default formatters to
	      // unformatted input (or formatting numbers as strings)
	      context.formatToString(locale, status);
	      CHECK_ERROR(status);
	      result += context.getStringOutput();
        }
    }
}

// ------------------------------------------------------
// Selection

// See https://github.com/unicode-org/message-format-wg/blob/main/spec/formatting.md#resolve-selectors
void MessageFormatter::resolveSelectors(MessageContext& context, const Environment& env, const ExpressionList& selectors, UErrorCode &status, std::vector<ExpressionContext>& res) const {
    CHECK_ERROR(status);

    // 1. Let res be a new empty list of resolved values that support selection.
    // (Implicit, since `res` is an out-parameter)
    // 2. For each expression exp of the message's selectors
    for (int32_t i = 0; i < (int32_t) selectors.size(); i++) {
        ExpressionContext rv(context);
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
        res.push_back(std::move(rv));
    }
}

// See https://github.com/unicode-org/message-format-wg/blob/main/spec/formatting.md#resolve-preferences
void MessageFormatter::matchSelectorKeys(const std::vector<UnicodeString>& keys,
					 ExpressionContext& rv,
					 std::vector<UnicodeString>& matches,
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
void MessageFormatter::resolvePreferences(std::vector<ExpressionContext>& res,
					  const VariantMap& variants,
					  std::vector<std::vector<UnicodeString>>& pref,
					  UErrorCode &status) const {
    CHECK_ERROR(status);

    // 1. Let pref be a new empty list of lists of strings.
    // (Implicit, since `pref` is an out-parameter)
    UnicodeString ks;
    int32_t numVariants = variants.size();
    std::vector<UnicodeString> matches(numVariants);
    // 2. For each index i in res
    for (int32_t i = 0; i < (int32_t) res.size(); i++) {
        // 2i. Let keys be a new empty list of strings.
	std::vector<UnicodeString> keys;
        // 2ii. For each variant `var` of the message
        for (auto iter = variants.begin(); iter != variants.end(); ++iter) {
            const SelectorKeys& selectorKeys = iter.first();

            // Note: Here, `var` names the key list of `var`,
            // not a Variant itself
            const KeyList& var = selectorKeys.getKeys();
            // 2ii(a). Let `key` be the `var` key at position i.
            U_ASSERT(i < (int32_t) var.size()); // established by semantic check in formatSelectors()
            const Key& key = var[i];
            // 2ii(b). If `key` is not the catch-all key '*'
            if (!key.isWildcard()) {
                // 2ii(b)(a) Assert that key is a literal.
                // (Not needed)
                // 2ii(b)(b) Let `ks` be the resolved value of `key`.
                ks = key.asLiteral().unquoted();
                // 2ii(b)(c) Append `ks` as the last element of the list `keys`.
                keys.push_back(ks);
            }
        }
        // 2iii. Let `rv` be the resolved value at index `i` of `res`.
        U_ASSERT(i < (int32_t) res.size());
        ExpressionContext& rv = res[i];
        // 2iv. Let matches be the result of calling the method MatchSelectorKeys(rv, keys)
        matchSelectorKeys(keys, rv, matches, status);
        // 2v. Append `matches` as the last element of the list `pref`
	pref[i] = std::move(matches);
    }
}

// See https://github.com/unicode-org/message-format-wg/blob/main/spec/formatting.md#filter-variants
void MessageFormatter::filterVariants(const VariantMap& variants,
				      const std::vector<std::vector<UnicodeString>>& pref,
				      std::vector<MessageFormatter::PrioritizedVariant>& vars,
				      UErrorCode &status) const {
    CHECK_ERROR(status);

    // 1. Let `vars` be a new empty list of variants.
    // (Not needed since `vars` is an out-parameter)
    // 2. For each variant `var` of the message:
    for (auto iter = variants.begin(); iter != variants.end(); ++iter) {
        const SelectorKeys& selectorKeys = iter.first();
        const Pattern& p = iter.second();

        // Note: Here, `var` names the key list of `var`,
        // not a Variant itself
        const KeyList& var = selectorKeys.getKeys();
        // 2i. For each index `i` in `pref`:
        bool noMatch = false;
        for (int32_t i = 0; i < (int32_t) pref.size(); i++) {
            // 2i(a). Let `key` be the `var` key at position `i`.
            U_ASSERT(i < (int32_t) var.size());
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
	    const std::vector<UnicodeString>& matches = pref.at(i);
            // 2i(f). If `matches` includes `ks`
	    if (std::find(matches.cbegin(), matches.cend(), ks) != std::end(matches)) {
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
	    PrioritizedVariant tuple(-1, selectorKeys, p);
	    vars.push_back(std::move(tuple));
        }
    }
}

// See https://github.com/unicode-org/message-format-wg/blob/main/spec/formatting.md#sort-variants
// Leaves the preferred variant as element 0 in `sortable`
// Note: this sorts in-place, so `sortable` is just `vars`
void MessageFormatter::sortVariants(const std::vector<std::vector<UnicodeString>>& pref,
				    std::vector<MessageFormatter::PrioritizedVariant>& vars) const {
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
	const std::vector<UnicodeString>& matches = pref.at(i);
        // 5ii. Let `minpref` be the integer count of items in `matches`.
        int32_t minpref = matches.size();
        // 5iii. For each tuple `tuple` of `sortable`:
        for (int32_t j = 0; j < (int32_t) vars.size(); j++) {
            PrioritizedVariant& tuple = vars[j];
            // 5iii(a). Let matchpref be an integer with the value minpref.
            int32_t matchpref = minpref;
            // 5iii(b). Let `key` be the tuple variant key at position `i`.
            const KeyList& tupleVariantKeys = tuple.keys.getKeys();
            U_ASSERT(i < ((int32_t) tupleVariantKeys.size())); // Given by earlier semantic checking
            const Key& key = tupleVariantKeys[i];
            // 5iii(c) If `key` is not the catch-all key '*':
            if (!key.isWildcard()) {
                // 5iii(c)(a). Assert that `key` is a literal.
                // (Not needed)
                // 5iii(c)(b). Let `ks` be the resolved value of `key`.
                UnicodeString ks = key.asLiteral().unquoted();
                // 5iii(c)(c) Let matchpref be the integer position of ks in `matches`.
                auto match = std::find(matches.cbegin(), matches.cend(), ks);
                U_ASSERT(match != std::end(matches));
		matchpref = match - matches.cbegin();
            }
            // 5iii(d) Set the `tuple` integer value as matchpref.
            tuple.priority = matchpref;
        }
        // 5iv. Set `sortable` to be the result of calling the method SortVariants(`sortable`)
	std::sort(vars.begin(), vars.end());
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
        formatLiteral(rand.asLiteral(), context);
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
            evalArgument(var, context);
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
        context.messageContext().getErrors().setReservedError();
        U_ASSERT(context.isFallback());
        return;
    }

    // Function call -- resolve the operand and options
    if (expr.isFunctionCall()) {
        const Operator& rator = expr.getOperator();
        context.setFunctionName(rator.getFunctionName());
        resolveOptions(env, rator.getOptions(), context, status);
        // Operand may be the null argument, but resolveVariables() handles that
        formatOperand(env, expr.getOperand(), context, status);
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
                U_ASSERT(!context.hasInput());
            } else {
                context.promoteFallbackToInput();
            }
        }
    } else {
        // Determine the type of error to set
        if (context.hasFunctionName()) {
            const FunctionName& fn = context.getFunctionName();
            // A selector used as a formatter is a selector error
            if (context.hasFormatter()) {
                err.setSelectorError(fn);
            } else {
                // Otherwise, the error is an unknown function error
                err.setUnknownFunction(fn);
            }
        } else {
            // No function name -- this is a missing selector annotation error
            U_ASSERT(err.hasMissingSelectorAnnotationError());
        }
        context.clearFunctionName();
        context.clearFunctionOptions();
        context.setFallback();
    }
}

void MessageFormatter::formatSelectors(MessageContext& context, const Environment& env, const ExpressionList& selectors, const VariantMap& variants, UErrorCode &status, UnicodeString& result) const {
    CHECK_ERROR(status);

    // See https://github.com/unicode-org/message-format-wg/blob/main/spec/formatting.md#pattern-selection

    // Resolve Selectors
    // res is a vector of ResolvedExpressions
    int32_t numSelectors = selectors.size();

    // vector of ExpressionContexts
    std::vector<ExpressionContext> res;
    resolveSelectors(context, env, selectors, status, res);

    // Resolve Preferences
    // pref is a vector of vectors of strings
    std::vector<std::vector<UnicodeString>> pref(numSelectors);
    resolvePreferences(res, variants, pref, status);

    // Filter Variants
    // vars is a vector of PrioritizedVariants
    std::vector<PrioritizedVariant> vars;
    filterVariants(variants, pref, vars, status);

    // Sort Variants and select the final pattern
    // Note: `sortable` in the spec is just `vars` here,
    // which is sorted in-place
    sortVariants(pref, vars);

    // 6. Let `var` be the `variant` element of the first element of `sortable`.
    U_ASSERT(vars.size() > 0); // This should have been checked earlier (having 0 variants would be a data model error)
    const PrioritizedVariant& var = vars.at(0);
    // 7. Select the pattern of `var`
    const Pattern& pat = var.pat;

    // Format the pattern
    formatPattern(context, env, pat, status, result);
}

UBool MessageFormatter::PrioritizedVariant::operator<(const MessageFormatter::PrioritizedVariant& other) const {
  if (priority < other.priority) {
      return true;
  }
  return false;
}

MessageFormatter::PrioritizedVariant::~PrioritizedVariant() {}

UnicodeString MessageFormatter::getPattern() const {
    // Converts the current data model back to a string
    UnicodeString result;
    Serializer serializer(getDataModel(), result);
    serializer.serialize();
    return result;
}

// Precondition: custom function registry exists
const FunctionRegistry& MessageFormatter::getCustomFunctionRegistry() const {
    U_ASSERT(hasCustomFunctionRegistry());
    return *customFunctionRegistry;
}

UnicodeString MessageFormatter::formatToString(const MessageArguments& arguments, UErrorCode &status) const {
    EMPTY_ON_ERROR(status);

    // Create a new environment that will store closures for all local variables
    Environment* env = Environment::create(status);
    // Create a new context with the given arguments and the `errors` structure
    LocalPointer<MessageContext> context(MessageContext::create(*this, arguments, errors, status));
    EMPTY_ON_ERROR(status);

    // Check for unresolved variable errors
    checkDeclarations(*context, env, status);
    LocalPointer<Environment> globalEnv(env);

    UnicodeString result;
    if (!dataModel.hasSelectors()) {
        formatPattern(*context, *globalEnv, dataModel.getPattern(), status, result);
    } else {
        // Check for errors/warnings -- if so, then the result of pattern selection is the fallback value
        // See https://github.com/unicode-org/message-format-wg/blob/main/spec/formatting.md#pattern-selection
        const DynamicErrors& err = context->getErrors();
        if (err.hasSyntaxError() || err.hasDataModelError()) {
            result += REPLACEMENT;
        } else {
            formatSelectors(*context, *globalEnv, dataModel.getSelectors(), dataModel.getVariants(), status, result);
        }
    }
    // Update status according to all errors seen while formatting
    context->checkErrors(status);
    return result;
}

// ----------------------------------------
// Checking for resolution errors

void MessageFormatter::check(MessageContext& context, const Environment& localEnv, const OptionMap& options, UErrorCode &status) const {
    CHECK_ERROR(status);

    // Check the RHS of each option
    for (auto iter = options.begin(); iter != options.end(); ++iter) {
        check(context, localEnv, iter.second(), status);
    }
}

void MessageFormatter::check(MessageContext& context, const Environment& localEnv, const Operand& rand, UErrorCode &status) const {
    CHECK_ERROR(status);

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
    if (context.hasGlobalAsFormattable(var) || context.hasGlobalAsObject(var)) {
        return;
    }
    context.getErrors().setUnresolvedVariable(var);
}

void MessageFormatter::check(MessageContext& context, const Environment& localEnv, const Expression& expr, UErrorCode &status) const {
    CHECK_ERROR(status);

    // Check for unresolved variable errors
    if (expr.isFunctionCall()) {
        const Operator& rator = expr.getOperator();
        const Operand& rand = expr.getOperand();
        check(context, localEnv, rand, status);
        check(context, localEnv, rator.getOptions(), status);
    }
}

// Check for resolution errors
void MessageFormatter::checkDeclarations(MessageContext& context, Environment*& env, UErrorCode &status) const {
    CHECK_ERROR(status);

    const Bindings& decls = getDataModel().getLocalVariables();
    U_ASSERT(env != nullptr);

    for (int32_t i = 0; i < (int32_t) decls.size(); i++) {
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
