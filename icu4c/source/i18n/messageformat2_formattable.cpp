// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/messageformat2_formattable.h"

U_NAMESPACE_BEGIN

namespace message2 {
    Formattable& Formattable::operator=(Formattable&& other) noexcept {
        type = other.type;
        isDecimal = other.isDecimal;

        switch (type) {
        case kDate:
        case kDouble:
        case kLong:
        case kInt64: {
            scalar = other.scalar;
            if (isDecimal) {
                icuFormattable = std::move(other.icuFormattable);
            }
            break;
        }
        case kString: {
            fString = std::move(other.fString);
            break;
        }
        case kArray: {
            array = other.array;
            arrayLen = other.arrayLen;
            break;
        }
        case kObject: {
            object = other.object;
            break;
        }
        }

        return *this;
    }

    Formattable& Formattable::operator=(const Formattable& other)  {
        if (this != &other) {
            type = other.type;
            isDecimal = other.isDecimal;

            switch (type) {
            case kDate:
            case kDouble:
            case kLong:
            case kInt64: {
                scalar = other.scalar;
                if (other.isDecimal) {
                    icuFormattable = other.icuFormattable;
                }
                break;
            }
            case kString: {
                fString = other.fString;
                break;
            }
            case kArray: {
                array = other.array;
                arrayLen = other.arrayLen;
                break;
            }
            case kObject: {
                object = other.object;
                break;
            }
            }
        }
        return *this;
    }

    Formattable::Formattable(const Formattable& other) {
        type = other.type;
        isDecimal = other.isDecimal;

        switch (type) {
        case kDate:
        case kDouble:
        case kLong:
        case kInt64: {
            scalar = other.scalar;
            if (other.isDecimal) {
                icuFormattable = other.icuFormattable;
            }
            break;
        }
        case kString: {
            fString = other.fString;
            break;
        }
        case kArray: {
            array = other.array;
            arrayLen = other.arrayLen;
            break;
        }
        case kObject: {
            object = other.object;
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
            type = Type::kLong;
            break;
        }
        case icu::Formattable::Type::kInt64: {
            type = Type::kInt64;
            break;
        }
        case icu::Formattable::Type::kDouble: {
            type = Type::kDouble;
            break;
        }
        default: {
            U_ASSERT(false); // Decimals should have a numeric type
            status = U_INVALID_STATE_ERROR;
            break;
        }
        }
        /*
        auto* dq = new number::impl::DecimalQuantity();
        if (dq == nullptr) {
            status = U_MEMORY_ALLOCATION_ERROR;
            bogus = true;
            return;
        }
        dq->setToDecNumber(number, status);
        if (dq->fitsInLong()) {
            scalar.fInt64 = dq->toLong();
            if (scalar.fInt64 <= INT32_MAX && scalar.fInt64 >= INT32_MIN) {
                type = Type::kLong;
            } else {
                type = Type::kInt64;
            }
        } else {
            type = Type::kDouble;
            scalar.fDouble = dq->toDouble();
        }
        decimalQuantity.adoptInstead(dq);
        */
    }

    Formattable::Type Formattable::getType() const {
        return type;
    }

    const Formattable* Formattable::getArray(int32_t& len) const {
        U_ASSERT(type == Type::kArray);
        U_ASSERT(array != nullptr);
        len = arrayLen;
        return array;
    }

    int64_t Formattable::getInt64(UErrorCode& status) const {
        if (isDecimal && isNumeric()) {
            return icuFormattable.getInt64(status);
        }

        switch (type) {
        case kLong:
        case kInt64: {
            return scalar.fInt64;
        }
        case kDouble: {
            return (icu::Formattable(scalar.fDouble)).getInt64(status);
        }
        default: {
            status = U_INVALID_FORMAT_ERROR;
            return 0;
        }
        }
    }

    icu::Formattable Formattable::asICUFormattable() const {
        // Type must not be kArray or kObject
        U_ASSERT(type != kArray && type != kObject);

        if (isDecimal) {
            return icuFormattable;
        }

        switch (type) {
        case kDate: {
            return icu::Formattable(scalar.fDate, icu::Formattable::kIsDate);
        }
        case kDouble: {
            return icu::Formattable(scalar.fDouble);
        }
        case kLong: {
            return icu::Formattable((int32_t) scalar.fInt64);
        }
        case kInt64: {
            return icu::Formattable(scalar.fInt64);
        }
        case kString: {
            return icu::Formattable(fString);
        }
        case kArray:
        case kObject: {
            // Already checked for
            return icu::Formattable();
        }
        }
    }

    Formattable::~Formattable() {}

    FormattableObject::~FormattableObject() {}

    FormattedValue::FormattedValue(UnicodeString&& s, const Formattable& input) {
        type = kStringValue;
        stringOutput = std::move(s);
        source = input;
    }

    FormattedValue::FormattedValue(number::FormattedNumber&& n, const Formattable& input) {
        type = kNumberValue;
        numberOutput = std::move(n);
        source = input;
    }

    FormattedValue::FormattedValue(FormattedValue&& other) {
        type = other.type;
        if (type == Type::kStringValue || type == Type::kFallbackValue) {
            stringOutput = std::move(other.stringOutput);
        } else if (isNumber()) {
            numberOutput = std::move(other.numberOutput);
        }
        source = other.source;
    }

    FormattedValue& FormattedValue::operator=(FormattedValue&& other) noexcept {
        type = other.type;
        if (type == kStringValue || type == kFallbackValue) {
            stringOutput = std::move(other.stringOutput);
        } else if (type == kNumberValue) {
            numberOutput = std::move(other.numberOutput);
        }
        source = other.source;

        return *this;
    }

    Formattable FormattedValue::asFormattable() const {
        if (type == kStringValue || type == kFallbackValue) {
            return Formattable(stringOutput);
        } else {
            return source;
        }
    }

} // namespace message2

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */
