// © 2024 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#include "unicode/utypes.h"

#ifndef U_HIDE_DEPRECATED_API

#ifndef MESSAGEFORMAT2_FUNCTION_REGISTRY_INTERNAL_H
#define MESSAGEFORMAT2_FUNCTION_REGISTRY_INTERNAL_H

#if U_SHOW_CPLUSPLUS_API

#if !UCONFIG_NO_FORMATTING

#if !UCONFIG_NO_MF2

#include "unicode/datefmt.h"
#include "unicode/messageformat2_function_registry.h"

U_NAMESPACE_BEGIN

namespace message2 {

    // Built-in functions
    /*
      The standard functions are :datetime, :date, :time,
      :number, :integer, and :string,
      per https://github.com/unicode-org/message-format-wg/blob/main/spec/registry.md
      as of https://github.com/unicode-org/message-format-wg/releases/tag/LDML45-alpha
    */
    class StandardFunctions {
        friend class MessageFormatter;

        class DateTime;
        class DateTimeValue;

        class DateTimeFactory : public FunctionFactory {
        public:
            Function* createFunction(const Locale& locale, UErrorCode& status) override;
            static DateTimeFactory* date(UErrorCode&);
            static DateTimeFactory* time(UErrorCode&);
            static DateTimeFactory* dateTime(UErrorCode&);
            DateTimeFactory() = delete;
            virtual ~DateTimeFactory();

        private:
            friend class DateTime;
            friend class DateTimeValue;

            typedef enum DateTimeType {
                Date,
                Time,
                DateTime
            } DateTimeType;

            DateTimeType type;
            DateTimeFactory(DateTimeType t) : type(t) {}
        };

        class DateTime : public Function {
        public:
            FunctionValue* call(FunctionValue* operand, FunctionOptions&& options) override;
            virtual ~DateTime();

        private:
            const Locale& locale;
            const DateTimeFactory::DateTimeType type;
            friend class DateTimeFactory;
            DateTime(const Locale& l, DateTimeFactory::DateTimeType t) : locale(l), type(t) {}
            const LocalPointer<icu::DateFormat> icuFormatter;
        };

        class Number;
        class NumberValue;

        class NumberFactory : public FunctionFactory {
        public:
            Function* createFunction(const Locale& locale, UErrorCode& status) override;
            static FunctionFactory* integerFactory(UErrorCode& status);
            NumberFactory();
            virtual ~NumberFactory();
        private:
            friend class Number;
            friend class NumberValue;

            typedef enum PluralType {
                PLURAL_ORDINAL,
                PLURAL_CARDINAL,
                PLURAL_EXACT
            } PluralType;

            NumberFactory(bool, PluralType);
            const Locale& locale;
            const bool isInteger = false;
        };

        class Number : public Function {
        public:
            FunctionValue* call(FunctionValue* operand, FunctionOptions&& options) override;
            virtual ~Number();

        private:
            friend class NumberFactory;
            friend class NumberValue;
            friend class StandardFunctions;

            Number(const Locale& loc) : locale(loc), icuFormatter(number::NumberFormatter::withLocale(loc)) {}
            Number(const Locale& loc, bool isInt) : locale(loc), isInteger(isInt), icuFormatter(number::NumberFormatter::withLocale(loc)) {}
            static Number integer(const Locale& loc);

        // These options have their own accessor methods, since they have different default values.
            int32_t digitSizeOption(const FunctionOptions&, const UnicodeString&) const;
            int32_t maximumFractionDigits(const FunctionOptions& options) const;
            int32_t minimumFractionDigits(const FunctionOptions& options) const;
            int32_t minimumSignificantDigits(const FunctionOptions& options) const;
            int32_t maximumSignificantDigits(const FunctionOptions& options) const;
            int32_t minimumIntegerDigits(const FunctionOptions& options) const;

            bool usePercent(const FunctionOptions& options) const;
            const Locale& locale;
            const bool isInteger = false;
            const number::LocalizedNumberFormatter icuFormatter;

            static NumberFactory::PluralType pluralType(const FunctionOptions& opts);
        };

        static number::LocalizedNumberFormatter formatterForOptions(const Number& number,
                                                                    const FunctionOptions& opts,
                                                                    UErrorCode& status);


        class NumberValue : public FunctionValue {
        public:
            UnicodeString formatToString(UErrorCode&) const;
            const Formattable& getOperand() const;
            const FunctionOptions& getResolvedOptions() const;
            void selectKeys(const UnicodeString* keys,
                            int32_t keysLen,
                            UnicodeString* prefs,
                            int32_t& prefsLen,
                            UErrorCode& status);
            NumberValue();
            virtual ~NumberValue();
        private:
            Locale locale;
            Formattable operand;
            FunctionOptions opts;
            number::FormattedNumber formattedNumber;
            NumberValue(const Number&, const Locale&, FunctionValue*, FunctionOptions&&, UErrorCode&);
        }; // class NumberValue

        class DateTimeValue : public FunctionValue {
        public:
            UnicodeString formatToString(UErrorCode&) const;
            const Formattable& getOperand() const;
            const FunctionOptions& getResolvedOptions() const;
            void selectKeys(const UnicodeString* keys,
                            int32_t keysLen,
                            UnicodeString* prefs,
                            int32_t& prefsLen,
                            UErrorCode& status);
            DateTimeValue();
            virtual ~DateTimeValue();
        private:
            Locale locale;
            Formattable operand;
            FunctionOptions opts;
            UDate date;
            UnicodeString formattedDate;
            DateTimeValue(const Locale&, DateTimeFactory::DateTimeType type,
                          FunctionValue*, FunctionOptions&&, UErrorCode&);
        }; // class DateTimeValue

        class StringFactory : public FunctionFactory {
        public:
            Function* createFunction(const Locale& locale, UErrorCode& status) override;
            virtual ~StringFactory();
        };

        class String : public Function {
        public:
            FunctionValue* call(FunctionValue* val, FunctionOptions&& opts) override;
            virtual ~String();

        private:
            friend class StringFactory;

            // Formatting `value` to a string might require the locale
            const Locale& locale;

            String(const Locale& l) : locale(l) {}
        };

        class StringValue : public FunctionValue {
        public:
            UnicodeString formatToString(UErrorCode&) const;
            const Formattable& getOperand() const;
            const FunctionOptions& getResolvedOptions() const;
            void selectKeys(const UnicodeString* keys,
                            int32_t keysLen,
                            UnicodeString* prefs,
                            int32_t& prefsLen,
                            UErrorCode& status);
            StringValue();
            virtual ~StringValue();
        private:
            Formattable operand;
            FunctionOptions opts;
            UnicodeString formattedString;
            StringValue(const Locale&, FunctionValue*, FunctionOptions&&, UErrorCode&);
        }; // class StringValue

    };

    extern void formatDateWithDefaults(const Locale& locale, UDate date, UnicodeString&, UErrorCode& errorCode);
    extern number::FormattedNumber formatNumberWithDefaults(const Locale& locale, double toFormat, UErrorCode& errorCode);
    extern number::FormattedNumber formatNumberWithDefaults(const Locale& locale, int32_t toFormat, UErrorCode& errorCode);
    extern number::FormattedNumber formatNumberWithDefaults(const Locale& locale, int64_t toFormat, UErrorCode& errorCode);
    extern number::FormattedNumber formatNumberWithDefaults(const Locale& locale, StringPiece toFormat, UErrorCode& errorCode);
    extern DateFormat* defaultDateTimeInstance(const Locale&, UErrorCode&);

} // namespace message2

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_MF2 */

#endif /* #if !UCONFIG_NO_FORMATTING */

#endif /* U_SHOW_CPLUSPLUS_API */

#endif // MESSAGEFORMAT2_FUNCTION_REGISTRY_INTERNAL_H

#endif // U_HIDE_DEPRECATED_API
// eof
