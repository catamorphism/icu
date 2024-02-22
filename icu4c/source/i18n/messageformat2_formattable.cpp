// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/messageformat2_formattable.h"
#include "unicode/smpdtfmt.h"
#include "messageformat2_macros.h"

U_NAMESPACE_BEGIN

namespace message2 {

    // Fallback values are enclosed in curly braces;
    // see https://github.com/unicode-org/message-format-wg/blob/main/spec/formatting.md#formatting-fallback-values

    static UnicodeString fallbackToString(const UnicodeString& s) {
        UnicodeString result;
        result += LEFT_CURLY_BRACE;
        result += s;
        result += RIGHT_CURLY_BRACE;
        return result;
    }

    Formattable& Formattable::operator=(Formattable other) noexcept {
        swap(*this, other);
        return *this;
    }

    Formattable::Formattable(const Formattable& other) {
        type = other.type;
        isDecimal = other.isDecimal;

        U_ASSERT(type != UFMT_COUNT);

        switch (type) {
        case UFMT_DATE:
        case UFMT_DOUBLE:
        case UFMT_LONG:
        case UFMT_INT64: {
            scalar = other.scalar;
            if (other.isDecimal) {
                icuFormattable = other.icuFormattable;
            }
            break;
        }
        case UFMT_STRING: {
            fString = other.fString;
            break;
        }
        case UFMT_ARRAY: {
            array = other.array;
            arrayLen = other.arrayLen;
            break;
        }
        case UFMT_OBJECT: {
            object = other.object;
            break;
        }
        default: {
            break;
        }
        }
    }

    Formattable::Formattable(StringPiece number, UErrorCode &status) {
        CHECK_ERROR(status);

        isDecimal = true;
        icuFormattable = icu::Formattable(number, status);
        switch (icuFormattable.getType()) {
        case icu::Formattable::Type::kLong: {
            type = UFMT_LONG;
            break;
        }
        case icu::Formattable::Type::kInt64: {
            type = UFMT_INT64;
            break;
        }
        case icu::Formattable::Type::kDouble: {
            type = UFMT_DOUBLE;
            break;
        }
        default: {
            U_ASSERT(false); // Decimals should have a numeric type
            status = U_INVALID_STATE_ERROR;
            break;
        }
        }
    }

    UFormattableType Formattable::getType() const {
        return type;
    }

    const Formattable* Formattable::getArray(int32_t& len) const {
        U_ASSERT(type == UFMT_ARRAY);
        U_ASSERT(array != nullptr);
        len = arrayLen;
        return array;
    }

    int64_t Formattable::getInt64(UErrorCode& status) const {
        if (isDecimal && isNumeric()) {
            return icuFormattable.getInt64(status);
        }

        switch (type) {
        case UFMT_LONG:
        case UFMT_INT64: {
            return scalar.fInt64;
        }
        case UFMT_DOUBLE: {
            return (icu::Formattable(scalar.fDouble)).getInt64(status);
        }
        default: {
            status = U_INVALID_FORMAT_ERROR;
            return 0;
        }
        }
    }

    icu::Formattable Formattable::asICUFormattable(UErrorCode& status) const {
        if (U_FAILURE(status)) {
            return {};
        }
        // Type must not be UFMT_ARRAY or UFMT_OBJECT
        if (type == UFMT_ARRAY || type == UFMT_OBJECT) {
            status = U_ILLEGAL_ARGUMENT_ERROR;
            return {};
        }

        if (isDecimal) {
            return icuFormattable;
        }

        switch (type) {
        case UFMT_DATE: {
            return icu::Formattable(scalar.fDate, icu::Formattable::kIsDate);
        }
        case UFMT_DOUBLE: {
            return icu::Formattable(scalar.fDouble);
        }
        case UFMT_LONG: {
            return icu::Formattable((int32_t) scalar.fInt64);
        }
        case UFMT_INT64: {
            return icu::Formattable(scalar.fInt64);
        }
        case UFMT_STRING: {
            return icu::Formattable(fString);
        }
        default: {
            // Already checked for UFMT_ARRAY and UFMT_OBJECT
            return icu::Formattable();
        }
        }
    }

    Formattable::~Formattable() {}

    FormattableObject::~FormattableObject() {}

    FormattedMessage::~FormattedMessage() {}

    FormattedValue::FormattedValue(const UnicodeString& s) {
        type = kString;
        stringOutput = std::move(s);
    }

    FormattedValue::FormattedValue(number::FormattedNumber&& n) {
        type = kNumber;
        numberOutput = std::move(n);
    }

    FormattedValue& FormattedValue::operator=(FormattedValue&& other) noexcept {
        type = other.type;
        if (type == kString) {
            stringOutput = std::move(other.stringOutput);
        } else {
            numberOutput = std::move(other.numberOutput);
        }
        return *this;
    }

    FormattedPlaceholder& FormattedPlaceholder::operator=(FormattedPlaceholder&& other) noexcept {
        type = other.type;
        source = other.source;
        if (type == kEvaluated) {
            formatted = std::move(other.formatted);
        }
        fallback = other.fallback;
        return *this;
    }

    Formattable FormattedPlaceholder::asFormattable() const {
        if (type == kEvaluated && formatted.isString()) {
            return Formattable(formatted.getString());
        }
        if (type == kFallback) {
            return Formattable(fallback);
        } else {
            return source;
        }
    }

    // Default formatters
    // ------------------

    number::FormattedNumber formatNumberWithDefaults(const Locale& locale, double toFormat, UErrorCode& errorCode) {
        return number::NumberFormatter::withLocale(locale).formatDouble(toFormat, errorCode);
    }

    number::FormattedNumber formatNumberWithDefaults(const Locale& locale, int32_t toFormat, UErrorCode& errorCode) {
        return number::NumberFormatter::withLocale(locale).formatInt(toFormat, errorCode);
    }

    number::FormattedNumber formatNumberWithDefaults(const Locale& locale, int64_t toFormat, UErrorCode& errorCode) {
        return number::NumberFormatter::withLocale(locale).formatInt(toFormat, errorCode);
    }

    number::FormattedNumber formatNumberWithDefaults(const Locale& locale, StringPiece toFormat, UErrorCode& errorCode) {
        return number::NumberFormatter::withLocale(locale).formatDecimal(toFormat, errorCode);
    }

    DateFormat* defaultDateTimeInstance(const Locale& locale, UErrorCode& errorCode) {
        NULL_ON_ERROR(errorCode);
        LocalPointer<DateFormat> df(DateFormat::createDateTimeInstance(DateFormat::SHORT, DateFormat::SHORT, locale));
        if (!df.isValid()) {
            errorCode = U_MEMORY_ALLOCATION_ERROR;
            return nullptr;
        }
        return df.orphan();
    }

    void formatDateWithDefaults(const Locale& locale, UDate date, UnicodeString& result, UErrorCode& errorCode) {
        CHECK_ERROR(errorCode);

        LocalPointer<DateFormat> df(defaultDateTimeInstance(locale, errorCode));
        CHECK_ERROR(errorCode);
        df->format(date, result, 0, errorCode);
    }

    // Called when output is required and the contents are an unevaluated `Formattable`;
    // formats the source `Formattable` to a string with defaults, if it can be
    // formatted with a default formatter
    static FormattedPlaceholder formatWithDefaults(const Locale& locale, const FormattedPlaceholder& input, UErrorCode& status) {
        if (U_FAILURE(status)) {
            return {};
        }

        const Formattable& toFormat = input.asFormattable();
        // Try as decimal number first
        if (toFormat.isNumeric()) {
            // Note: the ICU Formattable has to be created here since the StringPiece
            // refers to state inside the Formattable; so otherwise we'll have a reference
            // to a temporary object
            icu::Formattable icuFormattable = toFormat.asICUFormattable(status);
            StringPiece asDecimal = icuFormattable.getDecimalNumber(status);
            if (U_FAILURE(status)) {
                return {};
            }
            if (asDecimal != nullptr) {
                return FormattedPlaceholder(input, FormattedValue(formatNumberWithDefaults(locale, asDecimal, status)));
            }
        }

        UFormattableType type = toFormat.getType();
        switch (type) {
        case UFMT_DATE: {
            UnicodeString result;
            formatDateWithDefaults(locale, toFormat.getDate(), result, status);
            return FormattedPlaceholder(input, FormattedValue(std::move(result)));
        }
        case UFMT_DOUBLE: {
            return FormattedPlaceholder(input, FormattedValue(formatNumberWithDefaults(locale, toFormat.getDouble(), status)));
        }
        case UFMT_LONG: {
            return FormattedPlaceholder(input, FormattedValue(formatNumberWithDefaults(locale, toFormat.getLong(), status)));
        }
        case UFMT_INT64: {
            return FormattedPlaceholder(input, FormattedValue(formatNumberWithDefaults(locale, toFormat.getInt64(), status)));
        }
        case UFMT_STRING: {
            return FormattedPlaceholder(input, FormattedValue(UnicodeString(toFormat.getString())));
        }
        default: {
            // No default formatters for other types; use fallback
            status = U_FORMATTING_ERROR;
            // Note: it would be better to set an internal formatting error so that a string
            // (e.g. the type tag) can be provided. However, this  method is called by the
            // public method formatToString() and thus can't take a MessageContext
            return FormattedPlaceholder(input.getFallback());
        }
        }
    }

    // Called when string output is required; forces output to be produced
    // if none is present (including formatting number output as a string)
    UnicodeString FormattedPlaceholder::formatToString(const Locale& locale,
                                                       UErrorCode& status) const {
        if (U_FAILURE(status)) {
            return {};
        }
        if (isFallback() || isNullOperand()) {
            return fallbackToString(fallback);
        }

        // Evaluated value: either just return the string, or format the number
        // as a string and return it
        if (isEvaluated()) {
            if (formatted.isString()) {
                return formatted.getString();
            } else {
                return formatted.getNumber().toString(status);
            }
        }
        // Unevaluated value: first evaluate it fully, then format
        FormattedPlaceholder evaluated = formatWithDefaults(locale, *this, status);
        if (status == U_FORMATTING_ERROR) {
            U_ASSERT(evaluated.isFallback());
            return evaluated.getFallback();
        }
        return evaluated.formatToString(locale, status);
    }

} // namespace message2

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */
