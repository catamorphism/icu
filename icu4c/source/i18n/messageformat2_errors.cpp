// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "messageformat2_allocation.h"
#include "messageformat2_errors.h"
#include "messageformat2_macros.h"
#include "uvector.h" // U_ASSERT

U_NAMESPACE_BEGIN

namespace message2 {

    // Errors
    // -----------

    void DynamicErrors::setReservedError(UErrorCode& status) {
        addError(DynamicError(DynamicErrorType::ReservedError), status);
    }

    void DynamicErrors::setFormattingError(const FunctionName& formatterName, UErrorCode& status) {
        addError(DynamicError(DynamicErrorType::FormattingError, formatterName.toString()), status);
    }

    void DynamicErrors::setFormattingError(UErrorCode& status) {
        addError(DynamicError(DynamicErrorType::FormattingError, UnicodeString("unknown formatter")), status);
    }

    void StaticErrors::setDuplicateOptionName(UErrorCode& status) {
        addError(StaticError(StaticErrorType::DuplicateOptionName), status);
    }

    void StaticErrors::setMissingSelectorAnnotation(UErrorCode& status) {
        addError(StaticError(StaticErrorType::MissingSelectorAnnotation), status);
    }

    void DynamicErrors::setSelectorError(const FunctionName& selectorName, UErrorCode& status) {
        addError(DynamicError(DynamicErrorType::SelectorError, selectorName.toString()), status);
    }

    void DynamicErrors::setUnknownFunction(const FunctionName& functionName, UErrorCode& status) {
        addError(DynamicError(DynamicErrorType::UnknownFunction, functionName.toString()), status);
    }

    void DynamicErrors::setUnresolvedVariable(const VariableName& v, UErrorCode& status) {
        addError(DynamicError(DynamicErrorType::UnresolvedVariable, v.identifier()), status);
    }

    DynamicErrors::DynamicErrors(const StaticErrors& e, UErrorCode& status) : staticErrors(e) {
        resolutionAndFormattingErrors.adoptInstead(createUVector(status));
    }

    StaticErrors::StaticErrors(UErrorCode& status) {
        syntaxAndDataModelErrors.adoptInstead(createUVector(status));
    }

    StaticErrors::StaticErrors(StaticErrors&& other) noexcept {
        U_ASSERT(other.syntaxAndDataModelErrors.isValid());
        syntaxAndDataModelErrors.adoptInstead(other.syntaxAndDataModelErrors.orphan());
        dataModelError = other.dataModelError;
        missingSelectorAnnotationError = other.missingSelectorAnnotationError;
        syntaxError = other.syntaxError;
    }

    StaticErrors::StaticErrors(const StaticErrors& other, UErrorCode& errorCode) {
        CHECK_ERROR(errorCode);

        U_ASSERT(other.syntaxAndDataModelErrors.isValid());
        syntaxAndDataModelErrors.adoptInstead(createUVector(errorCode));
        CHECK_ERROR(errorCode);
        for (int32_t i = 0; i < other.syntaxAndDataModelErrors->size(); i++) {
            StaticError* e = static_cast<StaticError*>(other.syntaxAndDataModelErrors->elementAt(i));
            U_ASSERT(e != nullptr);
            StaticError* copy = new StaticError(*e);
            if (copy == nullptr) {
                errorCode = U_MEMORY_ALLOCATION_ERROR;
                return;
            }
            syntaxAndDataModelErrors->adoptElement(copy, errorCode);
        }
        dataModelError = other.dataModelError;
        missingSelectorAnnotationError = other.missingSelectorAnnotationError;
        syntaxError = other.syntaxError;
    }

    int32_t DynamicErrors::count() const {
        U_ASSERT(resolutionAndFormattingErrors.isValid() && staticErrors.syntaxAndDataModelErrors.isValid());
        return resolutionAndFormattingErrors->size() + staticErrors.syntaxAndDataModelErrors->size();
    }

    bool DynamicErrors::hasError() const {
        return count() > 0;
    }

    bool DynamicErrors::hasStaticError() const {
        U_ASSERT(staticErrors.syntaxAndDataModelErrors.isValid());
        return staticErrors.syntaxAndDataModelErrors->size() > 0;
    }

    const DynamicError& DynamicErrors::first() const {
        U_ASSERT(resolutionAndFormattingErrors->size() > 0);
        return *static_cast<DynamicError*>(resolutionAndFormattingErrors->elementAt(0));
    }

    void DynamicErrors::checkErrors(UErrorCode& status) const {
        if (status != U_ZERO_ERROR) {
            return;
        }

        // Just handle the first error
        // TODO: Eventually want to return all errors to caller
        if (count() == 0) {
            return;
        }
        if (staticErrors.syntaxAndDataModelErrors->size() > 0) {
            switch (staticErrors.first().type) {
            case StaticErrorType::DuplicateOptionName: {
                status = U_DUPLICATE_OPTION_NAME_ERROR;
                break;
            }
            case StaticErrorType::VariantKeyMismatchError: {
                status = U_VARIANT_KEY_MISMATCH_ERROR;
                break;
            }
            case StaticErrorType::NonexhaustivePattern: {
                status = U_NONEXHAUSTIVE_PATTERN_ERROR;
                break;
            }
            case StaticErrorType::MissingSelectorAnnotation: {
                status = U_MISSING_SELECTOR_ANNOTATION_ERROR;
                break;
            }
            case StaticErrorType::SyntaxError: {
                status = U_SYNTAX_ERROR;
                break;
            }
            }
        } else {
            U_ASSERT(resolutionAndFormattingErrors->size() > 0);
            switch (first().type) {
            case DynamicErrorType::UnknownFunction: {
                status = U_UNKNOWN_FUNCTION_ERROR;
                break;
            }
            case DynamicErrorType::UnresolvedVariable: {
                status = U_UNRESOLVED_VARIABLE_ERROR;
                break;
            }
            case DynamicErrorType::FormattingError: {
                status = U_FORMATTING_ERROR;
                break;
            }
            case DynamicErrorType::ReservedError: {
                status = U_UNSUPPORTED_PROPERTY;
                break;
            }
            case DynamicErrorType::SelectorError: {
                status = U_SELECTOR_ERROR;
                break;
            }
            }
        }
    }

    void StaticErrors::addSyntaxError(UErrorCode& status) {
        addError(StaticError(StaticErrorType::SyntaxError), status);
    }

    void StaticErrors::addError(StaticError&& e, UErrorCode& status) {
        CHECK_ERROR(status);

        void* errorP = static_cast<void*>(create<StaticError>(std::move(e), status));
        U_ASSERT(syntaxAndDataModelErrors.isValid());

        switch (e.type) {
        case StaticErrorType::SyntaxError: {
            syntaxError = true;
            syntaxAndDataModelErrors->adoptElement(errorP, status);
            break;
        }
        case StaticErrorType::DuplicateOptionName: {
            dataModelError = true;
            syntaxAndDataModelErrors->adoptElement(errorP, status);
            break;
        }
        case StaticErrorType::VariantKeyMismatchError: {
            dataModelError = true;
            syntaxAndDataModelErrors->adoptElement(errorP, status);
            break;
        }
        case StaticErrorType::NonexhaustivePattern: {
            dataModelError = true;
            syntaxAndDataModelErrors->adoptElement(errorP, status);
            break;
        }
        case StaticErrorType::MissingSelectorAnnotation: {
            missingSelectorAnnotationError = true;
            dataModelError = true;
            syntaxAndDataModelErrors->adoptElement(errorP, status);
            break;
        }
        }
    }

    void DynamicErrors::addError(DynamicError&& e, UErrorCode& status) {
        CHECK_ERROR(status);

        void* errorP = static_cast<void*>(create<DynamicError>(std::move(e), status));
        U_ASSERT(resolutionAndFormattingErrors.isValid());

        switch (e.type) {
        case DynamicErrorType::UnresolvedVariable: {
            unresolvedVariableError = true;
            resolutionAndFormattingErrors->adoptElement(errorP, status);
            break;
        }
        case DynamicErrorType::FormattingError: {
            formattingError = true;
            resolutionAndFormattingErrors->adoptElement(errorP, status);
            break;
        }
        case DynamicErrorType::ReservedError: {
            resolutionAndFormattingErrors->adoptElement(errorP, status);
            break;
        }
        case DynamicErrorType::SelectorError: {
            selectorError = true;
            resolutionAndFormattingErrors->adoptElement(errorP, status);
            break;
        }
        case DynamicErrorType::UnknownFunction: {
            unknownFunctionError = true;
            resolutionAndFormattingErrors->adoptElement(errorP, status);
            break;
        }
        }
    }

    const StaticError& StaticErrors::first() const {
        U_ASSERT(syntaxAndDataModelErrors.isValid() && syntaxAndDataModelErrors->size() > 0);
        return *static_cast<StaticError*>(syntaxAndDataModelErrors->elementAt(0));
    }

    StaticErrors::~StaticErrors() {}
    DynamicErrors::~DynamicErrors() {}

    template<typename ErrorType>
    Error<ErrorType>::~Error() {}

    template<>
    Error<StaticErrorType>::~Error() {}
    template<>
    Error<DynamicErrorType>::~Error() {}

} // namespace message2

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */
