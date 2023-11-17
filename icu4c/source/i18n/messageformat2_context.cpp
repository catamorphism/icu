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

U_NAMESPACE_BEGIN namespace message2 {

// The context contains all the information needed to process
// an entire message: arguments, formatter cache, and error list

// ------------------------------------------------------
// MessageArguments

using Arguments = MessageArguments;

bool Arguments::hasFormattable(const VariableName& arg) const {
    U_ASSERT(contents != nullptr && objectContents != nullptr);
    return contents->containsKey(arg.identifier());
}

bool Arguments::hasObject(const VariableName& arg) const {
    U_ASSERT(contents != nullptr && objectContents != nullptr);
    return objectContents->containsKey(arg.identifier());
}

const Formattable& Arguments::getFormattable(const VariableName& arg) const {
    U_ASSERT(hasFormattable(arg));
    const Formattable* result = static_cast<const Formattable*>(contents->get(arg.identifier()));
    U_ASSERT(result != nullptr);
    return *result;
}

const UObject* Arguments::getObject(const VariableName& arg) const {
    U_ASSERT(hasObject(arg));
    const UObject* result = static_cast<const UObject*>(objectContents->get(arg.identifier()));
    U_ASSERT(result != nullptr);
    return result;
}

Arguments::Builder::Builder(UErrorCode& errorCode) {
    CHECK_ERROR(errorCode);

    contents = new Hashtable(uhash_compareUnicodeString, nullptr, errorCode);
    objectContents = new Hashtable(uhash_compareUnicodeString, nullptr, errorCode);
    CHECK_ERROR(errorCode);
    if (contents == nullptr || objectContents == nullptr) {
        errorCode = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
    // The `contents` hashtable owns the values, but does not own the keys
    contents->setValueDeleter(uprv_deleteUObject);
    // The `objectContents` hashtable does not own the values
}

Arguments::Builder& Arguments::Builder::add(const UnicodeString& name, const UnicodeString& val, UErrorCode& errorCode) {
    THIS_ON_ERROR(errorCode);

    Formattable* valPtr(ExpressionContext::createFormattable(val, errorCode));
    THIS_ON_ERROR(errorCode);
    return add(name, valPtr, errorCode);
}

Arguments::Builder& Arguments::Builder::addDouble(const UnicodeString& name, double val, UErrorCode& errorCode) {
    THIS_ON_ERROR(errorCode);

    Formattable* valPtr(ExpressionContext::createFormattable(val, errorCode));
    THIS_ON_ERROR(errorCode);
    return add(name, valPtr, errorCode);
}

Arguments::Builder& Arguments::Builder::addInt64(const UnicodeString& name, int64_t val, UErrorCode& errorCode) {
    THIS_ON_ERROR(errorCode);

    Formattable* valPtr(ExpressionContext::createFormattable(val, errorCode));
    THIS_ON_ERROR(errorCode);
    return add(name, valPtr, errorCode);
}

Arguments::Builder& Arguments::Builder::addDate(const UnicodeString& name, UDate val, UErrorCode& errorCode) {
    THIS_ON_ERROR(errorCode);

    Formattable* valPtr(ExpressionContext::createFormattableDate(val, errorCode));
    THIS_ON_ERROR(errorCode);
    return add(name, valPtr, errorCode);
}

Arguments::Builder& Arguments::Builder::addDecimal(const UnicodeString& name, StringPiece val, UErrorCode& errorCode) {
    THIS_ON_ERROR(errorCode);

    Formattable* valPtr(ExpressionContext::createFormattableDecimal(val, errorCode));
    THIS_ON_ERROR(errorCode);
    return add(name, valPtr, errorCode);
}

Arguments::Builder& Arguments::Builder::adoptArray(const UnicodeString& name, const UnicodeString* arr, int32_t count, UErrorCode& errorCode) {
    THIS_ON_ERROR(errorCode);

    Formattable* valPtr(ExpressionContext::createFormattable(arr, count, errorCode));
    THIS_ON_ERROR(errorCode);
    return add(name, valPtr, errorCode);
}

// Does not adopt the object
Arguments::Builder& Arguments::Builder::addObject(const UnicodeString& name, const UObject* obj, UErrorCode& errorCode) {
    THIS_ON_ERROR(errorCode);

    // This const is safe because the values in the objectContents hash table
    // will only be accessed through a (const UObject*) pointer
    objectContents->put(name, const_cast<UObject*>(obj), errorCode);
    return *this;
}

// Adopts its argument
Arguments::Builder& Arguments::Builder::add(const UnicodeString& name, Formattable* value, UErrorCode& errorCode) {
    THIS_ON_ERROR(errorCode);

    U_ASSERT(value != nullptr);

    contents->put(name, value, errorCode);
    return *this;
}

/* static */ MessageArguments::Builder* MessageArguments::builder(UErrorCode& errorCode) {
    NULL_ON_ERROR(errorCode);
    MessageArguments::Builder* result = new MessageArguments::Builder(errorCode);
    if (result == nullptr) {
        errorCode = U_MEMORY_ALLOCATION_ERROR;
    }
    return result;
}

MessageArguments* MessageArguments::Builder::build(UErrorCode& errorCode) const {
    NULL_ON_ERROR(errorCode);
    U_ASSERT(contents != nullptr && objectContents != nullptr);

    LocalPointer<Hashtable> contentsCopied(new Hashtable(uhash_compareUnicodeString, nullptr, errorCode));
    LocalPointer<Hashtable> objectContentsCopied(new Hashtable(uhash_compareUnicodeString, nullptr, errorCode));
    NULL_ON_ERROR(errorCode);
    // The `contents` hashtable owns the values, but does not own the keys
    contentsCopied->setValueDeleter(uprv_deleteUObject);
    // The `objectContents` hashtable does not own the values

    int32_t pos = UHASH_FIRST;
    LocalPointer<Formattable> optionValue;
    // Copy the non-objects
    while (true) {
        const UHashElement* element = contents->nextElement(pos);
        if (element == nullptr) {
            break;
        }
        const Formattable& toCopy = *(static_cast<Formattable*>(element->value.pointer));
        optionValue.adoptInstead(new Formattable(toCopy));
        if (!optionValue.isValid()) {
            errorCode = U_MEMORY_ALLOCATION_ERROR;
            return nullptr;
        }
        UnicodeString* key = static_cast<UnicodeString*>(element->key.pointer);
        contentsCopied->put(*key, optionValue.orphan(), errorCode);
    }
    // Copy the objects
    pos = UHASH_FIRST;
    while (true) {
        const UHashElement* element = objectContents->nextElement(pos);
        if (element == nullptr) {
            break;
        }
        UnicodeString* key = static_cast<UnicodeString*>(element->key.pointer);
        objectContentsCopied->put(*key, element->value.pointer, errorCode);
    }
    MessageArguments* result = new MessageArguments(contentsCopied.orphan(), objectContentsCopied.orphan());
    if (result == nullptr) {
        errorCode = U_MEMORY_ALLOCATION_ERROR;
    }
    return result;
}

MessageArguments::MessageArguments(Hashtable* c, Hashtable* o) : contents(c), objectContents(o) {}

MessageArguments::~MessageArguments() {
    if (contents != nullptr) {
        delete contents;
    }
    if (objectContents != nullptr) {
        delete objectContents;
    }
}
MessageArguments::Builder::~Builder() {
    if (contents != nullptr) {
        delete contents;
    }
    if (objectContents != nullptr) {
        delete objectContents;
    }
}

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
    return parent.standardFunctionRegistry->hasSelector(functionName);
}

bool MessageContext::isBuiltInFormatter(const FunctionName& functionName) const {
    return parent.standardFunctionRegistry->hasFormatter(functionName);
}

// https://github.com/unicode-org/message-format-wg/issues/409
// Unknown function = unknown function error
// Formatter used as selector  = selector error
// Selector used as formatter = formatting error
const SelectorFactory* MessageContext::lookupSelectorFactory(const FunctionName& functionName, UErrorCode& status) const {
    NULL_ON_ERROR(status);

    if (isBuiltInSelector(functionName)) {
        return parent.standardFunctionRegistry->getSelector(functionName);
    }
    if (isBuiltInFormatter(functionName)) {
        errors.setSelectorError(functionName, status);
        return nullptr;
    }
    if (parent.hasCustomFunctionRegistry()) {
        const FunctionRegistry& customFunctionRegistry = parent.getCustomFunctionRegistry();
        const SelectorFactory* customSelector = customFunctionRegistry.getSelector(functionName);
        if (customSelector != nullptr) {
            return customSelector;
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

FormatterFactory* MessageContext::lookupFormatterFactory(const FunctionName& functionName, UErrorCode& status) const {
    NULL_ON_ERROR(status);

    if (isBuiltInFormatter(functionName)) {
        return parent.standardFunctionRegistry->getFormatter(functionName);
    }
    if (isBuiltInSelector(functionName)) {
        errors.setFormattingError(functionName, status);
        return nullptr;
    }
    if (parent.hasCustomFunctionRegistry()) {
        const FunctionRegistry& customFunctionRegistry = parent.getCustomFunctionRegistry();
        FormatterFactory* customFormatter = customFunctionRegistry.getFormatter(functionName);
        if (customFormatter != nullptr) {
            return customFormatter;
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


// -------------------------------------------------------
// MessageContext accessors and constructors

MessageContext::MessageContext(const MessageFormatter& mf, const MessageArguments& args, Errors& e) : parent(mf), arguments(args), errors(e) {}

/* static */ MessageContext* MessageContext::create(const MessageFormatter& mf, const MessageArguments& args, Errors& e, UErrorCode& errorCode) {
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

void Errors::setReservedError(UErrorCode& status) {
    CHECK_ERROR(status);

    Error err(Error::Type::ReservedError);
    addError(err, status);
}

void Errors::setFormattingError(const FunctionName& formatterName, UErrorCode& status) {
    CHECK_ERROR(status);

    Error err(Error::Type::FormattingError, formatterName.toString());
    addError(err, status);
}


void Errors::setMissingSelectorAnnotation(UErrorCode& status) {
    CHECK_ERROR(status);

    Error err(Error::Type::MissingSelectorAnnotation);
    addError(err, status);
}

void Errors::setSelectorError(const FunctionName& selectorName, UErrorCode& status) {
    CHECK_ERROR(status);

    Error err(Error::Type::SelectorError, selectorName.toString());
    addError(err, status);
}

void Errors::setUnknownFunction(const FunctionName& functionName, UErrorCode& status) {
    CHECK_ERROR(status);

    Error err(Error::Type::UnknownFunction, functionName.toString());
    addError(err, status);
}

void Errors::setUnresolvedVariable(const VariableName& v, UErrorCode& status) {
    CHECK_ERROR(status);

    Error err(Error::Type::UnresolvedVariable, v.identifier());
    addError(err, status);
}

Errors* Errors::create(UErrorCode& errorCode) {
    NULL_ON_ERROR(errorCode);
    return new Errors(errorCode);
}

Errors::Errors(UErrorCode& errorCode) {
    CHECK_ERROR(errorCode);
    syntaxAndDataModelErrors.adoptInstead(new UVector(errorCode));
    resolutionAndFormattingErrors.adoptInstead(new UVector(errorCode));
    CHECK_ERROR(errorCode);
    syntaxAndDataModelErrors->setDeleter(uprv_deleteUObject);
    resolutionAndFormattingErrors->setDeleter(uprv_deleteUObject);
    dataModelError = false;
    formattingError = false;
    missingSelectorAnnotationError = false;
    selectorError = false;
    syntaxError = false;
    unknownFunctionError = false;
}

int32_t Errors::count() const {
    return syntaxAndDataModelErrors->size() + resolutionAndFormattingErrors->size();
}

bool Errors::hasError() const {
    return count() > 0;
}

void Errors::clearResolutionAndFormattingErrors() {
    U_ASSERT(resolutionAndFormattingErrors.isValid());
    resolutionAndFormattingErrors->removeAllElements();
    formattingError = false;
    selectorError = false;    
}

void Errors::checkErrors(UErrorCode& status) {
    if (status != U_ZERO_ERROR) {
        return;
    }

    // Just handle the first error
    // TODO: Eventually want to return all errors to caller
    if (count() == 0) {
        return;
    }
    Error* err;
    if (syntaxAndDataModelErrors->size() > 0) {
        err = (Error*) (*syntaxAndDataModelErrors)[0];
    } else {
        U_ASSERT(resolutionAndFormattingErrors->size() > 0);
        err = (Error*) (*resolutionAndFormattingErrors)[0];
    }
    switch (err->type) {
        case Error::Type::DuplicateOptionName: {
            status = U_DUPLICATE_OPTION_NAME_ERROR;
            break;
        }
        case Error::Type::VariantKeyMismatchError: {
            status = U_VARIANT_KEY_MISMATCH_ERROR;
            break;
        }
        case Error::Type::NonexhaustivePattern: {
            status = U_NONEXHAUSTIVE_PATTERN_ERROR;
            break;
        }
        case Error::Type::UnknownFunction: {
            status = U_UNKNOWN_FUNCTION_ERROR;
            break;
        }
        case Error::Type::UnresolvedVariable: {
            status = U_UNRESOLVED_VARIABLE_ERROR;
            break;
        }
        case Error::Type::FormattingError: {
            status = U_FORMATTING_ERROR;
            break;
        }
        case Error::Type::MissingSelectorAnnotation: {
            status = U_MISSING_SELECTOR_ANNOTATION_ERROR;
            break;
        }

        case Error::Type::ReservedError: {
            status = U_UNSUPPORTED_PROPERTY;
            break;
        }
        case Error::Type::SyntaxError: {
            status = U_SYNTAX_ERROR;
            break;
        }
        case Error::Type::SelectorError: {
            status = U_SELECTOR_ERROR;
            break;
        }
    }
}

void Errors::addSyntaxError(UErrorCode& status) {
    CHECK_ERROR(status);
    addError(Error(Error::Type::SyntaxError), status);
}

void Errors::addError(Error e, UErrorCode& status) {
    CHECK_ERROR(status);

    Error* eP = new Error(e);
    if (eP == nullptr) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
    switch (e.type) {
        case Error::Type::SyntaxError: {
            syntaxError = true;
            syntaxAndDataModelErrors->adoptElement(eP, status);
            break;
        }
        case Error::Type::DuplicateOptionName: {
            dataModelError = true;
            syntaxAndDataModelErrors->adoptElement(eP, status);
            break;
        }
        case Error::Type::VariantKeyMismatchError: {
            dataModelError = true;
            syntaxAndDataModelErrors->adoptElement(eP, status);
            break;
        }
        case Error::Type::NonexhaustivePattern: {
            dataModelError = true;
            syntaxAndDataModelErrors->adoptElement(eP, status);
            break;
        }
        case Error::Type::UnresolvedVariable: {
            unresolvedVariableError = true;
            syntaxAndDataModelErrors->adoptElement(eP, status);
            break;
        }
        case Error::Type::FormattingError: {
            formattingError = true;
            resolutionAndFormattingErrors->adoptElement(eP, status);
            break;
        }
        case Error::Type::MissingSelectorAnnotation: {
            missingSelectorAnnotationError = true;
            dataModelError = true;
            syntaxAndDataModelErrors->adoptElement(eP, status);
            break;
        }
        case Error::Type::ReservedError: {
            dataModelError = true;
            syntaxAndDataModelErrors->adoptElement(eP, status);
            break;
        }
        case Error::Type::SelectorError: {
            selectorError = true;
            resolutionAndFormattingErrors->adoptElement(eP, status);
            break;
        }
        case Error::Type::UnknownFunction: {
            unknownFunctionError = true;
            resolutionAndFormattingErrors->adoptElement(eP, status);
            break;
        }
    }
}

Errors::~Errors() {}
Error::~Error() {}

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
