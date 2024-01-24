// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/messageformat2_data_model.h"
#include "unicode/messageformat2.h"
#include "messageformat2_checker.h"
#include "messageformat2_context.h"
#include "messageformat2_macros.h"
#include "messageformat2_serializer.h"
#include "uvector.h" // U_ASSERT

#include <algorithm>

U_NAMESPACE_BEGIN

namespace message2 {

using namespace data_model;

#define TEXT_SELECTOR UnicodeString("select")

static UnicodeString defaultFallback() { return UnicodeString(REPLACEMENT); }

// ------------------------------------------------------
// Formatting

// The result of formatting a literal is just itself.
static Formattable evalLiteral(const Literal& lit) {
    return Formattable(lit.unquoted());
}

// Assumes that `var` is a message argument; returns the argument's value.
[[nodiscard]] FormattedPlaceholder MessageFormatter::evalArgument(const VariableName& var, MessageContext& context) const {
    U_ASSERT(context.hasGlobal(var));
    // The fallback for a variable name is itself.
    return (FormattedPlaceholder(context.getGlobal(var), var.declaration()));
}

// Returns the contents of the literal
[[nodiscard]] FormattedPlaceholder MessageFormatter::formatLiteral(const Literal& lit) const {
    // The fallback for a literal is itself.
    return FormattedPlaceholder(evalLiteral(lit), lit.quoted());
}

[[nodiscard]] FormattedPlaceholder MessageFormatter::formatOperand(const Environment& env,
                                                             const Operand& rand,
                                                             MessageContext& context,
                                                             UErrorCode &status) const {
    if (U_FAILURE(status)) {
        return {};
    }

    if (rand.isNull()) {
        return FormattedPlaceholder();
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
        // Variable wasn't found in locals -- check if it's global
        if (context.hasGlobal(var)) {
            return evalArgument(var, context);
        } else {
            // Unbound variable -- set a resolution error
            context.getErrors().setUnresolvedVariable(var, status);
            // Use fallback per
            // https://github.com/unicode-org/message-format-wg/blob/main/spec/formatting.md#fallback-resolution
            return FormattedPlaceholder(var.declaration());
        }
    } else {
        U_ASSERT(rand.isLiteral());
        return formatLiteral(rand.asLiteral());
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
        FormattedPlaceholder rhsVal = formatOperand(env, v, context, status);
        if (U_FAILURE(status)) {
            return {};
        }
        if (!rhsVal.isFallback()) {
            resolvedOpt.adoptInstead(create<ResolvedFunctionOption>(ResolvedFunctionOption(k, rhsVal.asFormattable()), status));
            if (U_FAILURE(status)) {
                return {};
            }
            optionsVector->adoptElement(resolvedOpt.orphan(), status);
        }
    }

    return FunctionOptions(std::move(*optionsVector), status);
}

[[nodiscard]] FormattedPlaceholder MessageFormatter::evalFormatterCall(const FunctionName& functionName,
                                                                 FormattedPlaceholder&& argument,
                                                                 FunctionOptions&& options,
                                                                 MessageContext& context,
                                                                 UErrorCode& status) const {
    if (U_FAILURE(status)) {
        return {};
    }

    DynamicErrors& errs = context.getErrors();

    UnicodeString fallback = argument.isNullOperand() ? functionName.toString() : argument.fallback;

    if (isFormatter(functionName)) {
        const Formatter& formatterImpl = getFormatter(context, functionName, status);
        if (U_FAILURE(status)) {
            return {};
        }
        UErrorCode savedStatus = status;
        FormattedPlaceholder result = formatterImpl.format(std::move(argument), std::move(options), status);
        // Update errors
        if (savedStatus != status) {
            if (U_FAILURE(status)) {
                // Convey any error generated by the formatter
                // as a formatting error
                status = U_ZERO_ERROR;
                errs.setFormattingError(functionName.toString(), status);
                return FormattedPlaceholder(fallback);
            } else {
                // Ignore warnings
                status = savedStatus;
            }
        }
        // Ignore the output if any errors occurred
        if (errs.hasFormattingError()) {
            return FormattedPlaceholder(fallback);
        }
        return result;
    }
    // No formatter with this name -- set error
    if (isSelector(functionName)) {
        errs.setFormattingError(functionName.toString(), status);
    } else {
        errs.setUnknownFunction(functionName, status);
    }
    return FormattedPlaceholder(fallback);
}


// Formats an expression using `globalEnv` for the values of variables
[[nodiscard]] FormattedPlaceholder MessageFormatter::formatExpression(const Environment& globalEnv,
                                                                const Expression& expr,
                                                                MessageContext& context,
                                                                UErrorCode &status) const {
    if (U_FAILURE(status)) {
        return {};
    }

    // Formatting error
    if (expr.isReserved()) {
        context.getErrors().setReservedError(status);
        return FormattedPlaceholder(defaultFallback());
    }

    const Operand& rand = expr.getOperand();
    // Format the operand (formatOperand handles the case of a null operand)
    FormattedPlaceholder randVal = formatOperand(globalEnv, rand, context, status);

    if (!expr.isFunctionCall()) {
        return randVal;
    }
    const Operator& rator = expr.getOperator();
    const FunctionName& functionName = rator.getFunctionName();
    const OptionMap& options = rator.getOptionsInternal();
    // Resolve the options
    FunctionOptions resolvedOptions = resolveOptions(globalEnv, options, context, status);

    // Don't call the function on error values
    if (randVal.isFallback()) {
        return randVal;
    }

    // Call the formatter function
    // The fallback for a nullary function call is the function name
    UnicodeString fallback = rand.isNull() ? functionName.toString() : randVal.fallback;
    FormattedPlaceholder returnVal = evalFormatterCall(functionName,
                                                 std::move(randVal),
                                                 std::move(resolvedOptions),
                                                 context,
                                                 status);
    // If the call was successful, nothing more to do
    if (U_SUCCESS(status)) {
        return returnVal;
    } else if (!(context.getErrors().hasError())) {
        // Set formatting warning if formatting function had no output
        // but didn't set an error or warning
        context.getErrors().setFormattingError(functionName.toString(), status);
    }
    // If we reached this point, the formatter is null --
    // must have been a previous unknown function warning
    return FormattedPlaceholder(fallback);
}

// Formats each text and expression part of a pattern, appending the results to `result`
void MessageFormatter::formatPattern(MessageContext& context, const Environment& globalEnv, const Pattern& pat, UErrorCode &status, UnicodeString& result) const {
    CHECK_ERROR(status);

    for (int32_t i = 0; i < pat.numParts(); i++) {
        const PatternPart& part = pat.getPart(i);
        if (part.isText()) {
            result += part.asText();
        } else {
	      // Format the expression
	      FormattedPlaceholder partVal = formatExpression(globalEnv, part.contents(), context, status);
	      // Force full evaluation, e.g. applying default formatters to
	      // unformatted input (or formatting numbers as strings)
	      result += partVal.formatToString(locale, status);
        }
    }
}

// ------------------------------------------------------
// Selection

// See https://github.com/unicode-org/message-format-wg/blob/main/spec/formatting.md#resolve-selectors
// `res` is a vector of ResolvedSelectors
void MessageFormatter::resolveSelectors(MessageContext& context, const Environment& env, UErrorCode &status, UVector& res) const {
    CHECK_ERROR(status);
    U_ASSERT(dataModel.hasSelectors());

    const Expression* selectors = dataModel.selectors.getAlias();
    // 1. Let res be a new empty list of resolved values that support selection.
    // (Implicit, since `res` is an out-parameter)
    // 2. For each expression exp of the message's selectors
    for (int32_t i = 0; i < dataModel.numSelectors; i++) {
        // 2i. Let rv be the resolved value of exp.
        ResolvedSelector rv = formatSelectorExpression(env, selectors[i], context, status);
        if (rv.hasSelector()) {
            // 2ii. If selection is supported for rv:
            // (True if this code has been reached)
        } else {
            // 2iii. Else:
            // Let nomatch be a resolved value for which selection always fails.
            // Append nomatch as the last element of the list res.
            // Emit a Selection Error.
            // (Note: in this case, rv, being a fallback, serves as `nomatch`)
            #ifdef U_DEBUG
            const DynamicErrors& err = context.getErrors();
            U_ASSERT(err.hasError());
            U_ASSERT(rv.argument().isFallback());
            #endif
        }
        // 2ii(a). Append rv as the last element of the list res.
        // (Also fulfills 2iii)
        LocalPointer<ResolvedSelector> v(create<ResolvedSelector>(std::move(rv), status));
        CHECK_ERROR(status);
        res.adoptElement(v.orphan(), status);
    }
}

// See https://github.com/unicode-org/message-format-wg/blob/main/spec/formatting.md#resolve-preferences
// `keys` and `matches` are vectors of strings
void MessageFormatter::matchSelectorKeys(const UVector& keys,
                                         MessageContext& context,
					 ResolvedSelector&& rv,
					 UVector& keysOut,
					 UErrorCode& status) const {
    CHECK_ERROR(status);

    if (!rv.hasSelector()) {
        // Return an empty list of matches
        return;
    }

    auto selectorImpl = rv.getSelector();
    U_ASSERT(selectorImpl != nullptr);
    UErrorCode savedStatus = status;

    // Convert `keys` to an array
    int32_t keysLen = keys.size();
    UnicodeString* keysArr = new UnicodeString[keysLen];
    if (keysArr == nullptr) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
    for (int32_t i = 0; i < keysLen; i++) {
        const UnicodeString* k = static_cast<UnicodeString*>(keys[i]);
        U_ASSERT(k != nullptr);
        keysArr[i] = *k;
    }
    LocalArray<UnicodeString> adoptedKeys(keysArr);

    // Create an array to hold the output
    UnicodeString* prefsArr = new UnicodeString[keysLen];
    if (prefsArr == nullptr) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
    LocalArray<UnicodeString> adoptedPrefs(prefsArr);
    int32_t prefsLen = 0;

    // Call the selector
    selectorImpl->selectKey(rv.takeArgument(), rv.takeOptions(),
                            adoptedKeys.getAlias(), keysLen, adoptedPrefs.getAlias(), prefsLen,
                            status);

    // Update errors
    if (savedStatus != status) {
        if (U_FAILURE(status)) {
            status = U_ZERO_ERROR;
            context.getErrors().setSelectorError(rv.getSelectorName().toString(), status);
        } else {
            // Ignore warnings
            status = savedStatus;
        }
    }

    CHECK_ERROR(status);

    // Copy the resulting keys (if there was no error)
    keysOut.removeAllElements();
    for (int32_t i = 0; i < prefsLen; i++) {
        UnicodeString* k = message2::create<UnicodeString>(std::move(prefsArr[i]), status);
        if (k == nullptr) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
        keysOut.adoptElement(k, status);
        CHECK_ERROR(status);
    }
}

// See https://github.com/unicode-org/message-format-wg/blob/main/spec/formatting.md#resolve-preferences
// `res` is a vector of FormattedPlaceholders;
// `pref` is a vector of vectors of strings
void MessageFormatter::resolvePreferences(MessageContext& context, UVector& res, UVector& pref, UErrorCode &status) const {
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
            // 2ii(a). Let `key` be the `var` key at position i.
            U_ASSERT(i < selectorKeys.len); // established by semantic check in formatSelectors()
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
        ResolvedSelector rv = std::move(*(static_cast<ResolvedSelector*>(res[i])));
        // 2iv. Let matches be the result of calling the method MatchSelectorKeys(rv, keys)
        LocalPointer<UVector> matches(createUVector(status));
        matchSelectorKeys(*keys, context, std::move(rv), *matches, status);
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
                #ifdef U_DEBUG
                int32_t matchpref = vectorFind(matches, ks);
                U_ASSERT(matchpref >= 0);
                #endif
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
ResolvedSelector MessageFormatter::resolveVariables(const Environment& env, const Operand& rand, MessageContext& context, UErrorCode &status) const {
    if (U_FAILURE(status)) {
        return {};
    }

    if (rand.isNull()) {
        return ResolvedSelector(FormattedPlaceholder());
    }

    if (rand.isLiteral()) {
        return ResolvedSelector(formatLiteral(rand.asLiteral()));
    }

    // Must be variable
    const VariableName& var = rand.asVariable();
    // Resolve the variable
    if (env.has(var)) {
        const Closure& referent = env.lookup(var);
        // Resolve the referent
        return resolveVariables(referent.getEnv(), referent.getExpr(), context, status);
    }
    // Either this is a global var or an unbound var --
    // either way, it can't be bound to a function call.
    // Check globals
    if (context.hasGlobal(var)) {
        return ResolvedSelector(evalArgument(var, context));
    } else {
        // Unresolved variable -- could be a previous warning. Nothing to resolve
        U_ASSERT(context.getErrors().hasUnresolvedVariableError());
        return ResolvedSelector(FormattedPlaceholder(var.declaration()));
    }
}

// Evaluate the expression except for not performing the top-level function call
// (which is expected to be a selector, but may not be, in error cases)
ResolvedSelector MessageFormatter::resolveVariables(const Environment& env,
                                                    const Expression& expr,
                                                    MessageContext& context,
                                                    UErrorCode &status) const {
    if (U_FAILURE(status)) {
        return {};
    }

    // A `reserved` is an error
    if (expr.isReserved()) {
        context.getErrors().setReservedError(status);
        return ResolvedSelector(FormattedPlaceholder(defaultFallback()));
    }

    // Function call -- resolve the operand and options
    if (expr.isFunctionCall()) {
        const Operator& rator = expr.getOperator();
        // Already checked that rator is non-reserved
        const FunctionName& selectorName = rator.getFunctionName();
        if (isSelector(selectorName)) {
            auto selector = getSelector(context, selectorName, status);
            if (U_SUCCESS(status)) {
                FunctionOptions resolvedOptions = resolveOptions(env, rator.getOptionsInternal(), context, status);
                // Operand may be the null argument, but resolveVariables() handles that
                FormattedPlaceholder argument = formatOperand(env, expr.getOperand(), context, status);
                return ResolvedSelector(selectorName, selector, std::move(resolvedOptions), std::move(argument));
            }
        } else if (isFormatter(selectorName)) {
            context.getErrors().setSelectorError(selectorName, status);
        } else {
            context.getErrors().setUnknownFunction(selectorName, status);
        }
        // Non-selector used as selector; an error would have been recorded earlier
        UnicodeString fallback = (!expr.getOperand().isNull()) ?
            formatOperand(env, expr.getOperand(), context, status).fallback
            : selectorName.toString();
        return ResolvedSelector(FormattedPlaceholder(fallback));
    } else {
        // Might be a variable reference, so expand one more level of variable
        return resolveVariables(env, expr.getOperand(), context, status);
    }
}

ResolvedSelector MessageFormatter::formatSelectorExpression(const Environment& globalEnv, const Expression& expr, MessageContext& context, UErrorCode &status) const {
    if (U_FAILURE(status)) {
        return {};
    }

    // Resolve expression to determine if it's a function call
    ResolvedSelector exprResult = resolveVariables(globalEnv, expr, context, status);

    DynamicErrors& err = context.getErrors();

    // If there is a selector, then `resolveVariables()` recorded it in the context
    if (exprResult.hasSelector()) {
        // Check if there was an error
        if (exprResult.argument().isFallback()) {
            // Use a null expression if it's a syntax or data model warning;
            // create a valid (non-fallback) formatted placeholder from the
            // fallback string otherwise
            if (err.hasSyntaxError() || err.hasDataModelError()) {
                return ResolvedSelector(FormattedPlaceholder()); // Null operand
            } else {
                return ResolvedSelector(exprResult.takeArgument());
            }
        }
        return exprResult;
    }

    // No selector was found; error should already have been set
    U_ASSERT(err.hasMissingSelectorAnnotationError() || err.hasUnknownFunctionError() || err.hasSelectorError());
    return ResolvedSelector(FormattedPlaceholder(exprResult.argument().fallback));
}

void MessageFormatter::formatSelectors(MessageContext& context, const Environment& env, UErrorCode &status, UnicodeString& result) const {
    CHECK_ERROR(status);

    // See https://github.com/unicode-org/message-format-wg/blob/main/spec/formatting.md#pattern-selection

    // Resolve Selectors
    // res is a vector of FormattedPlaceholders
    LocalPointer<UVector> res(createUVector(status));
    CHECK_ERROR(status);
    resolveSelectors(context, env, status, *res);

    // Resolve Preferences
    // pref is a vector of vectors of strings
    LocalPointer<UVector> pref(createUVector(status));
    CHECK_ERROR(status);
    resolvePreferences(context, *res, *pref, status);

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
const FunctionRegistry& MessageFormatter::getCustomFunctionRegistry() const {
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
    MessageContext context(arguments, *errors, status);

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
