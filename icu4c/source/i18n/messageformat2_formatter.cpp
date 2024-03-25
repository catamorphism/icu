// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/messageformat2.h"
#include "messageformat2_allocation.h"
#include "messageformat2_cached_formatters.h"
#include "messageformat2_checker.h"
#include "messageformat2_errors.h"
#include "messageformat2_evaluation.h"
#include "messageformat2_function_registry_internal.h"
#include "messageformat2_macros.h"
#include "messageformat2_parser.h"
#include "messageformat2_serializer.h"
#include "uvector.h" // U_ASSERT

U_NAMESPACE_BEGIN

namespace message2 {

    // MessageFormatter::Builder

        // -------------------------------------
    // Creates a MessageFormat instance based on the pattern.

    MessageFormatter::Builder& MessageFormatter::Builder::setPattern(const UnicodeString& pat, UParseError& parseError, UErrorCode& errorCode) {
        normalizedInput.remove();
        // Parse the pattern
        MessageFormatDataModel::Builder tree(errorCode);
        Parser(pat, tree, *errors, normalizedInput).parse(parseError, errorCode);

        // Build the data model based on what was parsed
        dataModel = tree.build(errorCode);
        hasDataModel = true;
        hasPattern = true;
        pattern = pat;

        return *this;
    }

    // Precondition: `reg` is non-null
    // Does not adopt `reg`
    MessageFormatter::Builder& MessageFormatter::Builder::setFunctionRegistry(const FunctionRegistry& reg) {
        customFunctionRegistry = &reg;
        return *this;
    }

    MessageFormatter::Builder& MessageFormatter::Builder::setLocale(const Locale& loc) {
        locale = loc;
        return *this;
    }

    MessageFormatter::Builder& MessageFormatter::Builder::setDataModel(MessageFormatDataModel&& newDataModel) {
        normalizedInput.remove();
        delete errors;
        errors = nullptr;
        hasPattern = false;
        hasDataModel = true;
        dataModel = std::move(newDataModel);

        return *this;
    }

    /*
      This build() method is non-destructive, which entails the risk that
      its borrowed FunctionRegistry and (if the setDataModel() method was called)
      MessageFormatDataModel pointers could become invalidated.
    */
    MessageFormatter MessageFormatter::Builder::build(UErrorCode& errorCode) const {
        return MessageFormatter(*this, errorCode);
    }

    MessageFormatter::Builder::Builder(UErrorCode& errorCode) : locale(Locale::getDefault()), customFunctionRegistry(nullptr) {
        // Initialize errors
        errors = new StaticErrors(errorCode);
        CHECK_ERROR(errorCode);
        if (errors == nullptr) {
            errorCode = U_MEMORY_ALLOCATION_ERROR;
        }
    }

    MessageFormatter::Builder::~Builder() {
        if (errors != nullptr) {
            delete errors;
        }
    }

    // MessageFormatter

    MessageFormatter::MessageFormatter(const MessageFormatter::Builder& builder, UErrorCode &success) : locale(builder.locale), customFunctionRegistry(builder.customFunctionRegistry) {
        // Set up the standard function registry
        FunctionRegistry::Builder standardFunctionsBuilder(success);

        // FunctionRegistry does not own its formatter elements, so we keep a separate vector to ensure
        // the elements are deleted
        standardFormatters = createUVector(success);
        CHECK_ERROR(success);
        FormatterFactory* dateTime = new StandardFunctions::DateTimeFactory();
        FormatterFactory* number = new StandardFunctions::NumberFactory();
        FormatterFactory* identity = new StandardFunctions::IdentityFactory();
        standardFormatters->adoptElement(dateTime, success);
        standardFormatters->adoptElement(number, success);
        standardFormatters->adoptElement(identity, success);
        standardFunctionsBuilder.setFormatter(FunctionName(UnicodeString("datetime")), dateTime, success)
            .setFormatter(FunctionName(UnicodeString("number")), number, success)
            .setFormatter(FunctionName(UnicodeString("identity")), identity, success)
            .setSelector(FunctionName(UnicodeString("plural")), new StandardFunctions::PluralFactory(UPLURAL_TYPE_CARDINAL), success)
            .setSelector(FunctionName(UnicodeString("selectordinal")), new StandardFunctions::PluralFactory(UPLURAL_TYPE_ORDINAL), success)
            .setSelector(FunctionName(UnicodeString("select")), new StandardFunctions::TextFactory(), success)
            .setSelector(FunctionName(UnicodeString("gender")), new StandardFunctions::TextFactory(), success);
        CHECK_ERROR(success);
        standardFunctionRegistry = standardFunctionsBuilder.build();
        CHECK_ERROR(success);
        standardFunctionRegistry.checkStandard();

        // Validate pattern and build data model
        // First, check that there is a data model
        // (which might have been set by setDataModel(), or to
        // the data model parsed from the pattern by setPattern())

        if (!builder.hasDataModel) {
            success = U_INVALID_STATE_ERROR;
            return;
        }

        dataModel = builder.dataModel;
        if (builder.errors != nullptr) {
            errors = new StaticErrors(*builder.errors, success);
        } else {
            // Initialize errors
            LocalPointer<StaticErrors> errorsNew(new StaticErrors(success));
            CHECK_ERROR(success);
            errors = errorsNew.orphan();
        }

        // Initialize formatter cache
        cachedFormatters = new CachedFormatters();
        if (cachedFormatters == nullptr) {
            success = U_MEMORY_ALLOCATION_ERROR;
            return;
        }

        // Note: we currently evaluate variables lazily,
        // without memoization. This call is still necessary
        // to check out-of-scope uses of local variables in
        // right-hand sides (unresolved variable errors can
        // only be checked when arguments are known)

        // Check for resolution errors
        Checker(dataModel, *errors).check(success);
    }

    void MessageFormatter::cleanup() noexcept {
        if (cachedFormatters != nullptr) {
            delete cachedFormatters;
        }
        if (errors != nullptr) {
            delete errors;
        }
        if (standardFormatters != nullptr) {
            delete standardFormatters;
        }
    }

    MessageFormatter& MessageFormatter::operator=(MessageFormatter&& other) noexcept {
        cleanup();

        locale = std::move(other.locale);
        standardFunctionRegistry = std::move(other.standardFunctionRegistry);
        customFunctionRegistry = other.customFunctionRegistry;
        dataModel = std::move(other.dataModel);
        normalizedInput = std::move(other.normalizedInput);
        cachedFormatters = other.cachedFormatters;
        other.cachedFormatters = nullptr;
        errors = other.errors;
        other.errors = nullptr;
        standardFormatters = other.standardFormatters;
        other.standardFormatters = nullptr;

        return *this;
    }

    const MessageFormatDataModel& MessageFormatter::getDataModel() const { return dataModel; }

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

    MessageFormatter::~MessageFormatter() {
        cleanup();
    }

    // Selector and formatter lookup
    // -----------------------------

    // Postcondition: selector != nullptr || U_FAILURE(status)
    Selector* MessageFormatter::getSelector(MessageContext& context, const FunctionName& functionName, UErrorCode& status) const {
        NULL_ON_ERROR(status);
        U_ASSERT(isSelector(functionName));

        const SelectorFactory* selectorFactory = lookupSelectorFactory(context, functionName, status);
        NULL_ON_ERROR(status);
        if (selectorFactory == nullptr) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return nullptr;
        }
        // Create a specific instance of the selector
        auto result = selectorFactory->createSelector(getLocale(), status);
        NULL_ON_ERROR(status);
        return result;
    }

    // Precondition: formatter is defined
    const Formatter& MessageFormatter::getFormatter(MessageContext& context, const FunctionName& functionName, UErrorCode& status) const {
        U_ASSERT(isFormatter(functionName));
        return *maybeCachedFormatter(context, functionName, status);
    }

    bool MessageFormatter::getFormatterByType(const UnicodeString& type, FunctionName& name) const {
        U_ASSERT(hasCustomFunctionRegistry());
        const FunctionRegistry& reg = getCustomFunctionRegistry();
        return reg.getFormatterByType(type, name);
    }

        // ---------------------------------------------------
    // Function registry


    bool MessageFormatter::isBuiltInSelector(const FunctionName& functionName) const {
        return standardFunctionRegistry.hasSelector(functionName);
    }

    bool MessageFormatter::isBuiltInFormatter(const FunctionName& functionName) const {
        return standardFunctionRegistry.hasFormatter(functionName);
    }

    // https://github.com/unicode-org/message-format-wg/issues/409
    // Unknown function = unknown function error
    // Formatter used as selector  = selector error
    // Selector used as formatter = formatting error
    const SelectorFactory* MessageFormatter::lookupSelectorFactory(MessageContext& context, const FunctionName& functionName, UErrorCode& status) const {
        DynamicErrors& err = context.getErrors();

        if (isBuiltInSelector(functionName)) {
            return standardFunctionRegistry.getSelector(functionName);
        }
        if (isBuiltInFormatter(functionName)) {
            err.setSelectorError(functionName, status);
            return nullptr;
        }
        if (hasCustomFunctionRegistry()) {
            const FunctionRegistry& customFunctionRegistry = getCustomFunctionRegistry();
            const SelectorFactory* selectorFactory = customFunctionRegistry.getSelector(functionName);
            if (selectorFactory != nullptr) {
                return selectorFactory;
            }
            if (customFunctionRegistry.getFormatter(functionName) != nullptr) {
                err.setSelectorError(functionName, status);
                return nullptr;
            }
        }
        // Either there is no custom function registry and the function
        // isn't built-in, or the function doesn't exist in either the built-in
        // or custom registry.
        // Unknown function error
        err.setUnknownFunction(functionName, status);
        return nullptr;
    }

    FormatterFactory* MessageFormatter::lookupFormatterFactory(MessageContext& context, const FunctionName& functionName, UErrorCode& status) const {
        DynamicErrors& err = context.getErrors();

        if (isBuiltInFormatter(functionName)) {
            return standardFunctionRegistry.getFormatter(functionName);
        }
        if (isBuiltInSelector(functionName)) {
            err.setFormattingError(functionName, status);
            return nullptr;
        }
        if (hasCustomFunctionRegistry()) {
            const FunctionRegistry& customFunctionRegistry = getCustomFunctionRegistry();
            FormatterFactory* formatterFactory = customFunctionRegistry.getFormatter(functionName);
            if (formatterFactory != nullptr) {
                return formatterFactory;
            }
            if (customFunctionRegistry.getSelector(functionName) != nullptr) {
                err.setFormattingError(functionName, status);
                return nullptr;
            }
        }
        // Either there is no custom function registry and the function
        // isn't built-in, or the function doesn't exist in either the built-in
        // or custom registry.
        // Unknown function error
        err.setUnknownFunction(functionName, status);
        return nullptr;
    }

    bool MessageFormatter::isCustomFormatter(const FunctionName& fn) const {
        return hasCustomFunctionRegistry() && getCustomFunctionRegistry().getFormatter(fn) != nullptr;
    }


    bool MessageFormatter::isCustomSelector(const FunctionName& fn) const {
        return hasCustomFunctionRegistry() && getCustomFunctionRegistry().getSelector(fn) != nullptr;
    }

    const Formatter* MessageFormatter::maybeCachedFormatter(MessageContext& context, const FunctionName& functionName, UErrorCode& errorCode) const {
        NULL_ON_ERROR(errorCode);
        U_ASSERT(cachedFormatters != nullptr);

        const Formatter* result = cachedFormatters->getFormatter(functionName);
        if (result == nullptr) {
            // Create the formatter

            // First, look up the formatter factory for this function
            FormatterFactory* formatterFactory = lookupFormatterFactory(context, functionName, errorCode);
            NULL_ON_ERROR(errorCode);

            // If the formatter factory was null, there must have been
            // an earlier error/warning
            if (formatterFactory == nullptr) {
                U_ASSERT(context.getErrors().hasUnknownFunctionError() || context.getErrors().hasFormattingError());
                return nullptr;
            }

            // Create a specific instance of the formatter
            Formatter* formatter = formatterFactory->createFormatter(locale, errorCode);
            NULL_ON_ERROR(errorCode);
            if (formatter == nullptr) {
                errorCode = U_MEMORY_ALLOCATION_ERROR;
                return nullptr;
            }
            cachedFormatters->adoptFormatter(functionName, formatter, errorCode);
            return formatter;
        } else {
            return result;
        }
    }

} // namespace message2

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */
