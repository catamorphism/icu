// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/messageformat2.h"
#include "messageformat2_checker.h"
#include "messageformat2_context.h"
#include "messageformat2_macros.h"
#include "uvector.h" // U_ASSERT

#if U_PF_WINDOWS <= U_PLATFORM && U_PLATFORM <= U_PF_CYGWIN && defined(_MSC_VER)
// Ignore warning 4661 as LocalPointerBase does not use operator== or operator!=
#pragma warning(disable: 4661)
#endif

U_NAMESPACE_BEGIN namespace message2 {

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

TypeEnvironment::TypeEnvironment(UErrorCode& errorCode) {
    CHECK_ERROR(errorCode);

    // initialize `contents`
    annotated.adoptInstead(new UVector(errorCode));
    CHECK_ERROR(errorCode);
    if (U_FAILURE(errorCode)) {
        return;
    }
    annotated->setDeleter(uprv_deleteUObject);
}

TypeEnvironment::Type TypeEnvironment::get(const VariableName& var) const {
    for (int32_t i = 0; ((int32_t) i) < annotated->size(); i++) {
        VariableName* lhs = (VariableName*) (*annotated)[i];
        U_ASSERT(lhs != nullptr);
        if (*lhs == var) {
            return Annotated;
        }
    }
    return Unannotated;
}

void TypeEnvironment::extend(const VariableName& var, TypeEnvironment::Type t, UErrorCode& errorCode) {
    CHECK_ERROR(errorCode);

    if (t == Unannotated) {
        // Nothing to do, as variables are considered
        // unannotated by default
        return;
    }

    LocalPointer<VariableName> v(new VariableName(var));
    if (!v.isValid()) {
        errorCode = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
    annotated->adoptElement(v.orphan(), errorCode);
}

TypeEnvironment::~TypeEnvironment() {}

// ---------------------

static bool areDefaultKeys(const KeyList& keys) {
    U_ASSERT(keys.size() > 0);
    for (int32_t i = 0; i < (int32_t) keys.size(); i++) {
        if (!keys[i].isWildcard()) {
            return false;
        }
    }
    return true;
}

void Checker::checkVariants(UErrorCode& error) {
    CHECK_ERROR(error);
    U_ASSERT(dataModel.hasSelectors());

    // Determine the number of selectors
    int32_t numSelectors = dataModel.getSelectors().size();

    // Check that each variant has a key list with size
    // equal to the number of selectors
    const VariantMap& variants = dataModel.getVariants();

    // Check that one variant includes only wildcards
    bool defaultExists = false;

    for (auto iter = variants.begin(); iter != variants.end(); ++iter) {
        const KeyList& keys = iter.first().getKeys();
        if ((int32_t) keys.size() != numSelectors) {
            // Variant key mismatch
            errors.addError(StaticErrorType::VariantKeyMismatchError, error);
            return;
        }
        defaultExists |= areDefaultKeys(keys);
    }
    if (!defaultExists) {
        errors.addError(StaticErrorType::NonexhaustivePattern, error);
        return;
    }
}

void Checker::requireAnnotated(const TypeEnvironment& t, const Expression& selectorExpr, UErrorCode& error) {
    CHECK_ERROR(error);

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
    errors.addError(StaticErrorType::MissingSelectorAnnotation, error);
}

void Checker::checkSelectors(const TypeEnvironment& t, UErrorCode& error) {
    CHECK_ERROR(error);
    U_ASSERT(dataModel.hasSelectors());

    // Check each selector; if it's not annotated, emit a
    // "missing selector annotation" error
    const ExpressionList& selectors = dataModel.getSelectors();
    for (int32_t i = 0; i < (int32_t) selectors.size(); i++) {
        requireAnnotated(t, selectors[i], error);
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

void Checker::checkDeclarations(TypeEnvironment& t, UErrorCode& error) {
    CHECK_ERROR(error);

    // For each declaration, extend the type environment with its type
    // Only a very simple type system is necessary: local variables
    // have the type "annotated" or "unannotated".
    // Free variables (message arguments) are treated as unannotated.
    const Bindings& env = dataModel.getLocalVariables();
    for (int32_t i = 0; i < (int32_t) env.size(); i++) {
        const Binding& b = env[i];
        t.extend(b.getVariable(), typeOf(t, b.getValue()), error);
    }
}

void Checker::check(UErrorCode& error) {
    CHECK_ERROR(error);

    TypeEnvironment typeEnv(error);
    checkDeclarations(typeEnv, error);
    // Pattern message
    if (!dataModel.hasSelectors()) {
        return;
    } else {
      // Selectors message
      checkSelectors(typeEnv, error);
      checkVariants(error);
    }
}

} // namespace message2
U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */
