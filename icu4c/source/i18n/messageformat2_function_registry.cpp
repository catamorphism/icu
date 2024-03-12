// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/dtptngen.h"
#include "unicode/messageformat2_data_model_names.h"
#include "unicode/messageformat2_function_registry.h"
#include "unicode/smpdtfmt.h"
#include "messageformat2_allocation.h"
#include "messageformat2_function_registry_internal.h"
#include "messageformat2_macros.h"
#include "hash.h"
#include "number_types.h"
#include "uvector.h" // U_ASSERT

#include <math.h>

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
    checkFormatter("date");
    checkFormatter("time");
    checkFormatter("number");
    checkFormatter("integer");
    checkSelector("number");
    checkSelector("integer");
    checkSelector("string");
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

/* static */ number::LocalizedNumberFormatter StandardFunctions::formatterForOptions(const Number& number,
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
            int64_t maxFractionDigits = number.maximumFractionDigits(opts);
            if (opts.getFunctionOption(UnicodeString("minimumFractionDigits"), opt)) {
                UErrorCode localErrorCode = U_ZERO_ERROR;
                minFractionDigits = getInt64Value(number.locale, opt, localErrorCode);
                if (U_FAILURE(localErrorCode)) {
                    // Bad option => formatting error
                    status = U_FORMATTING_ERROR;
                }
            }
            nf = number::NumberFormatter::with()
                .precision(number::Precision::minMaxFraction(minFractionDigits, maxFractionDigits));
            if (number.usePercent(opts)) {
                nf = nf.unit(NoUnit::percent());
            }
        }
    }
    return number::LocalizedNumberFormatter(nf.locale(number.locale));
}

Formatter* StandardFunctions::NumberFactory::createFormatter(const Locale& locale, UErrorCode& errorCode) {
    NULL_ON_ERROR(errorCode);

    Formatter* result = new Number(locale);
    if (result == nullptr) {
        errorCode = U_MEMORY_ALLOCATION_ERROR;
    }
    return result;
}

Formatter* StandardFunctions::IntegerFactory::createFormatter(const Locale& locale, UErrorCode& errorCode) {
    NULL_ON_ERROR(errorCode);

    Formatter* result = new Number(Number::integer(locale));
    if (result == nullptr) {
        errorCode = U_MEMORY_ALLOCATION_ERROR;
    }
    return result;
}

StandardFunctions::IntegerFactory::~IntegerFactory() {}

static FormattedPlaceholder notANumber(const FormattedPlaceholder& input) {
    return FormattedPlaceholder(input, FormattedValue(UnicodeString("NaN")));
}

static FormattedPlaceholder stringAsNumber(const Locale& locale, const number::LocalizedNumberFormatter& nf, const FormattedPlaceholder& input, UErrorCode& errorCode) {
    if (U_FAILURE(errorCode)) {
        return {};
    }

    double numberValue;
    // Copying string to avoid GCC dangling-reference warning
    // (although the reference is safe)
    UnicodeString inputStr = input.asFormattable().getString(errorCode);
    // Precondition: `input`'s source Formattable has type string
    if (U_FAILURE(errorCode)) {
        return {};
    }
    UErrorCode localErrorCode = U_ZERO_ERROR;
    strToDouble(inputStr, locale, numberValue, localErrorCode);
    if (U_FAILURE(localErrorCode)) {
        errorCode = U_OPERAND_MISMATCH_ERROR;
        return notANumber(input);
    }
    UErrorCode savedStatus = errorCode;
    number::FormattedNumber result = nf.formatDouble(numberValue, errorCode);
    // Ignore U_USING_DEFAULT_WARNING
    if (errorCode == U_USING_DEFAULT_WARNING) {
        errorCode = savedStatus;
    }
    return FormattedPlaceholder(input, FormattedValue(std::move(result)));
}

int32_t StandardFunctions::Number::maximumFractionDigits(const FunctionOptions& opts) const {
    Formattable opt;

    if (isInteger) {
        return 0;
    }

    if (opts.getFunctionOption(UnicodeString("maximumFractionDigits"), opt)) {
        UErrorCode localErrorCode = U_ZERO_ERROR;
        int64_t val = getInt64Value(locale, opt, localErrorCode);
        if (U_SUCCESS(localErrorCode)) {
            return static_cast<int32_t>(val);
        }
    }
    return number::impl::kMaxIntFracSig;
}

bool StandardFunctions::Number::usePercent(const FunctionOptions& opts) const {
    Formattable opt;
    if (isInteger
        || !opts.getFunctionOption(UnicodeString("style"), opt)
        || opt.getType() != UFMT_STRING) {
        return false;
    }
    UErrorCode localErrorCode = U_ZERO_ERROR;
    const UnicodeString& style = opt.getString(localErrorCode);
    U_ASSERT(U_SUCCESS(localErrorCode));
    return (style == UnicodeString("percent"));
}

/* static */ StandardFunctions::Number StandardFunctions::Number::integer(const Locale& loc) {
    return StandardFunctions::Number(loc, true);
}

FormattedPlaceholder StandardFunctions::Number::format(FormattedPlaceholder&& arg, FunctionOptions&& opts, UErrorCode& errorCode) const {
    if (U_FAILURE(errorCode)) {
        return {};
    }

    // No argument => return "NaN"
    if (!arg.canFormat()) {
        errorCode = U_OPERAND_MISMATCH_ERROR;
        return notANumber(arg);
    }

    number::LocalizedNumberFormatter realFormatter;
    realFormatter = formatterForOptions(*this, opts, errorCode);

    number::FormattedNumber numberResult;
    if (U_SUCCESS(errorCode)) {
        // Already checked that contents can be formatted
        const Formattable& toFormat = arg.asFormattable();
        switch (toFormat.getType()) {
        case UFMT_DOUBLE: {
            double d = toFormat.getDouble(errorCode);
            U_ASSERT(U_SUCCESS(errorCode));
            numberResult = realFormatter.formatDouble(d, errorCode);
            break;
        }
        case UFMT_LONG: {
            int32_t l = toFormat.getLong(errorCode);
            U_ASSERT(U_SUCCESS(errorCode));
            numberResult = realFormatter.formatInt(l, errorCode);
            break;
        }
        case UFMT_INT64: {
            int64_t i = toFormat.getInt64(errorCode);
            U_ASSERT(U_SUCCESS(errorCode));
            numberResult = realFormatter.formatInt(i, errorCode);
            break;
        }
        case UFMT_STRING: {
            // Try to parse the string as a number
            return stringAsNumber(locale, realFormatter, arg, errorCode);
        }
        default: {
            // Other types can't be parsed as a number
            errorCode = U_OPERAND_MISMATCH_ERROR;
            return notANumber(arg);
        }
        }
    }

    return FormattedPlaceholder(arg, FormattedValue(std::move(numberResult)));
}

StandardFunctions::Number::~Number() {}
StandardFunctions::NumberFactory::~NumberFactory() {}

// --------- PluralFactory


StandardFunctions::Plural::PluralType StandardFunctions::Plural::pluralType(const FunctionOptions& opts) const {
    Formattable opt;

    if (opts.getFunctionOption(UnicodeString("select"), opt)) {
        UErrorCode localErrorCode = U_ZERO_ERROR;
        UnicodeString val = opt.getString(localErrorCode);
        if (U_SUCCESS(localErrorCode)) {
            if (val == UnicodeString("ordinal")) {
                return PluralType::PLURAL_ORDINAL;
            }
            if (val == UnicodeString("exact")) {
                return PluralType::PLURAL_EXACT;
            }
        }
    }
    return PluralType::PLURAL_CARDINAL;
}

Selector* StandardFunctions::PluralFactory::createSelector(const Locale& locale, UErrorCode& errorCode) const {
    NULL_ON_ERROR(errorCode);

    Selector* result;
    if (isInteger) {
        result = new Plural(Plural::integer(locale));
    } else {
        result = new Plural(locale);
    }
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

static UnicodeString toJSONString(double d) {
    // TODO :(
    char buffer[512];
    // "Only integer matching is required in the Technical Preview."
    snprintf(buffer, 512, "%li", static_cast<int64_t>(d));
    return UnicodeString(buffer);
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
    // TODO: This needs to be checked against https://github.com/unicode-org/message-format-wg/blob/main/spec/registry.md#number-selection
    // Determine `exact`, per step 1 under "Number Selection"
    UnicodeString exact = toJSONString(valToCheck);

    // Generate the matches
    // -----------------------

    prefsLen = 0;

    // First, check for an exact match
    double keyAsDouble = 0;
    for (int32_t i = 0; i < keysLen; i++) {
        // Try parsing the key as a double
        UErrorCode localErrorCode = U_ZERO_ERROR;
        strToDouble(keys[i], locale, keyAsDouble, localErrorCode);
        if (U_SUCCESS(localErrorCode)) {
            if (exact == keys[i]) {
		prefs[prefsLen] = keys[i];
                prefsLen++;
                break;
            }
        }
    }

    PluralType type = pluralType(opts);
    // Return immediately if exact matching was requested
    if (prefsLen == keysLen || type == PluralType::PLURAL_EXACT) {
        return;
    }

    UPluralType t = type == PluralType::PLURAL_ORDINAL ? UPLURAL_TYPE_ORDINAL : UPLURAL_TYPE_CARDINAL;
    // Look up plural rules by locale and type
    LocalPointer<PluralRules> rules(PluralRules::forLocale(locale, t, errorCode));
    CHECK_ERROR(errorCode);


    // Check for a match based on the plural category
    UnicodeString match;
    if (isFormattedNumber) {
        match = rules->select(toFormat.output().getNumber(), errorCode);
    } else {
        if (isInteger) {
            match = rules->select(static_cast<int32_t>(trunc(valToCheck)));
        } else {
            match = rules->select(valToCheck);
        }
    }
    CHECK_ERROR(errorCode);

    for (int32_t i = 0; i < keysLen; i ++) {
        if (prefsLen >= keysLen) {
            break;
        }
        if (match == keys[i]) {
            prefs[prefsLen] = keys[i];
            prefsLen++;
        }
    }
}

StandardFunctions::Plural::~Plural() {}
StandardFunctions::PluralFactory::~PluralFactory() {}

// --------- DateTimeFactory

/* static */ UnicodeString StandardFunctions::getStringOption(const FunctionOptions& opts,
                                                              const UnicodeString& optionName,
                                                              UErrorCode& errorCode) {
    if (U_SUCCESS(errorCode)) {
        Formattable opt;
        if (opts.getFunctionOption(optionName, opt)) {
            return opt.getString(errorCode); // In case it's not a string, error code will be set
        } else {
            errorCode = U_ILLEGAL_ARGUMENT_ERROR;
        }
    }
    // Default is empty string
    return {};
}

// Date/time options only
static UnicodeString defaultForOption(const UnicodeString& optionName) {
    if (optionName == UnicodeString("dateStyle")
        || optionName == UnicodeString("timeStyle")
        || optionName == UnicodeString("style")) {
        return UnicodeString("short");
    }
    return {}; // Empty string is default
}

// TODO
// Only DateTime currently uses the function options stored in the placeholder.
// It also doesn't use them very consistently (it looks at the previous set of options,
// and others aren't preserved). This needs to be generalized,
// but that depends on https://github.com/unicode-org/message-format-wg/issues/515
// Finally, the option value is assumed to be a string,
// which works for datetime options but not necessarily in general.
UnicodeString StandardFunctions::DateTime::getFunctionOption(const FormattedPlaceholder& toFormat,
                                                             const FunctionOptions& opts,
                                                             const UnicodeString& optionName) const {
    // Options passed to the current function invocation take priority
    Formattable opt;
    UnicodeString s;
    UErrorCode localErrorCode = U_ZERO_ERROR;
    s = getStringOption(opts, optionName, localErrorCode);
    if (U_SUCCESS(localErrorCode)) {
        return s;
    }
    // Next try the set of options used to construct `toFormat`
    localErrorCode = U_ZERO_ERROR;
    s = getStringOption(toFormat.options(), optionName, localErrorCode);
    if (U_SUCCESS(localErrorCode)) {
        return s;
    }
    // Finally, use default
    return defaultForOption(optionName);
}

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

/* static */ StandardFunctions::DateTimeFactory* StandardFunctions::DateTimeFactory::dateTime(UErrorCode& errorCode) {
    NULL_ON_ERROR(errorCode);

    DateTimeFactory* result = new StandardFunctions::DateTimeFactory(DateTimeType::DateTime);
    if (result == nullptr) {
        errorCode = U_MEMORY_ALLOCATION_ERROR;
    }
    return result;
}

/* static */ StandardFunctions::DateTimeFactory* StandardFunctions::DateTimeFactory::date(UErrorCode& errorCode) {
    NULL_ON_ERROR(errorCode);

    DateTimeFactory* result = new DateTimeFactory(DateTimeType::Date);
    if (result == nullptr) {
        errorCode = U_MEMORY_ALLOCATION_ERROR;
    }
    return result;
}

/* static */ StandardFunctions::DateTimeFactory* StandardFunctions::DateTimeFactory::time(UErrorCode& errorCode) {
    NULL_ON_ERROR(errorCode);

    DateTimeFactory* result = new DateTimeFactory(DateTimeType::Time);
    if (result == nullptr) {
        errorCode = U_MEMORY_ALLOCATION_ERROR;
    }
    return result;
}

Formatter* StandardFunctions::DateTimeFactory::createFormatter(const Locale& locale, UErrorCode& errorCode) {
    NULL_ON_ERROR(errorCode);

    Formatter* result = new StandardFunctions::DateTime(locale, type);
    if (result == nullptr) {
        errorCode = U_MEMORY_ALLOCATION_ERROR;
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
        errorCode = U_OPERAND_MISMATCH_ERROR;
        return std::move(toFormat);
    }

    LocalPointer<DateFormat> df;
    Formattable opt;

    DateFormat::EStyle dateStyle = DateFormat::kShort;
    DateFormat::EStyle timeStyle = DateFormat::kShort;

    // Extract style options
    if (type == DateTimeFactory::DateTimeType::DateTime) {
        dateStyle = stringToStyle(getFunctionOption(toFormat, opts, UnicodeString("dateStyle")), errorCode);
        timeStyle = stringToStyle(getFunctionOption(toFormat, opts, UnicodeString("timeStyle")), errorCode);
        if (dateStyle == DateFormat::NONE && timeStyle == DateFormat::NONE) {
            df.adoptInstead(defaultDateTimeInstance(locale, errorCode));
        } else {
            df.adoptInstead(DateFormat::createDateTimeInstance(dateStyle, timeStyle, locale));
        }
    } else if (type == DateTimeFactory::DateTimeType::Date) {
        dateStyle = stringToStyle(getFunctionOption(toFormat, opts, UnicodeString("style")), errorCode);
        df.adoptInstead(DateFormat::createDateInstance(dateStyle, locale));
    } else {
        // :time
        timeStyle = stringToStyle(getFunctionOption(toFormat, opts, UnicodeString("style")), errorCode);
        df.adoptInstead(DateFormat::createTimeInstance(timeStyle, locale));
    }

    if (U_FAILURE(errorCode)) {
        return {};
    }
    if (!df.isValid()) {
        errorCode = U_MEMORY_ALLOCATION_ERROR;
        return {};
    }

    UnicodeString result;
    const Formattable& source = toFormat.asFormattable();
    switch (source.getType()) {
    case UFMT_STRING: {
        const UnicodeString& sourceStr = source.getString(errorCode);
        U_ASSERT(U_SUCCESS(errorCode));
        // Pattern for ISO 8601 format - datetime
        UnicodeString pattern("Y-M-d'T'H:m:sZZZZZ");
        LocalPointer<DateFormat> dateParser(new SimpleDateFormat(pattern, errorCode));
        if (U_FAILURE(errorCode)) {
            errorCode = U_FORMATTING_ERROR;
        } else {
            // Parse the date
            UDate d = dateParser->parse(sourceStr, errorCode);
            if (U_FAILURE(errorCode)) {
                // Pattern for ISO 8601 format - date
                UnicodeString pattern("YYYY-MM-dd");
                errorCode = U_ZERO_ERROR;
                dateParser.adoptInstead(new SimpleDateFormat(pattern, errorCode));
                if (U_FAILURE(errorCode)) {
                    errorCode = U_FORMATTING_ERROR;
                } else {
                    d = dateParser->parse(sourceStr, errorCode);
                    if (U_FAILURE(errorCode)) {
                        errorCode = U_OPERAND_MISMATCH_ERROR;
                    }
                }
            }
            // Use the parsed date as the source value
            // in the returned FormattedPlaceholder; this is necessary
            // so the date can be re-formatted
            toFormat = FormattedPlaceholder(message2::Formattable::forDate(d),
                                            toFormat.getFallback());
            df->format(d, result, 0, errorCode);
            break;
        }
    }
    case UFMT_DATE: {
        df->format(source.asICUFormattable(errorCode), result, 0, errorCode);
        if (U_FAILURE(errorCode)) {
            if (errorCode == U_ILLEGAL_ARGUMENT_ERROR) {
                errorCode = U_OPERAND_MISMATCH_ERROR;
            }
        }
        break;
    }
    // Any other cases are an error
    default: {
        errorCode = U_OPERAND_MISMATCH_ERROR;
        break;
    }
    }
    if (U_FAILURE(errorCode)) {
        return {};
    }
    return FormattedPlaceholder(toFormat, std::move(opts), FormattedValue(std::move(result)));
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

} // namespace message2
U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */

