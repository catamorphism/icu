// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/dtptngen.h"
#include "unicode/messageformat2_function_registry.h"
#include "unicode/smpdtfmt.h"
#include "messageformat2_allocation.h"
#include "messageformat2_function_registry_internal.h"
#include "messageformat2_macros.h"
#include "hash.h"
#include "uvector.h" // U_ASSERT

U_NAMESPACE_BEGIN

namespace message2 {

// Function registry implementation

Formatter::~Formatter() {}
Selector::~Selector() {}
FormatterFactory::~FormatterFactory() {}
SelectorFactory::~SelectorFactory() {}

FunctionRegistry FunctionRegistry::Builder::build() {
    U_ASSERT(formatters != nullptr && selectors != nullptr && formattersByType != nullptr);
    FunctionRegistry result = FunctionRegistry(formatters, selectors, formattersByType);
    formatters = nullptr;
    selectors = nullptr;
    formattersByType = nullptr;
    return result;
}

// Does not adopt its argument
FunctionRegistry::Builder& FunctionRegistry::Builder::setSelector(const FunctionName& selectorName, SelectorFactory* selectorFactory, UErrorCode& errorCode) {
    if (U_SUCCESS(errorCode)) {
        U_ASSERT(selectors != nullptr);
        selectors->put(selectorName.toString(), selectorFactory, errorCode);
    }
    return *this;
}

// Does not adopt its argument
FunctionRegistry::Builder& FunctionRegistry::Builder::setFormatter(const FunctionName& formatterName, FormatterFactory* formatterFactory, UErrorCode& errorCode) {
    if (U_SUCCESS(errorCode)) {
        U_ASSERT(formatters != nullptr);
        formatters->put(formatterName.toString(), formatterFactory, errorCode);
    }
    return *this;
}

FunctionRegistry::Builder& FunctionRegistry::Builder::setFormatterByType(const UnicodeString& type, const FunctionName& functionName, UErrorCode& errorCode) {
    if (U_SUCCESS(errorCode)) {
        U_ASSERT(formattersByType != nullptr);
        FunctionName* f = create<FunctionName>(FunctionName(functionName), errorCode);
        formattersByType->put(type, f, errorCode);
    }
    return *this;
}

FunctionRegistry::Builder::Builder(UErrorCode& errorCode) {
    CHECK_ERROR(errorCode);

    formatters = new Hashtable();
    selectors = new Hashtable();
    formattersByType = new Hashtable();
    if (!(formatters != nullptr && selectors != nullptr && formattersByType != nullptr)) {
        errorCode = U_MEMORY_ALLOCATION_ERROR;
    }
    /*
      The `formatters` hash doesn't own its values. For motivation,
      see the TemperatureFormatter class and testFormatterIsCreatedOnce() test case,
      in messageformat2test_features.cpp
     */
    selectors->setValueDeleter(uprv_deleteUObject);
    formattersByType->setValueDeleter(uprv_deleteUObject);
}

FunctionRegistry::Builder::~Builder() {
    if (formatters != nullptr) {
        delete formatters;
    }
    if (selectors != nullptr) {
        delete selectors;
    }
    if (formattersByType != nullptr) {
        delete formattersByType;
    }
}

FormatterFactory* FunctionRegistry::getFormatter(const FunctionName& formatterName) const {
    U_ASSERT(formatters != nullptr);
    return static_cast<FormatterFactory*>(formatters->get(formatterName.toString()));
}

    UBool FunctionRegistry::getFormatterByType(const UnicodeString& type, FunctionName& name) const {
    U_ASSERT(formatters != nullptr);
    const FunctionName* f = static_cast<FunctionName*>(formattersByType->get(type));
    if (f != nullptr) {
        name = *f;
        return true;
    }
    return false;
}

const SelectorFactory* FunctionRegistry::getSelector(const FunctionName& selectorName) const {
    U_ASSERT(selectors != nullptr);
    return static_cast<const SelectorFactory*>(selectors->get(selectorName.toString()));
}

bool FunctionRegistry::hasFormatter(const FunctionName& f) const {
    return getFormatter(f) != nullptr;
}

bool FunctionRegistry::hasSelector(const FunctionName& s) const {
    return getSelector(s) != nullptr;
}

void FunctionRegistry::checkFormatter(const char* s) const {
#if U_DEBUG
    U_ASSERT(hasFormatter(FunctionName(UnicodeString(s))));
#else
   (void) s;
#endif
}

void FunctionRegistry::checkSelector(const char* s) const {
#if U_DEBUG
    U_ASSERT(hasSelector(FunctionName(UnicodeString(s))));
#else
    (void) s;
#endif
}

// Debugging
void FunctionRegistry::checkStandard() const {
    checkFormatter("datetime");
    checkFormatter("number");
    checkFormatter("identity");
    checkSelector("plural");
    checkSelector("selectordinal");
    checkSelector("select");
    checkSelector("gender");
}

// Formatter/selector helpers

// Converts `s` to a double, indicating failure via `errorCode`
static void strToDouble(const UnicodeString& s, const Locale& loc, double& result, UErrorCode& errorCode) {
    CHECK_ERROR(errorCode);

    LocalPointer<NumberFormat> numberFormat(NumberFormat::createInstance(loc, errorCode));
    CHECK_ERROR(errorCode);
    icu::Formattable asNumber;
    numberFormat->parse(s, asNumber, errorCode);
    CHECK_ERROR(errorCode);
    result = asNumber.getDouble(errorCode);
}

static int64_t tryStringAsNumber(const Locale& locale, const Formattable& val, UErrorCode& errorCode) {
    // Check for a string option, try to parse it as a number if present
    UnicodeString tempString = val.getString(errorCode);
    LocalPointer<NumberFormat> numberFormat(NumberFormat::createInstance(locale, errorCode));
    if (U_SUCCESS(errorCode)) {
        icu::Formattable asNumber;
        numberFormat->parse(tempString, asNumber, errorCode);
        if (U_SUCCESS(errorCode)) {
            return asNumber.getDouble(errorCode);
        }
    }
    return 0;
}

static int64_t getInt64Value(const Locale& locale, const Formattable& value, UErrorCode& errorCode) {
    if (U_SUCCESS(errorCode)) {
        if (!value.isNumeric()) {
            double doubleResult = tryStringAsNumber(locale, value, errorCode);
            if (U_SUCCESS(errorCode)) {
                return static_cast<int64_t>(doubleResult);
            }
        }
        else {
            int64_t result = value.getInt64(errorCode);
            if (U_SUCCESS(errorCode)) {
                return result;
            }
        }
    }
    // Option was numeric but couldn't be converted to int64_t -- could be overflow
    return 0;
}

// Adopts its arguments
FunctionRegistry::FunctionRegistry(FormatterMap* f, SelectorMap* s, Hashtable* byType) : formatters(f), selectors(s), formattersByType(byType) {
    U_ASSERT(f != nullptr && s != nullptr && byType != nullptr);
}

FunctionRegistry& FunctionRegistry::operator=(FunctionRegistry&& other) noexcept {
    cleanup();

    formatters = other.formatters;
    selectors = other.selectors;
    formattersByType = other.formattersByType;
    other.formatters = nullptr;
    other.selectors = nullptr;
    other.formattersByType = nullptr;

    return *this;
}

void FunctionRegistry::cleanup() noexcept {
    if (formatters != nullptr) {
        delete formatters;
    }
    if (selectors != nullptr) {
        delete selectors;
    }
    if (formattersByType != nullptr) {
        delete formattersByType;
    }
}


FunctionRegistry::~FunctionRegistry() {
    cleanup();
}

// Specific formatter implementations

// --------- Number

/* static */ number::LocalizedNumberFormatter StandardFunctions::formatterForOptions(Locale locale,
                                                                                     const FunctionOptions& opts,
                                                                                     UErrorCode& status) {
    number::UnlocalizedNumberFormatter nf;
    if (U_SUCCESS(status)) {
        Formattable opt;
        if (opts.getFunctionOption(UnicodeString("skeleton"), opt) && opt.getType() == UFMT_STRING) {
            const UnicodeString& skeletonStr = opt.getString(status);
            U_ASSERT(U_SUCCESS(status));
            nf = number::NumberFormatter::forSkeleton(skeletonStr, status);
        } else {
            int64_t minFractionDigits = 0;
            if (opts.getFunctionOption(UnicodeString("minimumFractionDigits"), opt)) {
                UErrorCode localErrorCode = U_ZERO_ERROR;
                minFractionDigits = getInt64Value(locale, opt, localErrorCode);
                if (U_FAILURE(localErrorCode)) {
                    minFractionDigits = 0;
                }
            }
            nf = number::NumberFormatter::with().precision(number::Precision::minFraction((int32_t) minFractionDigits));
        }
    }
    return number::LocalizedNumberFormatter(nf.locale(locale));
}

Formatter* StandardFunctions::NumberFactory::createFormatter(const Locale& locale, UErrorCode& errorCode) {
    NULL_ON_ERROR(errorCode);

    Formatter* result = new Number(locale);
    if (result == nullptr) {
        errorCode = U_MEMORY_ALLOCATION_ERROR;
        return nullptr;
    }
    return result;
}

static FormattedPlaceholder notANumber(const FormattedPlaceholder& input) {
    return FormattedPlaceholder(input, FormattedValue(UnicodeString("NaN")));
}

static FormattedPlaceholder stringAsNumber(Locale locale, const number::LocalizedNumberFormatter& nf, const FormattedPlaceholder& input, int64_t offset, UErrorCode& errorCode) {
    if (U_FAILURE(errorCode)) {
        return {};
    }

    double numberValue;
    const UnicodeString& inputStr = input.asFormattable().getString(errorCode);
    // Precondition: `input`'s source Formattable has type string
    if (U_FAILURE(errorCode)) {
        return {};
    }
    UErrorCode localErrorCode = U_ZERO_ERROR;
    strToDouble(inputStr, locale, numberValue, localErrorCode);
    if (U_FAILURE(localErrorCode)) {
        return notANumber(input);
    }
    UErrorCode savedStatus = errorCode;
    number::FormattedNumber result = nf.formatDouble(numberValue - offset, errorCode);
    // Ignore U_USING_DEFAULT_WARNING
    if (errorCode == U_USING_DEFAULT_WARNING) {
        errorCode = savedStatus;
    }
    return FormattedPlaceholder(input, FormattedValue(std::move(result)));
}

FormattedPlaceholder StandardFunctions::Number::format(FormattedPlaceholder&& arg, FunctionOptions&& opts, UErrorCode& errorCode) const {
    if (U_FAILURE(errorCode)) {
        return {};
    }

    // No argument => return "NaN"
    if (!arg.canFormat()) {
        return notANumber(arg);
    }

    int64_t offset = 0;
    Formattable opt;
    if (opts.getFunctionOption(UnicodeString("offset"), opt)) {
        UErrorCode localErrorCode = U_ZERO_ERROR;
        offset = getInt64Value(locale, opt, localErrorCode);
        if (U_FAILURE(localErrorCode)) {
            offset = 0;
        }
    }

    number::LocalizedNumberFormatter realFormatter;
    if (opts.optionsCount() == 0) {
        realFormatter = number::LocalizedNumberFormatter(icuFormatter);
    } else {
        realFormatter = formatterForOptions(locale, opts, errorCode);
    }

    number::FormattedNumber numberResult;
    if (U_SUCCESS(errorCode)) {
        // Already checked that contents can be formatted
        const Formattable& toFormat = arg.asFormattable();
        switch (toFormat.getType()) {
        case UFMT_DOUBLE: {
            double d = toFormat.getDouble(errorCode);
            U_ASSERT(U_SUCCESS(errorCode));
            numberResult = realFormatter.formatDouble(d - offset, errorCode);
            break;
        }
        case UFMT_LONG: {
            int32_t l = toFormat.getLong(errorCode);
            U_ASSERT(U_SUCCESS(errorCode));
            numberResult = realFormatter.formatInt(l - offset, errorCode);
            break;
        }
        case UFMT_INT64: {
            int64_t i = toFormat.getInt64(errorCode);
            U_ASSERT(U_SUCCESS(errorCode));
            numberResult = realFormatter.formatInt(i - offset, errorCode);
            break;
        }
        case UFMT_STRING: {
            // Try to parse the string as a number
            return stringAsNumber(locale, realFormatter, arg, offset, errorCode);
        }
        default: {
            // Other types can't be parsed as a number
            return notANumber(arg);
        }
        }
    }

    return FormattedPlaceholder(arg, FormattedValue(std::move(numberResult)));
}

StandardFunctions::Number::~Number() {}
StandardFunctions::NumberFactory::~NumberFactory() {}

// --------- PluralFactory

Selector* StandardFunctions::PluralFactory::createSelector(const Locale& locale, UErrorCode& errorCode) const {
    NULL_ON_ERROR(errorCode);

    // Look up plural rules by locale
    LocalPointer<PluralRules> rules(PluralRules::forLocale(locale, type, errorCode));
    NULL_ON_ERROR(errorCode);
    Selector* result = new Plural(locale, rules.orphan());
    if (result == nullptr) {
        errorCode = U_MEMORY_ALLOCATION_ERROR;
        return nullptr;
    }
    return result;
}

static double tryAsString(const Locale& locale, const UnicodeString& s, UErrorCode& errorCode) {
    if (U_FAILURE(errorCode)) {
        return 0;
    }
    // Try parsing the inputString as a double
    double valToCheck;
    strToDouble(s, locale, valToCheck, errorCode);
    return valToCheck;
}

static double tryWithFormattable(const Locale& locale, const Formattable& value, UErrorCode& errorCode) {
    if (U_FAILURE(errorCode)) {
        return 0;
    }
    double valToCheck;
    switch (value.getType()) {
        case UFMT_DOUBLE: {
            valToCheck = value.getDouble(errorCode);
            break;
        }
        case UFMT_LONG: {
            valToCheck = (double) value.getLong(errorCode);
            break;
        }
        case UFMT_INT64: {
            valToCheck = (double) value.getInt64(errorCode);
            break;
        }
        case UFMT_STRING: {
            const UnicodeString& s = value.getString(errorCode);
            U_ASSERT(U_SUCCESS(errorCode));
            return tryAsString(locale, s, errorCode);
        }
        default: {
            errorCode = U_ILLEGAL_ARGUMENT_ERROR;
            return 0;
        }
    }
    U_ASSERT(U_SUCCESS(errorCode));
    return valToCheck;
}

void StandardFunctions::Plural::selectKey(FormattedPlaceholder&& toFormat,
                                          FunctionOptions&& opts,
                                          const UnicodeString* keys,
                                          int32_t keysLen,
                                          UnicodeString* prefs,
                                          int32_t& prefsLen,
					  UErrorCode& errorCode) const {
    CHECK_ERROR(errorCode);

    // No argument => return "NaN"
    if (!toFormat.canFormat()) {
        errorCode = U_SELECTOR_ERROR;
        return;
    }

    int64_t offset = 0;
    Formattable opt;
    if (opts.getFunctionOption(UnicodeString("offset"), opt)) {
        UErrorCode localErrorCode = U_ZERO_ERROR;
        offset = getInt64Value(locale, opt, localErrorCode);
        if (U_FAILURE(localErrorCode)) {
            offset = 0;
        }
    }

    // Only doubles and integers can match
    double valToCheck;

    bool isFormattedString = toFormat.isEvaluated() && toFormat.output().isString();
    bool isFormattedNumber = toFormat.isEvaluated() && toFormat.output().isNumber();

    if (isFormattedString) {
        // Formatted string: try parsing it as a number
        valToCheck = tryAsString(locale, toFormat.output().getString(), errorCode);
    } else {
        // Already checked that contents can be formatted
        valToCheck = tryWithFormattable(locale, toFormat.asFormattable(), errorCode);
    }

    if (U_FAILURE(errorCode)) {
        // Non-number => selector error
        errorCode = U_SELECTOR_ERROR;
        return;
    }

    // Generate the matches
    // -----------------------

    // First, check for an exact match
    prefsLen = 0;
    double keyAsDouble = 0;
    for (int32_t i = 0; i < keysLen; i++) {
        // Try parsing the key as a double
        UErrorCode localErrorCode = U_ZERO_ERROR;
        strToDouble(keys[i], locale, keyAsDouble, localErrorCode);
        if (U_SUCCESS(localErrorCode)) {
            if (valToCheck == keyAsDouble) {
		prefs[0] = keys[i];
                prefsLen = 1;
                break;
            }
        }
    }
    if (prefsLen > 0) {
        return;
    }

    // If there was no exact match, check for a match based on the plural category
    UnicodeString match;
    if (isFormattedNumber) {
        match = rules->select(toFormat.output().getNumber(), errorCode);
    } else {
        match = rules->select(valToCheck - offset);
    }
    CHECK_ERROR(errorCode);

    for (int32_t i = 0; i < keysLen; i ++) {
        if (match == keys[i]) {
            prefs[0] = keys[i];
            prefsLen = 1;
            break;
        }
    }
}

StandardFunctions::Plural::~Plural() {}
StandardFunctions::PluralFactory::~PluralFactory() {}

// --------- DateTimeFactory

static DateFormat::EStyle stringToStyle(UnicodeString option, UErrorCode& errorCode) {
    if (U_SUCCESS(errorCode)) {
        UnicodeString upper = option.toUpper();
        if (upper == UnicodeString("FULL")) {
            return DateFormat::EStyle::kFull;
        }
        if (upper == UnicodeString("LONG")) {
            return DateFormat::EStyle::kLong;
        }
        if (upper == UnicodeString("MEDIUM")) {
            return DateFormat::EStyle::kMedium;
        }
        if (upper == UnicodeString("SHORT")) {
            return DateFormat::EStyle::kShort;
        }
        if (upper.isEmpty() || upper == UnicodeString("DEFAULT")) {
            return DateFormat::EStyle::kDefault;
        }
        errorCode = U_ILLEGAL_ARGUMENT_ERROR;
    }
    return DateFormat::EStyle::kNone;
}

Formatter* StandardFunctions::DateTimeFactory::createFormatter(const Locale& locale, UErrorCode& errorCode) {
    NULL_ON_ERROR(errorCode);

    Formatter* result = new DateTime(locale);
    if (result == nullptr) {
        errorCode = U_MEMORY_ALLOCATION_ERROR;
        return nullptr;
    }
    return result;
}

FormattedPlaceholder StandardFunctions::DateTime::format(FormattedPlaceholder&& toFormat,
                                                   FunctionOptions&& opts,
                                                   UErrorCode& errorCode) const {
    if (U_FAILURE(errorCode)) {
        return {};
    }

    // Argument must be present
    if (!toFormat.canFormat()) {
        errorCode = U_FORMATTING_ERROR;
        return std::move(toFormat);
    }

    LocalPointer<DateFormat> df;
    Formattable opt;
    if (opts.getFunctionOption(UnicodeString("skeleton"), opt) && opt.getType() == UFMT_STRING) {
        // Same as getInstanceForSkeleton(), see ICU 9029
        // Based on test/intltest/dtfmttst.cpp - TestPatterns()
        LocalPointer<DateTimePatternGenerator> generator(DateTimePatternGenerator::createInstance(locale, errorCode));
        const UnicodeString& s = opt.getString(errorCode);
        U_ASSERT(U_SUCCESS(errorCode));
        UnicodeString pattern = generator->getBestPattern(s, errorCode);
        df.adoptInstead(new SimpleDateFormat(pattern, locale, errorCode));
    } else {
        if (opts.getFunctionOption(UnicodeString("pattern"), opt) && opt.getType() == UFMT_STRING) {
            const UnicodeString& s = opt.getString(errorCode);
            U_ASSERT(U_SUCCESS(errorCode));
            df.adoptInstead(new SimpleDateFormat(s, locale, errorCode));
        } else {
            DateFormat::EStyle dateStyle = DateFormat::NONE;
            if (opts.getFunctionOption(UnicodeString("datestyle"), opt) && opt.getType() == UFMT_STRING) {
                const UnicodeString& s = opt.getString(errorCode);
                U_ASSERT(U_SUCCESS(errorCode));
                dateStyle = stringToStyle(s, errorCode);
            }
            DateFormat::EStyle timeStyle = DateFormat::NONE;
            if (opts.getFunctionOption(UnicodeString("timestyle"), opt) && opt.getType() == UFMT_STRING) {
                const UnicodeString& s = opt.getString(errorCode);
                U_ASSERT(U_SUCCESS(errorCode));
                timeStyle = stringToStyle(s, errorCode);
            }
            if (dateStyle == DateFormat::NONE && timeStyle == DateFormat::NONE) {
                df.adoptInstead(defaultDateTimeInstance(locale, errorCode));
            } else {
                df.adoptInstead(DateFormat::createDateTimeInstance(dateStyle, timeStyle, locale));
                if (!df.isValid()) {
                    errorCode = U_MEMORY_ALLOCATION_ERROR;
                    return {};
                }
            }
        }
    }

    if (U_FAILURE(errorCode)) {
        return {};
    }

    UnicodeString result;
    const Formattable& source = toFormat.asFormattable();
    df->format(source.asICUFormattable(errorCode), result, 0, errorCode);
    if (U_FAILURE(errorCode)) {
        return {};
    }
    return FormattedPlaceholder(toFormat, FormattedValue(std::move(result)));
}

StandardFunctions::DateTimeFactory::~DateTimeFactory() {}
StandardFunctions::DateTime::~DateTime() {}

// --------- TextFactory

Selector* StandardFunctions::TextFactory::createSelector(const Locale& locale, UErrorCode& errorCode) const {
    Selector* result = new TextSelector(locale);
    if (result == nullptr) {
        errorCode = U_MEMORY_ALLOCATION_ERROR;
        return nullptr;
    }
    return result;
}

void StandardFunctions::TextSelector::selectKey(FormattedPlaceholder&& toFormat,
                                                FunctionOptions&& opts,
                                                const UnicodeString* keys,
                                                int32_t keysLen,
                                                UnicodeString* prefs,
                                                int32_t& prefsLen,
						UErrorCode& errorCode) const {
    // No options
    (void) opts;

    CHECK_ERROR(errorCode);

    // Just compares the key and value as strings

    // Argument must be present
    if (!toFormat.canFormat()) {
        errorCode = U_SELECTOR_ERROR;
        return;
    }

    prefsLen = 0;

    // Convert to string
    const UnicodeString& formattedValue = toFormat.formatToString(locale, errorCode);
    if (U_FAILURE(errorCode)) {
        return;
    }

    for (int32_t i = 0; i < keysLen; i++) {
        if (keys[i] == formattedValue) {
	    prefs[0] = keys[i];
            prefsLen = 1;
            break;
        }
    }
}

StandardFunctions::TextFactory::~TextFactory() {}
StandardFunctions::TextSelector::~TextSelector() {}

// --------- IdentityFactory

Formatter* StandardFunctions::IdentityFactory::createFormatter(const Locale& locale, UErrorCode& errorCode) {
    Formatter* result = new Identity(locale);
    if (result == nullptr) {
        errorCode = U_MEMORY_ALLOCATION_ERROR;
        return nullptr;
    }
    return result;

}

FormattedPlaceholder StandardFunctions::Identity::format(FormattedPlaceholder&& toFormat,
                                                   FunctionOptions&& opts,
                                                   UErrorCode& errorCode) const {
    // No options
    (void) opts;

    if (U_FAILURE(errorCode)) {
        return {};
    }

    // Argument must be present
    if (!toFormat.canFormat()) {
        errorCode = U_FORMATTING_ERROR;
        return std::move(toFormat);
    }

    // Just returns the contents as a string
    return FormattedPlaceholder(toFormat, FormattedValue(toFormat.formatToString(locale, errorCode)));
}

StandardFunctions::IdentityFactory::~IdentityFactory() {}
StandardFunctions::Identity::~Identity() {}

} // namespace message2
U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */

