// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/dtptngen.h"
#include "unicode/messageformat2.h"
#include "unicode/numberformatter.h"
#include "unicode/smpdtfmt.h"
#include "messageformat2_context.h"
#include "messageformat2_function_registry_internal.h"
#include "messageformat2_macros.h"
#include "uvector.h" // U_ASSERT

U_NAMESPACE_BEGIN

namespace message2 {

// Function registry implementation

Formatter::~Formatter() {}
Selector::~Selector() {}
FormatterFactory::~FormatterFactory() {}
SelectorFactory::~SelectorFactory() {}

FunctionRegistry FunctionRegistry::Builder::build() {
    U_ASSERT(formatters != nullptr && selectors != nullptr);
    FunctionRegistry result = FunctionRegistry(formatters, selectors);
    formatters = nullptr;
    selectors = nullptr;
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

FunctionRegistry::Builder::Builder(UErrorCode& errorCode) {
    CHECK_ERROR(errorCode);

    // Maps don't own their values
    formatters = new Hashtable();
    selectors = new Hashtable();
    if (!(formatters != nullptr && selectors != nullptr)) {
        errorCode = U_MEMORY_ALLOCATION_ERROR;
    }
}

FunctionRegistry::Builder::~Builder() {
    if (formatters != nullptr) {
        delete formatters;
    }
    if (selectors != nullptr) {
        delete selectors;
    }
}

FormatterFactory* FunctionRegistry::getFormatter(const FunctionName& formatterName) const {
    U_ASSERT(formatters != nullptr);
    return static_cast<FormatterFactory*>(formatters->get(formatterName.toString()));
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
#ifdef _DEBUG
    U_ASSERT(hasFormatter(FunctionName(UnicodeString(s))));
#else
   (void) s;
#endif
}

void FunctionRegistry::checkSelector(const char* s) const {
#ifdef _DEBUG
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

// Converts `s` to an int64 value if possible, returning false
// if it can't be parsed
static bool tryStringToNumber(const UnicodeString& s, int64_t& result) {
    UErrorCode localErrorCode = U_ZERO_ERROR;
    // Try to parse string as int

    LocalPointer<NumberFormat> numberFormat(NumberFormat::createInstance(localErrorCode));
    if (U_FAILURE(localErrorCode)) {
        return false;
    }
    numberFormat->setParseIntegerOnly(true);
    icu::Formattable asNumber;
    numberFormat->parse(s, asNumber, localErrorCode);
    if (U_SUCCESS(localErrorCode)) {
        result = asNumber.getInt64(localErrorCode);
        if (U_SUCCESS(localErrorCode)) {
            return true;
        }
    }
    return false;
}

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

// Converts `optionValue` to an int64 value if possible, returning false
// if it can't be parsed
bool tryFormattableAsNumber(const Formattable& optionValue, int64_t& result) {
    UErrorCode localErrorCode = U_ZERO_ERROR;
    if (optionValue.isNumeric()) {
        result = optionValue.getInt64(localErrorCode);
        if (U_SUCCESS(localErrorCode)) {
            return true;
        }
    } else {
        if (tryStringToNumber(optionValue.getString(), result)) {
            return true;
        }
    }
    return false;
}

static bool tryStringAsNumber(const Locale& locale, const Formattable& val, double& value) {
    // Check for a string option, try to parse it as a number if present
    if (val.getType() != Formattable::Type::kString) {
        return false;
    }
    UnicodeString tempString = val.getString();
    UErrorCode localErrorCode = U_ZERO_ERROR;
    LocalPointer<NumberFormat> numberFormat(NumberFormat::createInstance(locale, localErrorCode));
    if (U_FAILURE(localErrorCode)) {
        return false;
    }
    icu::Formattable asNumber;
    numberFormat->parse(tempString, asNumber, localErrorCode);
    if (U_FAILURE(localErrorCode)) {
        return false;
    }
    value = asNumber.getDouble(localErrorCode);
    if (U_FAILURE(localErrorCode)) {
        return false;
    }
    return true;
}

static UBool getInt64Value(const Locale& locale, const Formattable& value, int64_t& result) {
    if (!value.isNumeric()) {
        double doubleResult;
        if (tryStringAsNumber(locale, value, doubleResult)) {
            result = (int64_t) doubleResult;
            return true;
        }
        return false;
    }
    UErrorCode localErrorCode = U_ZERO_ERROR;
    result = value.getInt64(localErrorCode);
    if (U_SUCCESS(localErrorCode)) {
        return true;
    }
    // Option was numeric but couldn't be converted to int64_t -- could be overflow
    return false;
}

// Adopts its arguments
FunctionRegistry::FunctionRegistry(FormatterMap* f, SelectorMap* s) : formatters(f), selectors(s) {
    U_ASSERT(f != nullptr && s != nullptr);
}

FunctionRegistry& FunctionRegistry::operator=(FunctionRegistry&& other) noexcept {
    formatters = other.formatters;
    selectors = other.selectors;
    other.formatters = nullptr;
    other.selectors = nullptr;

    return *this;
}

FunctionRegistry::~FunctionRegistry() {
    if (formatters != nullptr) {
        delete formatters;
    }
    if (selectors != nullptr) {
        delete selectors;
    }
}

// Specific formatter implementations

// --------- Number

/* static */ number::LocalizedNumberFormatter StandardFunctions::formatterForOptions(Locale locale,
                                                                                     const FunctionOptions& opts,
                                                                                     UErrorCode& status) {
    number::UnlocalizedNumberFormatter nf;
    if (U_SUCCESS(status)) {
        Formattable opt;
        if (opts.getFunctionOption(UnicodeString("skeleton"), opt) && opt.getType() == Formattable::Type::kString) {
            nf = number::NumberFormatter::forSkeleton(opt.getString(), status);
        } else {
            int64_t minFractionDigits = 0;
            if (opts.getFunctionOption(UnicodeString("minimumFractionDigits"), opt)) {
                if (!getInt64Value(locale, opt, minFractionDigits)) {
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

    // Precondition: `input`'s source Formattable has type string

    double numberValue;
    UErrorCode localErrorCode = U_ZERO_ERROR;
    strToDouble(input.asFormattable().getString(), locale, numberValue, localErrorCode);
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

    int64_t offset;
    Formattable opt;
    if (!(opts.getFunctionOption(UnicodeString("offset"), opt) && getInt64Value(locale, opt, offset))) {
        offset = 0;
    }

    number::LocalizedNumberFormatter realFormatter;
    if (opts.optionsCount() == 0) {
        realFormatter = number::LocalizedNumberFormatter(icuFormatter);
    } else {
        realFormatter = formatterForOptions(locale, opts, errorCode);
    }

    number::FormattedNumber numberResult;
    // Already checked that contents can be formatted
    const Formattable& toFormat = arg.asFormattable();
    switch (toFormat.getType()) {
    case Formattable::Type::kDouble: {
        numberResult = realFormatter.formatDouble(toFormat.getDouble() - offset, errorCode);
        break;
    }
    case Formattable::Type::kLong: {
        numberResult = realFormatter.formatInt(toFormat.getLong() - offset, errorCode);
        break;
    }
    case Formattable::Type::kInt64: {
        numberResult = realFormatter.formatInt(toFormat.getInt64() - offset, errorCode);
        break;
    }
    case Formattable::Type::kString: {
        // Try to parse the string as a number
        return stringAsNumber(locale, realFormatter, arg, offset, errorCode);
    }
    default: {
        // Other types can't be parsed as a number
        return notANumber(arg);
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

static void tryAsString(const Locale& locale, const UnicodeString& s, double& valToCheck, bool& noMatch) {
    // Try parsing the inputString as a double
    UErrorCode localErrorCode = U_ZERO_ERROR;
    strToDouble(s, locale, valToCheck, localErrorCode);
    // Invalid format error => value is not a number; no match
    if (U_FAILURE(localErrorCode)) {
        noMatch = true;
        return;
    }
    noMatch = false;
}

static void tryWithFormattable(const Locale& locale, const Formattable& value, double& valToCheck, bool& noMatch) {
    switch (value.getType()) {
        case Formattable::Type::kDouble: {
            valToCheck = value.getDouble();
            break;
        }
        case Formattable::Type::kLong: {
            valToCheck = (double) value.getLong();
            break;
        }
        case Formattable::Type::kInt64: {
            valToCheck = (double) value.getInt64();
            break;
        }
        case Formattable::Type::kString: {
            tryAsString(locale, value.getString(), valToCheck, noMatch);
            return;
        }
        default: {
            noMatch = true;
            return;
        }
    }
    noMatch = false;
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

    int64_t offset;
    Formattable opt;
    if (!(opts.getFunctionOption(UnicodeString("offset"), opt) && getInt64Value(locale, opt, offset))) {
        offset = 0;
    }

    // Only doubles and integers can match
    double valToCheck;
    bool noMatch = true;

    bool isFormattedString = toFormat.isEvaluated() && toFormat.output().isString();
    bool isFormattedNumber = toFormat.isEvaluated() && toFormat.output().isNumber();

    if (isFormattedString) {
        // Formatted string: try parsing it as a number
        tryAsString(locale, toFormat.output().getString(), valToCheck, noMatch);
    } else {
        // Already checked that contents can be formatted
        tryWithFormattable(locale, toFormat.asFormattable(), valToCheck, noMatch);
    }

    if (noMatch) {
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
    if (opts.getFunctionOption(UnicodeString("skeleton"), opt) && opt.getType() == Formattable::Type::kString) {
        // Same as getInstanceForSkeleton(), see ICU 9029
        // Based on test/intltest/dtfmttst.cpp - TestPatterns()
        LocalPointer<DateTimePatternGenerator> generator(DateTimePatternGenerator::createInstance(locale, errorCode));
        UnicodeString pattern = generator->getBestPattern(opt.getString(), errorCode);
        df.adoptInstead(new SimpleDateFormat(pattern, locale, errorCode));
    } else {
        if (opts.getFunctionOption(UnicodeString("pattern"), opt) && opt.getType() == Formattable::Type::kString) {
            df.adoptInstead(new SimpleDateFormat(opt.getString(), locale, errorCode));
        } else {
            DateFormat::EStyle dateStyle = DateFormat::NONE;
            if (opts.getFunctionOption(UnicodeString("datestyle"), opt) && opt.getType() == Formattable::Type::kString) {
                dateStyle = stringToStyle(opt.getString(), errorCode);
            }
            DateFormat::EStyle timeStyle = DateFormat::NONE;
            if (opts.getFunctionOption(UnicodeString("timestyle"), opt) && opt.getType() == Formattable::Type::kString) {
                timeStyle = stringToStyle(opt.getString(), errorCode);
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

