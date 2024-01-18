// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#ifndef U_HIDE_DEPRECATED_API

#ifndef MESSAGEFORMAT2_FORMATTABLE_H
#define MESSAGEFORMAT2_FORMATTABLE_H

#include "unicode/utypes.h"

#if U_SHOW_CPLUSPLUS_API

#if !UCONFIG_NO_FORMATTING

#include "unicode/fmtable.h"
#include "unicode/formattedvalue.h"
#include "unicode/numberformatter.h"
#include "unicode/unistr.h"
#include "messageformat2_macros.h"
#include "number_decimalquantity.h"

U_NAMESPACE_BEGIN

namespace message2 {

// Formattable
// ----------

    // TODO add doc comments here

    // Abstract class used to represent object variants in a
    // Formattable
    class U_I18N_API FormattableObject : public UObject {
    public:
        // Returns an arbitrary string representing the type of this object.
        // It's up to the implementor of this class, as well as the implementors
        // of any custom functions that rely on particular values of this tag
        // corresponding to particular classes that the object contents can be
        // downcast to, to ensure that the type tags are used soundly.
        virtual const UnicodeString& tag() const = 0;
        virtual ~FormattableObject();
    }; // class FormattableObject

    /*
      ICU's Formattable class is not used because it's unsafe to copy a
      Formattable value that contains an object.

      This class is immutable (not deeply immutable) and
      is movable and copyable; and allows for type-safe casting.
      (Copying does not do a deep copy of the object value, if applicable.)
     */
    class U_I18N_API Formattable : public UObject {
    public:

        /**
         * See icu::Formattable for explanation
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        enum ISDATE { kIsDate };

        /**
         * Selector for flavor of data type contained within a
         * Formattable object.  Formattable is a union of several
         * different types, and at any time contains exactly one type.
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        enum Type {
            /**
             * Selector indicating a UDate value.  Use getDate to retrieve
             * the value.
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            kDate,

            /**
             * Selector indicating a double value. Use getDouble to retrieve
             * the value.
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            kDouble,

            /**
             * Selector indicating a 32-bit integer value. Use getLong to retrieve
             * the value.
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            kLong,

            /**
             * Selector indicating a UnicodeString value.  Use getString
             * to retrieve the value.
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            kString,

            /**
             * Selector indicating a 64-bit integer value. Use getInt64 to retrieve
             * the value.
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            kInt64,

            /**
             * Selector indicating an array of Formattable values. Use getArray to
             * retrieve the value.
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            kArray,

            /**
             * Selector indicating a pointer to a FormattableObject value.  Use getObject to
             * retrieve the value.
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            kObject
        };

        /**
         * Gets the data type of this Formattable object.
         * @return    the data type of this Formattable object.
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        Type getType() const;

        /**
         * Gets the double value of this object. If this object is not of type
         * kDouble then the result is undefined.
         * @return    the double value of this object.
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        double getDouble() const {
            if (isDecimal && type == Type::kDouble) {
                return icuFormattable.getDouble();
            }
            return scalar.fDouble;
        }

        /**
         * Gets the long value of this object. If this object is not of type
         * kLong then the result is undefined.
         * @return    the long value of this object.
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        int32_t getLong() const {
            if (isDecimal && type == Type::kLong) {
                return icuFormattable.getLong();
            }
            return (int32_t) scalar.fInt64;
        }

        /**
         * Gets the int64 value of this object. If this object is not of type
         * kInt64 then the result is undefined.
         * @return    the int64 value of this object.
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        int64_t getInt64() const {
            if (isDecimal && type == Type::kInt64) {
                return icuFormattable.getInt64();
            }
            return scalar.fInt64;
        }

        /**
         * Gets the int64 value of this object. If this object is of a numeric
         * type and the magnitude is too large to fit in an int64, then
         * the maximum or minimum int64 value, as appropriate, is returned
         * and the status is set to U_INVALID_FORMAT_ERROR.  If the
         * magnitude fits in an int64, then a casting conversion is
         * performed, with truncation of any fractional part. If this object is
         * not a numeric type, then 0 is returned and
         * the status is set to U_INVALID_FORMAT_ERROR.
         * @param status the error code
         * @return    the int64 value of this object.
         * @stable ICU 3.0
         */
        int64_t         getInt64(UErrorCode& status) const;
        /**
         * Gets the string value of this object. If this object is not of type
         * kString then the result is undefined.
         * @return          A reference to the string value of this object.
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        const UnicodeString&  getString() const {
            return fString;
        }

        /**
         * Gets the Date value of this object. If this object is not of type
         * kDate then the result is undefined.
         * @return    the Date value of this object.
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        UDate getDate() const { return scalar.fDate; }

        /**
         * Returns true if the data type of this Formattable object
         * is kDouble
         * @return true if this is a pure numeric object
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        UBool isNumeric() const { return (type == kDouble || type == kLong || type == kInt64); }

        /**
         * Gets the array value and count of this object. If this object
         * is not of type kArray then the result is undefined.
         * @param count    fill-in with the count of this object.
         * @return         the array value of this object.
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        const Formattable* getArray(int32_t& count) const;

        /**
         * Returns a pointer to the FormattableObject contained within this
         * formattable, or nullptr if this object does not contain a FormattableObject.
         * @return a FormattableObject pointer, or nullptr
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        const FormattableObject* getObject() const {
            // Can't return a reference since FormattableObject
            // is an abstract class
            if (type != Type::kObject) {
                // TODO: should assert that if type is object, object is non-null
                return nullptr;
            }
            return object;
        }
        /**
         * Copy constructor.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        Formattable(const Formattable&);
        /**
         * Move constructor:
         * The source Formattable will be left in a valid but undefined state.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        Formattable(Formattable&&);
        /**
         * Copy assignment operator
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        Formattable& operator=(const Formattable&);
        /**
         * Move assignment operator:
         * The source Formattable will be left in a valid but undefined state.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        Formattable& operator=(Formattable&&) noexcept;
        /**
         * Default constructor. Leaves the Formattable in a
         * valid but undefined state.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        Formattable() : type(Type::kDouble) {
            scalar.fDouble = 0.0;
        }
        /**
         * String constructor.
         *
         * @param s A string to wrap as a Formattable.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        Formattable(const UnicodeString& s) : fString(s), type(Type::kString) {}
        /**
         * Double constructor.
         *
         * @param d A double value to wrap as a Formattable.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        Formattable(double d) {
            scalar.fDouble = d;
            type = Type::kDouble;
        }
        /**
         * Int64 constructor.
         *
         * @param i An int64 value to wrap as a Formattable.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        Formattable(int64_t i) {
            scalar.fInt64 = i;
            type = Type::kInt64;
        }
        /**
         * Date constructor.
         *
         * @param d A UDate value to wrap as a Formattable.
         * @param isDate Flag that specifies this argument should be treated as a UDate.
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        Formattable(UDate d, ISDATE isDate) {
            (void) isDate;
            scalar.fDate = d;
            type = Type::kDate;
        }
        /**
         * Creates a Formattable object of an appropriate numeric type from a
         * a decimal number in string form.  The Formattable will retain the
         * full precision of the input in decimal format, even when it exceeds
         * what can be represented by a double or int64_t.
         *
         * @param number  the unformatted (not localized) string representation
         *                     of the Decimal number.
         * @param status  the error code.  Possible errors include U_INVALID_FORMAT_ERROR
         *                if the format of the string does not conform to that of a
         *                decimal number.
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        Formattable(StringPiece number, UErrorCode &status);
        /**
         * Array constructor.
         *
         * @param arr An array of Formattables, which is adopted.
         * @param len The length of the array.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        Formattable(const Formattable* arr, int32_t len) : array(arr), arrayLen(len), type(Type::kArray) {}
        /**
         * Object constructor.
         *
         * @param obj A FormattableObject (not adopted).
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        Formattable(const FormattableObject* obj) : object(obj), type(Type::kObject) {}
        /**
         * Destructor.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        virtual ~Formattable();
        /**
         * Converts the Formattable object to an ICU Formattable object.
         * It must not have type kObject or kArray.
         *
         * @return An icu::Formattable value with the same value as this.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        icu::Formattable asICUFormattable() const;
    private:

        // Ignored if type is kObject, kArray, or kString
        union FormattableContents {
            double          fDouble;
            int64_t         fInt64;
            UDate           fDate;
            bool            fDecimal;
        } scalar;

        // True iff this was constructed with the decimal constructor
        bool isDecimal = false;
        // Ignored unless isDecimal = true
        icu::Formattable icuFormattable;

        // Ignored if type != kString
        UnicodeString   fString;

        // Null if type != kObject; not owned
        const FormattableObject* object;

        // Null if type != kArray; not owned
        const Formattable* array;

        // Ignored if type != kArray
        int32_t arrayLen;

        Type type;
    }; // class Formattable

    // TODO doc comments
    // Encapsulates either a formatted string or formatted number;
    // more output types could be added in the future.
    class U_I18N_API FormattedValue : public UObject {
    public:
        explicit FormattedValue(const UnicodeString&);
        explicit FormattedValue(number::FormattedNumber&&);
        FormattedValue() : type(kString) {}
        bool isString() const { return type == kString; }
        bool isNumber() const { return type == kNumber; }
        // Precondition: type == kString
        const UnicodeString& getString() const { return stringOutput; }
        // Precondition: type == kNumber
        const number::FormattedNumber& getNumber() const { return numberOutput; }
    private:
        enum Type {
            kString,
            kNumber
        };
        Type type;
        UnicodeString stringOutput;
        number::FormattedNumber numberOutput;
    }; // class FormattedValue

    /*
      As in the equivalent ICU4J class, a FormattedPlaceholder encapsulates
      an input value (here, a `Formattable` to correspond to Java's `Object`)
      and an output value (here, our own `FormattedValue` class, different from
      the existing `FormattedValue` class of the same name as it can encapsulate
      either a string or number value.)
      More information, such as source line/column numbers, could be added in
      the future.
     */
    // TODO doc comments
    class U_I18N_API FormattedPlaceholder : public UObject {
    public:
        explicit FormattedPlaceholder(const UnicodeString& s) : fallback(s), type(kFallback) {} // Fallback constructor
        FormattedPlaceholder(FormattedValue&& v, const FormattedPlaceholder& f) : fallback(f.fallback),
                                                                                  source(f.source),
                                                                                  formatted(std::move(v)),
                                                                                  type(kEvaluated) {}
        FormattedPlaceholder() : type(kNull) {}
        FormattedPlaceholder(const Formattable& input, const UnicodeString& fb) : fallback(fb), source(input), type(kUnevaluated) {}
        message2::Formattable asFormattable() const;
        bool isFallback() const { return type == kFallback; }
        bool isNullOperand() const { return type == kNull; }
        bool isEvaluated() const { return (type == kEvaluated); }
        // Returns true if this is a valid argument to the formatter
        // (it's not null and is not a fallback value)
        bool canFormat() const { return !(isFallback() || isNullOperand()); }
        const UnicodeString& getFallback() const { return fallback; }
        FormattedPlaceholder promote() const {
            // Return a non-error value with string contents `fallback`
            return FormattedPlaceholder(FormattedValue(fallback), FormattedPlaceholder(Formattable(fallback), fallback));
        }
        // Precondition: type is evaluated
        const FormattedValue& output() const { return formatted; }
        FormattedPlaceholder(FormattedPlaceholder&&);
        FormattedPlaceholder& operator=(FormattedPlaceholder&&) noexcept;

        /**
         * Formats this as a string, using defaults.  If this is
         * either the null operand or is a fallback value, the return value is the result of formatting the
         * fallback value (which is the default fallback string if this is the null operand).
         * If this is object- or array-typed, then the argument is treated as a
         * fallback value, since there is no default formatter for objects or arrays.
         *
         * @param locale The locale to use for formatting numbers or dates
         * @param status Input/output error code
         * @return The result of formatting the input.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        UnicodeString formatToString(const Locale& locale, UErrorCode& status) const;

    private:
        friend class MessageFormatter;

        enum Type {
            kFallback,    // Represents the result of formatting that encountered an error
            kNull,        // Represents the absence of both an output and an input (not necessarily an error)
            kUnevaluated, // `source` should be valid, but there's no result yet
            kEvaluated,   // `formatted` exists
        };
        UnicodeString fallback;
        Formattable source;
        FormattedValue formatted;
        Type type;
    }; // class FormattedPlaceholder

} // namespace message2

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */

#endif /* U_SHOW_CPLUSPLUS_API */

#endif // MESSAGEFORMAT2_FORMATTABLE_H

#endif // U_HIDE_DEPRECATED_API
// eof
