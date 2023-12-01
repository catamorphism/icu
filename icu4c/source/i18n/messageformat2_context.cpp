// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/messageformat2_function_registry.h"
#include "unicode/messageformat2.h"
#include "messageformat2_context.h"
#include "messageformat2_expression_context.h"
#include "messageformat2_macros.h"
#include "uvector.h" // U_ASSERT

#if U_PF_WINDOWS <= U_PLATFORM && U_PLATFORM <= U_PF_CYGWIN && defined(_MSC_VER)
// Ignore warning 4661 as LocalPointerBase does not use operator== or operator!=
#pragma warning(disable: 4661)
#endif

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
Arguments::Builder& Arguments::Builder::addObject(const UnicodeString& name, const UObject* obj) {
    objectContents[name] = obj;
    return *this;
}

Arguments::Builder& Arguments::Builder::addFormattable(const UnicodeString& name, Formattable&& value) {
    contents[name] = std::move(value);
    return *this;
}

MessageArguments MessageArguments::Builder::build() const {
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
    U_ASSERT(cache.isValid());
    return ((Formatter*) cache->get(f.toString()));
}

void CachedFormatters::setFormatter(const FunctionName& f, Formatter* val, UErrorCode& errorCode) {
    CHECK_ERROR(errorCode);
    U_ASSERT(cache.isValid());
    cache->put(f.toString(), val, errorCode);
}

CachedFormatters::CachedFormatters(UErrorCode& errorCode) {
    CHECK_ERROR(errorCode);
    cache.adoptInstead(new Hashtable(uhash_compareUnicodeString, nullptr, errorCode));
    CHECK_ERROR(errorCode);
    // The cache owns the values
    cache->setValueDeleter(uprv_deleteUObject);
}

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
    NULL_ON_ERROR(status);

    if (isBuiltInSelector(functionName)) {
        return parent.standardFunctionRegistry.getSelector(functionName);
    }
    if (isBuiltInFormatter(functionName)) {
        errors.setSelectorError(functionName);
        return nullptr;
    }
    if (parent.hasCustomFunctionRegistry()) {
        const FunctionRegistry& customFunctionRegistry = parent.getCustomFunctionRegistry();
        const SelectorFactory* customSelector = customFunctionRegistry.getSelector(functionName);
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

FormatterFactory* MessageContext::lookupFormatterFactory(const FunctionName& functionName, UErrorCode& status) {
    NULL_ON_ERROR(status);

    if (isBuiltInFormatter(functionName)) {
        return parent.standardFunctionRegistry.getFormatter(functionName);
    }
    if (isBuiltInSelector(functionName)) {
        errors.setFormattingError(functionName);
        return nullptr;
    }
    if (parent.hasCustomFunctionRegistry()) {
        const FunctionRegistry& customFunctionRegistry = parent.getCustomFunctionRegistry();
        FormatterFactory* customFormatter = customFunctionRegistry.getFormatter(functionName);
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
        FormatterFactory* formatterFactory = lookupFormatterFactory(f, errorCode);
        NULL_ON_ERROR(errorCode);
        // If the formatter factory was null, there must have been
        // an earlier error/warning
        if (formatterFactory == nullptr) {
            U_ASSERT(errors.hasUnknownFunctionError() || errors.hasFormattingError());
            return nullptr;
        }
        NULL_ON_ERROR(errorCode);

        // Create a specific instance of the formatter
        Formatter* formatter = formatterFactory->createFormatter(parent.locale, errorCode);
        NULL_ON_ERROR(errorCode);
        if (formatter == nullptr) {
            errorCode = U_MEMORY_ALLOCATION_ERROR;
            return nullptr;
        }
        parent.cachedFormatters->setFormatter(f, formatter, errorCode);
        return formatter;
    } else {
        return result;
    }
}

MessageArguments::MessageArguments(const std::map<UnicodeString, Formattable>& vals, const std::map<UnicodeString, const UObject*>& objs) : contents(vals), objectContents(objs) {}

// -------------------------------------------------------
// MessageContext accessors and constructors

MessageContext::MessageContext(const MessageFormatter& mf, const MessageArguments& args, const StaticErrors& e) : parent(mf), arguments(args), errors(e) {}

/* static */ MessageContext* MessageContext::create(const MessageFormatter& mf, const MessageArguments& args, const StaticErrors& e, UErrorCode& errorCode) {
    NULL_ON_ERROR(errorCode);

    LocalPointer<MessageContext> result(new MessageContext(mf, args, e));
    NULL_ON_ERROR(errorCode);
    return result.orphan();
}

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

void StaticErrors::addError(StaticError e) {
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

void DynamicErrors::addError(DynamicError e) {
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

Environment* Environment::create(const VariableName& var, Closure* c, Environment* parent, UErrorCode& errorCode) {
    NULL_ON_ERROR(errorCode);
    Environment* result = new NonEmptyEnvironment(var, c, parent);
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

Closure* Closure::create(const Expression& expr, const Environment& env, UErrorCode& errorCode) {
    NULL_ON_ERROR(errorCode);
    Closure* result = new Closure(expr, env);
    if (result == nullptr) {
        errorCode = U_MEMORY_ALLOCATION_ERROR;
        return nullptr;
    }
    return result;
}

const Closure* EmptyEnvironment::lookup(const VariableName& v) const {
    (void) v;
    return nullptr;
}

const Closure* NonEmptyEnvironment::lookup(const VariableName& v) const {
    if (v == var) {
        U_ASSERT(rhs.isValid());
        return rhs.getAlias();
    }
    return parent->lookup(v);
}

Environment::~Environment() {}
NonEmptyEnvironment::~NonEmptyEnvironment() {}
EmptyEnvironment::~EmptyEnvironment() {}

Closure::~Closure() {}

} // namespace message2
U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */
