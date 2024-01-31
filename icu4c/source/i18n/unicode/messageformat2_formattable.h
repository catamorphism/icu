// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#include "unicode/uconfig.h"

#ifndef U_HIDE_DEPRECATED_API

#ifndef MESSAGEFORMAT2_FORMATTABLE_H
#define MESSAGEFORMAT2_FORMATTABLE_H

#if U_SHOW_CPLUSPLUS_API

#if !UCONFIG_NO_FORMATTING

#include "unicode/chariter.h"
#include "unicode/fmtable.h"
#include "unicode/formattedvalue.h"
#include "unicode/numberformatter.h"
#include "unicode/unistr.h"

U_NAMESPACE_BEGIN

namespace message2 {

// Formattable
// ----------

    /**
     * `FormattableObject` is an abstract class that can be implemented in order to define
     * an arbitrary class that can be passed to a custom formatter or selector function.
     * To be passed in such a way, it must be wrapped in a `Formattable` object.
     *
     * @internal ICU 75.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    class U_I18N_API FormattableObject : public UObject {
    public:
        /**
         * Returns an arbitrary string representing the type of this object.
         * It's up to the implementor of this class, as well as the implementors
         * of any custom functions that rely on particular values of this tag
         * corresponding to particular classes that the object contents can be
         * downcast to, to ensure that the type tags are used soundly.
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        virtual const UnicodeString& tag() const = 0;
        /**
         * Destructor.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        virtual ~FormattableObject();
    }; // class FormattableObject

    /**
     * The `Formattable` class represents a typed value that can be formatted,
     * originating either from a message argument or a literal in the code.
     * ICU's Formattable class is not used in MessageFormat 2 because it's unsafe to copy an
     * icu::Formattable value that contains an object. (See ICU-20275).
     *
     * `Formattable` is immutable (not deeply immutable) and
     * is movable and copyable.
     * (Copying does not do a deep copy when the wrapped value is an array or
     * object. Likewise, while a pointer to a wrapped array or object is `const`,
     * the referents of the pointers may be mutated by other code.)
     *
     * @internal ICU 75.0 technology preview
     * @deprecated This API is for technology preview only.
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
         * If this has type kObject or kArray, then `status` is set to
         * U_ILLEGAL_ARGUMENT_ERROR.
         *
         * @param status Input/output error code.
         * @return An icu::Formattable value with the same value as this.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        icu::Formattable asICUFormattable(UErrorCode& status) const;
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

    /**
     * A `FormattedValue` represents the result of formatting a `message2::Formattable`.
     * It contains either a string or a formatted number. (More types could be added
     * in the future.)
     *
     * `FormattedValue` is immutable and movable. It is not copyable.
     *
     * @internal ICU 75.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    class U_I18N_API FormattedValue : public UObject {
    public:
        /**
         * Formatted string constructor.
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        explicit FormattedValue(const UnicodeString&);
        /**
         * Formatted number constructor.
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        explicit FormattedValue(number::FormattedNumber&&);
        /**
         * Default constructor. Leaves the FormattedValue in
         * a valid but undefined state.
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        FormattedValue() : type(kString) {}
        /**
         * Returns true iff this is a formatted string.
         *
         * @return True if and only if this value is a formatted string.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        bool isString() const { return type == kString; }
        /**
         * Returns true iff this is a formatted number.
         *
         * @return True if and only if this value is a formatted number.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        bool isNumber() const { return type == kNumber; }
        /**
         * Gets the string contents of this value. If !isString(), then
         * the result is undefined.
         * @return          A reference to a formatted string.
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        const UnicodeString& getString() const { return stringOutput; }
        /**
         * Gets the number contents of this value. If !isNumber(), then
         * the result is undefined.
         * @return          A reference to a formatted number.
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        const number::FormattedNumber& getNumber() const { return numberOutput; }
        /**
         * Move assignment operator:
         * The source FormattedValue will be left in a valid but undefined state.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        FormattedValue& operator=(FormattedValue&&) noexcept;
        /**
         * Move constructor:
         * The source FormattedValue will be left in a valid but undefined state.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        FormattedValue(FormattedValue&& other) { *this = std::move(other); }
    private:
        enum Type {
            kString,
            kNumber
        };
        Type type;
        UnicodeString stringOutput;
        number::FormattedNumber numberOutput;
    }; // class FormattedValue

    /**
     * A `FormattablePlaceholder` encapsulates an input value (a `message2::Formattable`)
     * together with an optional output value (a `message2::FormattedValue`).
     *  More information, such as source line/column numbers, could be added to the class
     * in the future.
     *
     * `FormattablePlaceholder` is immutable (not deeply immutable) and movable.
     * It is not copyable.
     *
     * @internal ICU 75.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    class U_I18N_API FormattedPlaceholder : public UObject {
    public:
        /**
         * Fallback constructor. Constructs a value that represents a formatting error,
         * without recording an input `Formattable` as the source.
         *
         * @param s An error string. (See the MessageFormat specification for details
         *        on fallback strings.)
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        explicit FormattedPlaceholder(const UnicodeString& s) : fallback(s), type(kFallback) {}
        /**
         * Constructor for fully formatted placeholders.
         *
         * @param input A `FormattedPlaceholder` containing the fallback string and source
         *        `Formattable` used to construct the formatted value.
         * @param output A `FormattedValue` representing the formatted output of `input`.
         *        Passed by move.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        FormattedPlaceholder(const FormattedPlaceholder& input, FormattedValue&& output)
            : fallback(input.fallback), source(input.source), formatted(std::move(output)), type(kEvaluated) {}
        /**
         * Constructor for unformatted placeholders.
         *
         * @param input A `Formattable` object.
         * @param fb Fallback string to use if an error occurs while formatting the input.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        FormattedPlaceholder(const Formattable& input, const UnicodeString& fb)
            : fallback(fb), source(input), type(kUnevaluated) {}
        /**
         * Default constructor. Leaves the FormattedPlaceholder in a
         * valid but undefined state.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        FormattedPlaceholder() : type(kNull) {}
        /**
         * Returns a `Formattable` value corresponding to this placeholder.
         * If this is a fallback value, a `Formattable` value wrapping the fallback string is returned.onverts the Formattable object to an ICU Formattable object.
         * The result is undefined if this is a null operand.
         * If this has no formatting output, then the source `Formattable` is returned.
         * If this has a string-typed `FormattedValue` output, then a `Formattable` wrapping the formatted string is returned.
         * If this has any other type of `FormattedValue` output, then the source `Formattable` is returned.
         *
         * @return A message2::Formattable value.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        message2::Formattable asFormattable() const;
        /**
         * Returns true iff this is a fallback placeholder.
         *
         * @return True if and only if this placeholder was constructed from a fallback string,
         *         with no `Formattable` source or formatting output.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        bool isFallback() const { return type == kFallback; }
        /**
         * Returns true iff this is a null placeholder.
         *
         * @return True if and only if this placeholder represents the absent argument to a formatter
         *         that was invoked without an argument.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        bool isNullOperand() const { return type == kNull; }
        /**
         * Returns true iff this has formatting output.
         *
         * @return True if and only if this was constructed from both an input `Formattable` and
         *         output `FormattedValue`.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        bool isEvaluated() const { return (type == kEvaluated); }
        /**
         * Returns true iff this represents a valid argument to the formatter.
         *
         * @return True if and only if this is neither the null argument nor a fallback placeholder.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        bool canFormat() const { return !(isFallback() || isNullOperand()); }
        /**
         * Gets the fallback value of this placeholder, to be used in its place if an error occurs while
         * formatting it.
         * @return          A reference to this placeholder's fallback string.
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        const UnicodeString& getFallback() const { return fallback; }
        /**
         * Returns the formatted output of this placeholder. The result is undefined if !isEvaluated().
         * @return          A fully formatted `FormattedPlaceholder`.
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        const FormattedValue& output() const { return formatted; }
        /**
         * Move assignment operator:
         * The source FormattedPlaceholder will be left in a valid but undefined state.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        FormattedPlaceholder& operator=(FormattedPlaceholder&&) noexcept;
        /**
         * Move constructor:
         * The source FormattedPlaceholder will be left in a valid but undefined state.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        FormattedPlaceholder(FormattedPlaceholder&& other) { *this = std::move(other); }
        /**
         * Formats this as a string, using defaults.  If this is
         * either the null operand or is a fallback value, the return value is the result of formatting the
         * fallback value (which is the default fallback string if this is the null operand).
         * If there is no formatted output and the input is object- or array-typed,
         * then the argument is treated as a fallback value, since there is no default formatter
         * for objects or arrays.
         *
         * @param locale The locale to use for formatting numbers or dates
         * @param status Input/output error code
         * @return The result of formatting this placeholder.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        UnicodeString formatToString(const Locale& locale,
                                     UErrorCode& status) const;

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

    /**
     * Not yet implemented: The result of a message formatting operation. Based on
     * ICU4J's FormattedMessage.java.
     *
     * The class will contain information allowing the result to be viewed as a string,
     * iterator, etc. (TBD)
     *
     * @internal ICU 75.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    class U_I18N_API FormattedMessage : public icu::FormattedValue {
    public:
        /**
         * Not yet implemented.
         *
         * @internal ICU 75 technology preview
         * @deprecated This API is for ICU internal use only.
         */
        FormattedMessage(UErrorCode& status) {
            if (U_SUCCESS(status)) {
                status = U_UNSUPPORTED_ERROR;
            }
        }
        /**
         * Not yet implemented.
         *
         * @internal ICU 75 technology preview
         * @deprecated This API is for ICU internal use only.
         */
        int32_t length(UErrorCode& status) const {
            if (U_SUCCESS(status)) {
                status = U_UNSUPPORTED_ERROR;
            }
            return -1;
        }
        /**
         * Not yet implemented.
         *
         * @internal ICU 75 technology preview
         * @deprecated This API is for ICU internal use only.
         */
        char16_t charAt(int32_t index, UErrorCode& status) const {
            (void) index;
            if (U_SUCCESS(status)) {
                status = U_UNSUPPORTED_ERROR;
            }
            return 0;
        }
        /**
         * Not yet implemented.
         *
         * @internal ICU 75 technology preview
         * @deprecated This API is for ICU internal use only.
         */
        StringPiece subSequence(int32_t start, int32_t end, UErrorCode& status) const {
            (void) start;
            (void) end;
            if (U_SUCCESS(status)) {
                status = U_UNSUPPORTED_ERROR;
            }
            return "";
        }
        /**
         * Not yet implemented.
         *
         * @internal ICU 75 technology preview
         * @deprecated This API is for ICU internal use only.
         */
        UnicodeString toString(UErrorCode& status) const override {
            if (U_SUCCESS(status)) {
                status = U_UNSUPPORTED_ERROR;
            }
            return {};
        }
        /**
         * Not yet implemented.
         *
         * @internal ICU 75 technology preview
         * @deprecated This API is for ICU internal use only.
         */
        UnicodeString toTempString(UErrorCode& status) const override {
            if (U_SUCCESS(status)) {
                status = U_UNSUPPORTED_ERROR;
            }
            return {};
        }
        /**
         * Not yet implemented.
         *
         * @internal ICU 75 technology preview
         * @deprecated This API is for ICU internal use only.
         */
        Appendable& appendTo(Appendable& appendable, UErrorCode& status) const override {
            if (U_SUCCESS(status)) {
                status = U_UNSUPPORTED_ERROR;
            }
            return appendable;
        }
        /**
         * Not yet implemented.
         *
         * @internal ICU 75 technology preview
         * @deprecated This API is for ICU internal use only.
         */
        UBool nextPosition(ConstrainedFieldPosition& cfpos, UErrorCode& status) const override {
            (void) cfpos;
            if (U_SUCCESS(status)) {
                status = U_UNSUPPORTED_ERROR;
            }
            return false;
        }
        /**
         * Not yet implemented.
         *
         * @internal ICU 75 technology preview
         * @deprecated This API is for ICU internal use only.
         */
        CharacterIterator* toCharacterIterator(UErrorCode& status) {
            if (U_SUCCESS(status)) {
                status = U_UNSUPPORTED_ERROR;
            }
            return nullptr;
        }
        /**
         * Destructor.
         *
         * @internal ICU 75 technology preview
         * @deprecated This API is for ICU internal use only.
         */
        virtual ~FormattedMessage();
    }; // class FormattedMessage

} // namespace message2

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */

#endif /* U_SHOW_CPLUSPLUS_API */

#endif // MESSAGEFORMAT2_FORMATTABLE_H

#endif // U_HIDE_DEPRECATED_API
// eof
