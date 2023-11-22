// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/messageformat2.h"
#include "messageformat2_context.h"
#include "messageformat2_function_registry_internal.h"
#include "messageformat2_macros.h"
#include "messageformat2_parser.h"
#include "uvector.h" // U_ASSERT

U_NAMESPACE_BEGIN

// Export an explicit template instantiation of the LocalPointer that is used as a
// data member of various MessageFormatDataModel classes.
// (When building DLLs for Windows this is required.)
// (See messageformat2_data_model_forward_decls.h for similar examples.)
#if U_PF_WINDOWS <= U_PLATFORM && U_PLATFORM <= U_PF_CYGWIN
#if defined(_MSC_VER)
// Ignore warning 4661 as LocalPointerBase does not use operator== or operator!=
#pragma warning(disable: 4661)
#endif
template class U_I18N_API LocalPointer<message2::FunctionRegistry::Builder>;
template class U_I18N_API LocalPointer<message2::MessageFormatDataModel::Builder>;
template class U_I18N_API LocalPointer<message2::Parser>;
#endif

namespace message2 {

// -------------------------------------
// Creates a MessageFormat instance based on the pattern.

// Returns a new (uninitialized) builder
MessageFormatter::Builder* MessageFormatter::builder(UErrorCode& errorCode) {
    NULL_ON_ERROR(errorCode);
    MessageFormatter::Builder* tree = new Builder();
    if (tree == nullptr) {
        errorCode = U_MEMORY_ALLOCATION_ERROR;
    }
    return tree;
}

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
MessageFormatter* MessageFormatter::Builder::build(UParseError& parseError, UErrorCode& errorCode) const {
    NULL_ON_ERROR(errorCode);

    return new MessageFormatter(*this, parseError, errorCode);
}

MessageFormatter::MessageFormatter(const MessageFormatter::Builder& builder, UParseError &parseError,
                                   UErrorCode &success) : locale(builder.locale), customFunctionRegistry(builder.customFunctionRegistry) {
    CHECK_ERROR(success);

    // Set up the standard function registry
    LocalPointer<FunctionRegistry::Builder> standardFunctionsBuilder(FunctionRegistry::builder(success));
    CHECK_ERROR(success);

    standardFunctionsBuilder->setFormatter(UnicodeString("datetime"), new StandardFunctions::DateTimeFactory(), success)
        .setFormatter(UnicodeString("number"), new StandardFunctions::NumberFactory(), success)
        .setFormatter(UnicodeString("identity"), new StandardFunctions::IdentityFactory(), success)
        .setSelector(UnicodeString("plural"), new StandardFunctions::PluralFactory(UPLURAL_TYPE_CARDINAL), success)
        .setSelector(UnicodeString("selectordinal"), new StandardFunctions::PluralFactory(UPLURAL_TYPE_ORDINAL), success)
        .setSelector(UnicodeString("select"), new StandardFunctions::TextFactory(), success)
        .setSelector(UnicodeString("gender"), new StandardFunctions::TextFactory(), success);
    standardFunctionRegistry.adoptInstead(standardFunctionsBuilder->build(success));
    CHECK_ERROR(success);
    standardFunctionRegistry->checkStandard();

    errors = Errors::create(success);
    CHECK_ERROR(success);

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
    cachedFormatters = new CachedFormatters(success);
    CHECK_ERROR(success);

    // Parse the pattern
    LocalPointer<Parser> parser(Parser::create(builder.pattern, tree, normalizedInput, *errors, success));
    CHECK_ERROR(success);
    parser->parse(parseError, success);

    // Build the data model based on what was parsed
    dataModel = tree.build(success);
}

const MessageFormatDataModel& MessageFormatter::getDataModel() const { return dataModel; }

MessageFormatter::~MessageFormatter() {
    if (cachedFormatters != nullptr) {
        delete cachedFormatters;
    }
    if (errors != nullptr) {
        delete errors;
    }
}
MessageFormatter::Builder::~Builder() {}

} // namespace message2

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */
