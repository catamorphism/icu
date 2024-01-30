// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/messageformat2.h"
#include "messageformat2_checker.h"
#include "messageformat2_context.h"
#include "messageformat2_function_registry_internal.h"
#include "messageformat2_macros.h"
#include "messageformat2_parser.h"
#include "uvector.h" // U_ASSERT

U_NAMESPACE_BEGIN

namespace message2 {

    // -------------------------------------
    // Creates a MessageFormat instance based on the pattern.

    MessageFormatter::Builder& MessageFormatter::Builder::setPattern(const UnicodeString& pat) {
        hasPattern = true;
        hasDataModel = false;
        pattern = pat;

        return *this;
    }

    // Precondition: `reg` is non-null
    // Does not adopt `reg`
    MessageFormatter::Builder& MessageFormatter::Builder::setFunctionRegistry(const FunctionRegistry* reg) {
        U_ASSERT(reg != nullptr);
        customFunctionRegistry = reg;
        return *this;
    }

    MessageFormatter::Builder& MessageFormatter::Builder::setLocale(const Locale& loc) {
        locale = loc;
        return *this;
    }

    MessageFormatter::Builder& MessageFormatter::Builder::setDataModel(MessageFormatDataModel&& newDataModel) {
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
    MessageFormatter MessageFormatter::Builder::build(UParseError& parseError, UErrorCode& errorCode) const {
        return MessageFormatter(*this, parseError, errorCode);
    }

    MessageFormatter::MessageFormatter(const MessageFormatter::Builder& builder, UParseError &parseError,
                                       UErrorCode &success) : locale(builder.locale), customFunctionRegistry(builder.customFunctionRegistry) {
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
        // First, check that exactly one of the pattern and data model are set, but not both

        if ((!builder.hasPattern && !builder.hasDataModel)
            || (builder.hasPattern && builder.hasDataModel)) {
            success = U_INVALID_STATE_ERROR;
            return;
        }

        // If data model was set, just assign it
        if (builder.hasDataModel) {
            dataModel = builder.dataModel;
            return;
        }

        MessageFormatDataModel::Builder tree(success);
        // Initialize errors
        LocalPointer<StaticErrors> errorsNew(new StaticErrors(success));
        CHECK_ERROR(success);
        errors = errorsNew.orphan();

        // Initialize formatter cache
        cachedFormatters = new CachedFormatters();
        if (cachedFormatters == nullptr) {
            success = U_MEMORY_ALLOCATION_ERROR;
            return;
        }

        // Parse the pattern
        Parser(builder.pattern, tree, *errors, normalizedInput).parse(parseError, success);

        // Build the data model based on what was parsed
        dataModel = tree.build(success);

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

    MessageFormatter::~MessageFormatter() {
        cleanup();
    }
    MessageFormatter::Builder::~Builder() {}

} // namespace message2

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */
