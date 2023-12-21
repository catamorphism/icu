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
    MessageFormatter::Builder& MessageFormatter::Builder::setFunctionRegistry(std::shared_ptr<FunctionRegistry> reg) {
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
    MessageFormatter MessageFormatter::Builder::build(UParseError& parseError, UErrorCode& errorCode) const noexcept {
        return MessageFormatter(*this, parseError, errorCode);
    }

    MessageFormatter::MessageFormatter(const MessageFormatter::Builder& builder, UParseError &parseError,
                                       UErrorCode &success) noexcept : locale(builder.locale), customFunctionRegistry(builder.customFunctionRegistry) {
        CHECK_ERROR(success);

        // Set up the standard function registry
        FunctionRegistry::Builder standardFunctionsBuilder;

        standardFunctionsBuilder.setFormatter(UnicodeString("datetime"), new StandardFunctions::DateTimeFactory())
            .setFormatter(UnicodeString("number"), new StandardFunctions::NumberFactory())
            .setFormatter(UnicodeString("identity"), new StandardFunctions::IdentityFactory())
            .setSelector(UnicodeString("plural"), new StandardFunctions::PluralFactory(UPLURAL_TYPE_CARDINAL))
            .setSelector(UnicodeString("selectordinal"), new StandardFunctions::PluralFactory(UPLURAL_TYPE_ORDINAL))
            .setSelector(UnicodeString("select"), new StandardFunctions::TextFactory())
            .setSelector(UnicodeString("gender"), new StandardFunctions::TextFactory());
        standardFunctionRegistry = standardFunctionsBuilder.build();
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

        MessageFormatDataModel::Builder tree;

        // Initialize formatter cache
        CachedFormatters* cachedFormattersPtr = new CachedFormatters();
        if (cachedFormattersPtr == nullptr) {
            success = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
        cachedFormatters = std::unique_ptr<CachedFormatters>(cachedFormattersPtr);

        // Parse the pattern
        Parser(builder.pattern, tree, errors, normalizedInput).parse(parseError, success);

        // Build the data model based on what was parsed
        dataModel = tree.build(success);

        // Note: we currently evaluate variables lazily,
        // without memoization. This call is still necessary
        // to check out-of-scope uses of local variables in
        // right-hand sides (unresolved variable errors can
        // only be checked when arguments are known)

        // Check for resolution errors
        Checker(dataModel, errors).check();
    }

    MessageFormatter& MessageFormatter::operator=(MessageFormatter&& other) noexcept {
        locale = std::move(other.locale);
        standardFunctionRegistry = std::move(other.standardFunctionRegistry);
        customFunctionRegistry = other.customFunctionRegistry;
        dataModel = std::move(other.dataModel);
        normalizedInput = std::move(other.normalizedInput);
        cachedFormatters = std::move(other.cachedFormatters);
        errors = std::move(other.errors);

        return *this;
    }

    MessageFormatter::MessageFormatter(MessageFormatter&& other) noexcept {
        if (this != &other) {
            locale = std::move(other.locale);
            standardFunctionRegistry = std::move(other.standardFunctionRegistry);
            customFunctionRegistry = other.customFunctionRegistry;
            dataModel = std::move(other.dataModel);
            normalizedInput = std::move(other.normalizedInput);
            cachedFormatters = std::move(other.cachedFormatters);
            errors = std::move(other.errors);
        }
    }

    const MessageFormatDataModel& MessageFormatter::getDataModel() const { return dataModel; }

    MessageFormatter::~MessageFormatter() {}
    MessageFormatter::Builder::~Builder() {}

} // namespace message2

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */
