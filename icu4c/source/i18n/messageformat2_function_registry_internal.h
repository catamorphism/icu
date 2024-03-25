// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#ifndef U_HIDE_DEPRECATED_API

#ifndef MESSAGEFORMAT2_FUNCTION_REGISTRY_INTERNAL_H
#define MESSAGEFORMAT2_FUNCTION_REGISTRY_INTERNAL_H

#if U_SHOW_CPLUSPLUS_API

#if !UCONFIG_NO_FORMATTING

#include "unicode/messageformat2_function_registry.h"

U_NAMESPACE_BEGIN

namespace message2 {

    // Built-in functions
    /*
      The standard functions are :datetime, :number,
      :identity, :plural, :selectordinal, :select, and :gender.
      Subject to change
    */
    class StandardFunctions {
        friend class MessageFormatter;

        static number::LocalizedNumberFormatter formatterForOptions(Locale locale, const FormattingContext& context, UErrorCode& status);

        class DateTimeFactory : public FormatterFactory {
        public:
            Formatter* createFormatter(const Locale& locale, UErrorCode& status) override;
            virtual ~DateTimeFactory();
        };

        class DateTime : public Formatter {
        public:
            FormattedValue format(FormattingContext& context, UErrorCode& status) const override;
            virtual ~DateTime();

        private:
            const Locale& locale;
            friend class DateTimeFactory;
            DateTime(const Locale& l) : locale(l) {}
            const LocalPointer<icu::DateFormat> icuFormatter;
        };

        class NumberFactory : public FormatterFactory {
        public:
            Formatter* createFormatter(const Locale& locale, UErrorCode& status) override;
            virtual ~NumberFactory();
        };

        class Number : public Formatter {
        public:
            FormattedValue format(FormattingContext& context, UErrorCode& status) const override;
            virtual ~Number();

        private:
            friend class NumberFactory;

            Number(const Locale& loc) : locale(loc), icuFormatter(number::NumberFormatter::withLocale(loc)) {}

            const Locale& locale;
            const number::LocalizedNumberFormatter icuFormatter;
        };

        class IdentityFactory : public FormatterFactory {
        public:
            Formatter* createFormatter(const Locale& locale, UErrorCode& status) override;
            virtual ~IdentityFactory();
        };

        class Identity : public Formatter {
        public:
            FormattedValue format(FormattingContext& context, UErrorCode& status) const override;
            virtual ~Identity();

        private:
            friend class IdentityFactory;

            const Locale& locale;
            Identity(const Locale& loc) : locale(loc) {}
        };

        class PluralFactory : public SelectorFactory {
        public:
            Selector* createSelector(const Locale& locale, UErrorCode& status) const override;
            virtual ~PluralFactory();

        private:
            friend class MessageFormatter;

            PluralFactory(UPluralType t) : type(t) {}
            const UPluralType type;
        };

        class Plural : public Selector {
        public:
            void selectKey(FormattingContext& context,
                           const UnicodeString* keys,
                           int32_t keysLen,
                           UnicodeString* prefs,
                           int32_t& prefsLen,
                           UErrorCode& status) const override;
            virtual ~Plural();

        private:
            friend class PluralFactory;

            // Adopts `r`
            Plural(const Locale& loc, PluralRules* r) : locale(loc), rules(r) {}

            const Locale& locale;
            LocalPointer<PluralRules> rules;
        };

        class TextFactory : public SelectorFactory {
        public:
            Selector* createSelector(const Locale& locale, UErrorCode& status) const override;
            virtual ~TextFactory();
        };

        class TextSelector : public Selector {
        public:
            void selectKey(FormattingContext& context,
                           const UnicodeString* keys,
                           int32_t keysLen,
                           UnicodeString* prefs,
                           int32_t& prefsLen,
                           UErrorCode& status) const override;
            virtual ~TextSelector();

        private:
            friend class TextFactory;

            // Formatting `value` to a string might require the locale
            const Locale& locale;

            TextSelector(const Locale& l) : locale(l) {}
        };
    };

} // namespace message2

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */

#endif /* U_SHOW_CPLUSPLUS_API */

#endif // MESSAGEFORMAT2_FUNCTION_REGISTRY_INTERNAL_H

#endif // U_HIDE_DEPRECATED_API
// eof
