// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/messageformat2_function_registry.h"
#include "unicode/messageformat2.h"
#include "messageformat2_context.h"
#include "messageformat2_macros.h"
#include "hash.h"
#include "uvector.h" // U_ASSERT

#if U_PF_WINDOWS <= U_PLATFORM && U_PLATFORM <= U_PF_CYGWIN && defined(_MSC_VER)
// Ignore warning 4661 as LocalPointerBase does not use operator== or operator!=
#pragma warning(disable: 4661)
#endif

U_NAMESPACE_BEGIN

namespace message2 {

using namespace data_model;

// Functions
// -------------

ResolvedFunctionOption::ResolvedFunctionOption(ResolvedFunctionOption&& other) {
    name = std::move(other.name);
    value = std::move(other.value);
}

ResolvedFunctionOption::~ResolvedFunctionOption() {}


const ResolvedFunctionOption* FunctionOptions::getResolvedFunctionOptions(int32_t& len) const {
    len = functionOptionsLen;
    U_ASSERT(len == 0 || options != nullptr);
    return options;
}

FunctionOptions::FunctionOptions(UVector&& optionsVector, UErrorCode& status) {
    CHECK_ERROR(status);

    options = moveVectorToArray<ResolvedFunctionOption>(optionsVector, functionOptionsLen);
    if (options == nullptr) {
        status = U_MEMORY_ALLOCATION_ERROR;
    }
}

UBool FunctionOptions::getFunctionOption(const UnicodeString& key, Formattable& option) const {
    if (options == nullptr) {
        U_ASSERT(functionOptionsLen == 0);
    }
    for (int32_t i = 0; i < functionOptionsLen; i++) {
        const ResolvedFunctionOption& opt = options[i];
        if (opt.getName() == key) {
            option = opt.getValue();
            return true;
        }
    }
    return false;
}

// ResolvedSelector
// ----------------

ResolvedSelector::ResolvedSelector(const FunctionName& fn,
                                   Selector* sel,
                                   FunctionOptions&& opts,
                                   FormattedPlaceholder&& val)
    : selectorName(fn), selector(sel), options(std::move(opts)), value(std::move(val))  {
    U_ASSERT(sel != nullptr);
}

ResolvedSelector::ResolvedSelector(FormattedPlaceholder&& val) : value(std::move(val)) {}

// Selector and formatter lookup
// -----------------------------

// Postcondition: selector != nullptr || U_FAILURE(status)
Selector* MessageContext::getSelector(const FunctionName& functionName, UErrorCode& status) {
    NULL_ON_ERROR(status);
    U_ASSERT(isSelector(functionName));

    const SelectorFactory* selectorFactory = lookupSelectorFactory(functionName, status);
    NULL_ON_ERROR(status);
    if (selectorFactory == nullptr) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return nullptr;
    }
    // Create a specific instance of the selector
    auto result = selectorFactory->createSelector(messageFormatter().getLocale(), status);
    NULL_ON_ERROR(status);
    return result;
}

// Precondition: formatter is defined
const Formatter& MessageContext::getFormatter(const FunctionName& functionName, UErrorCode& status) {
    U_ASSERT(isFormatter(functionName));
    return *maybeCachedFormatter(functionName, status);
}

} // namespace message2
U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */
