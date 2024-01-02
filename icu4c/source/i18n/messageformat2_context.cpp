// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/messageformat2_function_registry.h"
#include "messageformat2_context.h"
#include "messageformat2_macros.h"
#include "uvector.h" // U_ASSERT

U_NAMESPACE_BEGIN

namespace message2 {

    // The context contains all the information needed to process
    // an entire message: arguments, formatter cache, and error list

    // ------------------------------------------------------
    // MessageArguments

    using Arguments = MessageArguments;

    int32_t Arguments::findArg(const VariableName& arg) const {
        U_ASSERT(argsLen == 0 || arguments.isValid());
        for (int32_t i = 0; i < argsLen; i++) {
            if (arguments[i].name == arg.identifier()) {
                return i;
            }
        }
        return -1;
    }

    bool Arguments::hasFormattable(const VariableName& arg) const {
        int32_t i = findArg(arg);
        return ((i != -1) && !arguments[i].isObject());
    }

    bool Arguments::hasObject(const VariableName& arg) const {
        int32_t i = findArg(arg);
        return ((i != -1) && arguments[i].isObject());
    }

    const Formattable& Arguments::getFormattable(const VariableName& arg) const {
        int32_t i = findArg(arg);
        U_ASSERT(!arguments[i].isObject());
        return arguments[i].value;
    }

    const UObject* Arguments::getObject(const VariableName& arg) const {
        int32_t i = findArg(arg);
        U_ASSERT(arguments[i].isObject());
        return arguments[i].objectValue;
    }

    MessageArguments::~MessageArguments() {}

    // Message arguments
    // -----------------


    MessageArgument::MessageArgument(const UnicodeString& n, Formattable&& f) : name(n), objectValue(nullptr), value(std::move(f)) {
        U_ASSERT(f.getType() != Formattable::kObject);
    }
    MessageArgument::MessageArgument(const UnicodeString& n, const Formattable& f) : name(n), objectValue(nullptr), value(f) {
        U_ASSERT(f.getType() != Formattable::kObject);
    }
    MessageArgument::MessageArgument(const UnicodeString& n, const UObject* p) : name(n), objectValue(p) {
        U_ASSERT(p != nullptr);
    }
    MessageArguments& MessageArguments::operator=(MessageArguments&& other) noexcept {
        U_ASSERT(other.arguments.isValid() || other.argsLen == 0);
        argsLen = other.argsLen;
        if (argsLen != 0) {
            arguments.adoptInstead(other.arguments.orphan());
        }
        return *this;
    }

    bool MessageContext::hasGlobalAsObject(const VariableName& v) const {
        return arguments.hasObject(v);
    }

    bool MessageContext::hasGlobalAsFormattable(const VariableName& v) const {
        return arguments.hasFormattable(v);
    }

    const UObject* MessageContext::getGlobalAsObject(const VariableName& v) const {
        U_ASSERT(hasGlobalAsObject(v));
        return arguments.getObject(v);
    }

    const Formattable& MessageContext::getGlobalAsFormattable(const VariableName& v) const {
        U_ASSERT(hasGlobalAsFormattable(v));
        return arguments.getFormattable(v);
    }

    // ------------------------------------------------------
    // Formatter cache

    const Formatter* CachedFormatters::getFormatter(const FunctionName& f) {
        return static_cast<const Formatter*>(cache.get(f.toString()));
    }

    void CachedFormatters::adoptFormatter(const FunctionName& f, Formatter* val, UErrorCode& status) {
        cache.put(f.toString(), val, status);
    }

    CachedFormatters::CachedFormatters() {}

    CachedFormatters::~CachedFormatters() {}

    // ---------------------------------------------------
    // Function registry


    bool MessageContext::isBuiltInSelector(const FunctionName& functionName) const {
        return parent.standardFunctionRegistry.hasSelector(functionName);
    }

    bool MessageContext::isBuiltInFormatter(const FunctionName& functionName) const {
        return parent.standardFunctionRegistry.hasFormatter(functionName);
    }

    // https://github.com/unicode-org/message-format-wg/issues/409
    // Unknown function = unknown function error
    // Formatter used as selector  = selector error
    // Selector used as formatter = formatting error
    const SelectorFactory* MessageContext::lookupSelectorFactory(const FunctionName& functionName, UErrorCode& status) {
        if (isBuiltInSelector(functionName)) {
            return parent.standardFunctionRegistry.getSelector(functionName);
        }
        if (isBuiltInFormatter(functionName)) {
            errors.setSelectorError(functionName, status);
            return nullptr;
        }
        if (parent.hasCustomFunctionRegistry()) {
            FunctionRegistry& customFunctionRegistry = parent.getCustomFunctionRegistry();
            const SelectorFactory* selectorFactory = customFunctionRegistry.getSelector(functionName);
            if (selectorFactory != nullptr) {
                return selectorFactory;
            }
            if (customFunctionRegistry.getFormatter(functionName) != nullptr) {
                errors.setSelectorError(functionName, status);
                return nullptr;
            }
        }
        // Either there is no custom function registry and the function
        // isn't built-in, or the function doesn't exist in either the built-in
        // or custom registry.
        // Unknown function error
        errors.setUnknownFunction(functionName, status);
        return nullptr;
    }

    FormatterFactory* MessageContext::lookupFormatterFactory(const FunctionName& functionName, UErrorCode& status) {
        if (isBuiltInFormatter(functionName)) {
            return parent.standardFunctionRegistry.getFormatter(functionName);
        }
        if (isBuiltInSelector(functionName)) {
            errors.setFormattingError(functionName, status);
            return nullptr;
        }
        if (parent.hasCustomFunctionRegistry()) {
            FunctionRegistry& customFunctionRegistry = parent.getCustomFunctionRegistry();
            FormatterFactory* formatterFactory = customFunctionRegistry.getFormatter(functionName);
            if (formatterFactory != nullptr) {
                return formatterFactory;
            }
            if (customFunctionRegistry.getSelector(functionName) != nullptr) {
                errors.setFormattingError(functionName, status);
                return nullptr;
            }
        }
        // Either there is no custom function registry and the function
        // isn't built-in, or the function doesn't exist in either the built-in
        // or custom registry.
        // Unknown function error
        errors.setUnknownFunction(functionName, status);
        return nullptr;
    }

    bool MessageContext::isCustomFormatter(const FunctionName& fn) const {
        return parent.hasCustomFunctionRegistry() && parent.getCustomFunctionRegistry().getFormatter(fn) != nullptr;
    }


    bool MessageContext::isCustomSelector(const FunctionName& fn) const {
        return parent.hasCustomFunctionRegistry() && parent.getCustomFunctionRegistry().getSelector(fn) != nullptr;
    }

    const Formatter* MessageContext::maybeCachedFormatter(const FunctionName& f, UErrorCode& errorCode) {
        NULL_ON_ERROR(errorCode);
        U_ASSERT(parent.cachedFormatters != nullptr);

        const Formatter* result = parent.cachedFormatters->getFormatter(f);
        if (result == nullptr) {
            // Create the formatter

            // First, look up the formatter factory for this function
            FormatterFactory* formatterFactory = lookupFormatterFactory(f, errorCode);
            NULL_ON_ERROR(errorCode);

            // If the formatter factory was null, there must have been
            // an earlier error/warning
            if (formatterFactory == nullptr) {
                U_ASSERT(errors.hasUnknownFunctionError() || errors.hasFormattingError());
                return nullptr;
            }

            // Create a specific instance of the formatter
            Formatter* formatter = formatterFactory->createFormatter(parent.locale, errorCode);
            NULL_ON_ERROR(errorCode);
            if (formatter == nullptr) {
                errorCode = U_MEMORY_ALLOCATION_ERROR;
                return nullptr;
            }
            parent.cachedFormatters->adoptFormatter(f, formatter, errorCode);
            return formatter;
        } else {
            return result;
        }
    }

    // -------------------------------------------------------
    // MessageContext accessors and constructors

    MessageContext::MessageContext(MessageFormatter& mf,
                                   const MessageArguments& args,
                                   const StaticErrors& e,
                                   UErrorCode& status) : parent(mf), arguments(args), errors(e, status) {}

    // Errors
    // -----------

    void MessageContext::checkErrors(UErrorCode& status) const {
        CHECK_ERROR(status);
        errors.checkErrors(status);
    }

    void DynamicErrors::setReservedError(UErrorCode& status) {
        addError(DynamicError(DynamicErrorType::ReservedError), status);
    }

    void DynamicErrors::setFormattingError(const FunctionName& formatterName, UErrorCode& status) {
        addError(DynamicError(DynamicErrorType::FormattingError, formatterName.toString()), status);
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

    MessageContext::~MessageContext() {}


    // ---------------- Environments and closures

    Environment* Environment::create(const VariableName& var, Closure&& c, Environment* parent, UErrorCode& errorCode) {
        NULL_ON_ERROR(errorCode);
        Environment* result = new NonEmptyEnvironment(var, std::move(c), parent);
        if (result == nullptr) {
            errorCode = U_MEMORY_ALLOCATION_ERROR;
            return nullptr;
        }
        return result;
    }

    Environment* Environment::create(UErrorCode& errorCode) {
        NULL_ON_ERROR(errorCode);
        Environment* result = new EmptyEnvironment();
        if (result == nullptr) {
            errorCode = U_MEMORY_ALLOCATION_ERROR;
            return nullptr;
        }
        return result;
    }

    const Closure& EmptyEnvironment::lookup(const VariableName& v) const {
        (void) v;
        U_ASSERT(false);
        UPRV_UNREACHABLE_EXIT;
    }

    const Closure& NonEmptyEnvironment::lookup(const VariableName& v) const {
        if (v == var) {
            return rhs;
        }
        return parent->lookup(v);
    }

    bool EmptyEnvironment::has(const VariableName& v) const {
        (void) v;
        return false;
    }

    bool NonEmptyEnvironment::has(const VariableName& v) const {
        if (v == var) {
            return true;
        }
        return parent->has(v);
    }

    Environment::~Environment() {}
    NonEmptyEnvironment::~NonEmptyEnvironment() {}
    EmptyEnvironment::~EmptyEnvironment() {}

    Closure::~Closure() {}

} // namespace message2

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */
