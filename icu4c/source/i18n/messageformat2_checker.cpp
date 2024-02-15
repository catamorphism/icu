// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "messageformat2_checker.h"
#include "messageformat2_macros.h"
#include "uvector.h" // U_ASSERT

U_NAMESPACE_BEGIN

namespace message2 {

/*
Checks data model errors
(see https://github.com/unicode-org/message-format-wg/blob/main/spec/formatting.md#error-handling )

The following are checked here:
Variant Key Mismatch
Missing Fallback Variant (called NonexhaustivePattern here)
Missing Selector Annotation

(Duplicate option names are checked by the parser)
*/

// Type environments
// -----------------

TypeEnvironment::TypeEnvironment(UErrorCode& status) {
    CHECK_ERROR(status);

    UVector* result = new UVector(status);
    CHECK_ERROR(status);
    annotated.adoptInstead(result);
    // `annotated` does not adopt its elements
}

TypeEnvironment::Type TypeEnvironment::get(const VariableName& var) const {
    U_ASSERT(annotated.isValid());
    for (int32_t i = 0; i < annotated->size(); i++) {
        const VariableName& lhs = *(static_cast<VariableName*>(annotated->elementAt(i)));
        if (lhs == var) {
            return Annotated;
        }
    }
    return Unannotated;
}

void TypeEnvironment::extend(const VariableName& var, TypeEnvironment::Type t, UErrorCode& status) {
    if (t == Unannotated) {
        // Nothing to do, as variables are considered
        // unannotated by default
        return;
    }

    U_ASSERT(annotated.isValid());
    // This is safe because elements of `annotated` are never written
    // and the lifetime of `var` is guaranteed to include the lifetime of
    // `annotated`
    annotated->addElement(const_cast<void*>(static_cast<const void*>(&var)), status);
}

TypeEnvironment::~TypeEnvironment() {}

// ---------------------

static bool areDefaultKeys(const Key* keys, int32_t len) {
    U_ASSERT(len > 0);
    for (int32_t i = 0; i < len; i++) {
        if (!keys[i].isWildcard()) {
            return false;
        }
    }
    return true;
}

void Checker::checkVariants(UErrorCode& status) {
    CHECK_ERROR(status);

    U_ASSERT(dataModel.hasSelectors());

    // Check that each variant has a key list with size
    // equal to the number of selectors
    const Variant* variants = dataModel.getVariantsInternal();

    // Check that one variant includes only wildcards
    bool defaultExists = false;

    for (int32_t i = 0; i < dataModel.numVariants(); i++) {
        const SelectorKeys& k = variants[i].getKeys();
        const Key* keys = k.getKeysInternal();
        int32_t len = k.len;
        if (len != dataModel.numSelectors()) {
            // Variant key mismatch
            errors.addError(StaticErrorType::VariantKeyMismatchError, status);
            return;
        }
        defaultExists |= areDefaultKeys(keys, len);
    }
    if (!defaultExists) {
        errors.addError(StaticErrorType::NonexhaustivePattern, status);
        return;
    }
}

void Checker::requireAnnotated(const TypeEnvironment& t, const Expression& selectorExpr, UErrorCode& status) {
    CHECK_ERROR(status);

    if (selectorExpr.isFunctionCall()) {
        return; // No error
    }
    if (!selectorExpr.isReserved()) {
        const Operand& rand = selectorExpr.getOperand();
        if (rand.isVariable()) {
            if (t.get(rand.asVariable()) == TypeEnvironment::Type::Annotated) {
                return; // No error
            }
        }
    }
    // If this code is reached, an error was detected
    errors.addError(StaticErrorType::MissingSelectorAnnotation, status);
}

void Checker::checkSelectors(const TypeEnvironment& t, UErrorCode& status) {
    U_ASSERT(dataModel.hasSelectors());

    // Check each selector; if it's not annotated, emit a
    // "missing selector annotation" error
    const Expression* selectors = dataModel.getSelectorsInternal();
    for (int32_t i = 0; i < dataModel.numSelectors(); i++) {
        requireAnnotated(t, selectors[i], status);
    }
}

TypeEnvironment::Type typeOf(TypeEnvironment& t, const Expression& expr) {
    if (expr.isFunctionCall()) {
        return TypeEnvironment::Type::Annotated;
    }
    if (expr.isReserved()) {
        return TypeEnvironment::Type::Unannotated;
    }
    const Operand& rand = expr.getOperand();
    U_ASSERT(!rand.isNull());
    if (rand.isLiteral()) {
        return TypeEnvironment::Type::Unannotated;
    }
    U_ASSERT(rand.isVariable());
    return t.get(rand.asVariable());
}

void Checker::checkDeclarations(TypeEnvironment& t, UErrorCode& status) {
    CHECK_ERROR(status);

    // For each declaration, extend the type environment with its type
    // Only a very simple type system is necessary: local variables
    // have the type "annotated" or "unannotated".
    // Free variables (message arguments) are treated as unannotated.
    const Binding* env = dataModel.getLocalVariablesInternal();
    for (int32_t i = 0; i < dataModel.bindingsLen; i++) {
        const Binding& b = env[i];
        t.extend(b.getVariable(), typeOf(t, b.getValue()), status);
    }
}

void Checker::check(UErrorCode& status) {
    CHECK_ERROR(status);

    TypeEnvironment typeEnv(status);
    checkDeclarations(typeEnv, status);
    // Pattern message
    if (!dataModel.hasSelectors()) {
        return;
    } else {
      // Selectors message
      checkSelectors(typeEnv, status);
      checkVariants(status);
    }
}

} // namespace message2
U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */
