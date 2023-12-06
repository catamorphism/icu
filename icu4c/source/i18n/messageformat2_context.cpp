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

    bool Arguments::hasFormattable(const VariableName& arg) const {
        return contents.count(arg.identifier()) > 0;
    }

    bool Arguments::hasObject(const VariableName& arg) const {
        return objectContents.count(arg.identifier()) > 0;
    }

    const Formattable& Arguments::getFormattable(const VariableName& arg) const {
        U_ASSERT(hasFormattable(arg));
        return contents.at(arg.identifier());
    }

    const UObject* Arguments::getObject(const VariableName& arg) const {
        U_ASSERT(hasObject(arg));
        return objectContents.at(arg.identifier());
    }

    Arguments::Builder::Builder() {}

    Arguments::Builder& Arguments::Builder::add(const UnicodeString& name, const UnicodeString& val) {
        return addFormattable(name, Formattable(val));
    }

    Arguments::Builder& Arguments::Builder::addDouble(const UnicodeString& name, double val) {
        return addFormattable(name, Formattable(val));
    }

    Arguments::Builder& Arguments::Builder::addInt64(const UnicodeString& name, int64_t val) {
        return addFormattable(name, Formattable(val));
    }

    Arguments::Builder& Arguments::Builder::addDate(const UnicodeString& name, UDate val) {
        return addFormattable(name, Formattable(val, Formattable::kIsDate));
    }

    Arguments::Builder& Arguments::Builder::addDecimal(const UnicodeString& name, StringPiece val, UErrorCode& errorCode) {
        Formattable result(val, errorCode);
        THIS_ON_ERROR(errorCode);
        return addFormattable(name, std::move(result));
    }

    // members of `arr` should be strings
    Arguments::Builder& Arguments::Builder::adoptArray(const UnicodeString& name, const Formattable* arr, int32_t count) {
        return addFormattable(name, Formattable(arr, count));
    }

    // Does not adopt the object
    Arguments::Builder& Arguments::Builder::addObject(const UnicodeString& name, const UObject* obj) noexcept {
        objectContents[name] = obj;
        return *this;
    }

    Arguments::Builder& Arguments::Builder::addFormattable(const UnicodeString& name, Formattable&& value) noexcept {
        contents[name] = std::move(value);
        return *this;
    }

    MessageArguments MessageArguments::Builder::build() const noexcept {
        return MessageArguments(contents, objectContents);
    }

    MessageArguments::~MessageArguments() {}

    MessageArguments::Builder::~Builder() {}

    // Message arguments
    // -----------------

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
        if (cache.count(f) <= 0) {
            return nullptr;
        }
        return cache[f].get();
    }

    void CachedFormatters::setFormatter(const FunctionName& f, Formatter* val) noexcept {
        cache[f] = std::unique_ptr<Formatter>(val);
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
    const std::shared_ptr<SelectorFactory> MessageContext::lookupSelectorFactory(const FunctionName& functionName) {
        if (isBuiltInSelector(functionName)) {
            return parent.standardFunctionRegistry.getSelector(functionName);
        }
        if (isBuiltInFormatter(functionName)) {
            errors.setSelectorError(functionName);
            return nullptr;
        }
        if (parent.hasCustomFunctionRegistry()) {
            FunctionRegistry& customFunctionRegistry = parent.getCustomFunctionRegistry();
            const std::shared_ptr<SelectorFactory> customSelector = customFunctionRegistry.getSelector(functionName);
            if (customSelector != nullptr) {
                return customSelector;
            }
            if (customFunctionRegistry.getFormatter(functionName) != nullptr) {
                errors.setSelectorError(functionName);
                return nullptr;
            }
        }
        // Either there is no custom function registry and the function
        // isn't built-in, or the function doesn't exist in either the built-in
        // or custom registry.
        // Unknown function error
        errors.setUnknownFunction(functionName);
        return nullptr;
    }

    std::shared_ptr<FormatterFactory> MessageContext::lookupFormatterFactory(const FunctionName& functionName) {
        if (isBuiltInFormatter(functionName)) {
            return parent.standardFunctionRegistry.getFormatter(functionName);
        }
        if (isBuiltInSelector(functionName)) {
            errors.setFormattingError(functionName);
            return nullptr;
        }
        if (parent.hasCustomFunctionRegistry()) {
            FunctionRegistry& customFunctionRegistry = parent.getCustomFunctionRegistry();
            std::shared_ptr<FormatterFactory> customFormatter = customFunctionRegistry.getFormatter(functionName);
            if (customFormatter != nullptr) {
                return customFormatter;
            }
            if (customFunctionRegistry.getSelector(functionName) != nullptr) {
                errors.setFormattingError(functionName);
                return nullptr;
            }
        }
        // Either there is no custom function registry and the function
        // isn't built-in, or the function doesn't exist in either the built-in
        // or custom registry.
        // Unknown function error
        errors.setUnknownFunction(functionName);
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
            std::shared_ptr<FormatterFactory> formatterFactory = lookupFormatterFactory(f);
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
            parent.cachedFormatters->setFormatter(f, formatter);
            return formatter;
        } else {
            return result;
        }
    }

    MessageArguments::MessageArguments(const std::map<UnicodeString, Formattable>& vals, const std::map<UnicodeString, const UObject*>& objs) noexcept : contents(vals), objectContents(objs) {}

    // -------------------------------------------------------
    // MessageContext accessors and constructors

    MessageContext::MessageContext(MessageFormatter& mf, const MessageArguments& args, const StaticErrors& e) : parent(mf), arguments(args), errors(e) {}

    // Errors
    // -----------

    void MessageContext::checkErrors(UErrorCode& status) const {
        CHECK_ERROR(status);
        errors.checkErrors(status);
    }

    void DynamicErrors::setReservedError() {
        addError(DynamicError(DynamicErrorType::ReservedError));
    }

    void DynamicErrors::setFormattingError(const FunctionName& formatterName) {
        addError(DynamicError(DynamicErrorType::FormattingError, formatterName.toString()));
    }

    void StaticErrors::setDuplicateOptionName() {
        addError(StaticError(StaticErrorType::DuplicateOptionName));
    }

    void StaticErrors::setMissingSelectorAnnotation() {
        addError(StaticError(StaticErrorType::MissingSelectorAnnotation));
    }

    void DynamicErrors::setSelectorError(const FunctionName& selectorName) {
        addError(DynamicError(DynamicErrorType::SelectorError, selectorName.toString()));
    }

    void DynamicErrors::setUnknownFunction(const FunctionName& functionName) {
        addError(DynamicError(DynamicErrorType::UnknownFunction, functionName.toString()));
    }

    void DynamicErrors::setUnresolvedVariable(const VariableName& v) {
        addError(DynamicError(DynamicErrorType::UnresolvedVariable, v.identifier()));
    }

    DynamicErrors::DynamicErrors(const StaticErrors& e) : staticErrors(e) {}

    StaticErrors::StaticErrors() {}

    int32_t DynamicErrors::count() const {
        return resolutionAndFormattingErrors.size() + staticErrors.syntaxAndDataModelErrors.size();
    }

    bool DynamicErrors::hasError() const {
        return count() > 0;
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
        if (staticErrors.syntaxAndDataModelErrors.size() > 0) {
            switch (staticErrors.syntaxAndDataModelErrors[0].type) {
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
            U_ASSERT(resolutionAndFormattingErrors.size() > 0);
            switch (resolutionAndFormattingErrors[0].type) {
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

    void StaticErrors::addSyntaxError() {
        addError(StaticError(StaticErrorType::SyntaxError));
    }

    void StaticErrors::addError(StaticError e) noexcept {
        switch (e.type) {
        case StaticErrorType::SyntaxError: {
            syntaxError = true;
            syntaxAndDataModelErrors.push_back(e);
            break;
        }
        case StaticErrorType::DuplicateOptionName: {
            dataModelError = true;
            syntaxAndDataModelErrors.push_back(e);
            break;
        }
        case StaticErrorType::VariantKeyMismatchError: {
            dataModelError = true;
            syntaxAndDataModelErrors.push_back(e);
            break;
        }
        case StaticErrorType::NonexhaustivePattern: {
            dataModelError = true;
            syntaxAndDataModelErrors.push_back(e);
            break;
        }
        case StaticErrorType::MissingSelectorAnnotation: {
            missingSelectorAnnotationError = true;
            dataModelError = true;
            syntaxAndDataModelErrors.push_back(e);
            break;
        }
        }
    }

    void DynamicErrors::addError(DynamicError e) noexcept {
        switch (e.type) {
        case DynamicErrorType::UnresolvedVariable: {
            unresolvedVariableError = true;
            resolutionAndFormattingErrors.push_back(e);
            break;
        }
        case DynamicErrorType::FormattingError: {
            formattingError = true;
            resolutionAndFormattingErrors.push_back(e);
            break;
        }
        case DynamicErrorType::ReservedError: {
            resolutionAndFormattingErrors.push_back(e);
            break;
        }
        case DynamicErrorType::SelectorError: {
            selectorError = true;
            resolutionAndFormattingErrors.push_back(e);
            break;
        }
        case DynamicErrorType::UnknownFunction: {
            unknownFunctionError = true;
            resolutionAndFormattingErrors.push_back(e);
            break;
        }
        }
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
