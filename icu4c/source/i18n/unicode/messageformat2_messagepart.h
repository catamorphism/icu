// © 2024 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#include "unicode/utypes.h"

#ifndef MESSAGEFORMAT2_MESSAGEPART_H
#define MESSAGEFORMAT2_MESSAGEPART_H

#if U_SHOW_CPLUSPLUS_API

#if !UCONFIG_NO_FORMATTING

#if !UCONFIG_NO_MF2

#include "unicode/formattednumber.h"
#include "unicode/messageformat2_data_model.h"
#include "unicode/unistr.h"

#ifndef U_HIDE_DEPRECATED_API

#include <map>
#include <variant>

U_NAMESPACE_BEGIN

class Hashtable;
class UVector;

namespace message2 {

    struct U_I18N_API MarkupOption : public UObject {
        UnicodeString name;
        UnicodeString value;
    };

    class U_I18N_API MessagePart : public UObject {
    public:
        /**
         * Enum designating the type of a MessagePart instance.
         *
         * @internal ICU 76 technology preview
         * @deprecated This API is for technology preview only.
         */
        typedef enum UMessagePartType {
            UPART_LITERAL = 0, /**< asLiteral() will return without error.*/
            UPART_MARKUP,      /**< asMarkup() will return without error.*/
            UPART_FALLBACK,    /**< asFallback() will return without error.*/
            UPART_NUMBER,      /**< asNumber() will return without error.*/
            UPART_STRING,      /**< asString() will return without error.*/
             /**
             * One more than the highest normal UFormattableType value.
             * @deprecated ICU 58 The numeric value may change over time, see ICU ticket #12420.
             */
            UPART_COUNT
        } UMessagePartType;
        // note: both read and write methods need to be public,
        // since custom functions could create MessageParts
        /**
         * Gets the type of this part.
         *
         * @internal ICU 76 technology preview
         * @deprecated This API is for technology preview only.
         */
        UMessagePartType getType() const { return type; }
        /**
         * Gets the value of this part.
         *
         * @param errorCode Input/output error code; set to U_ILLEGAL_ARGUMENT_ERROR
         *         if this is not a literal or string part
         * @internal ICU 76 technology preview
         * @deprecated This API is for technology preview only.
         */
        const UnicodeString& getValue(UErrorCode&) const;
        /**
         * Gets the type of a markup part.
         *
         * @param errorCode Input/output error code; set to U_ILLEGAL_ARGUMENT_ERROR
         *         if this is not a markup part
         * @internal ICU 76 technology preview
         * @deprecated This API is for technology preview only.
         */
        UMarkupType getMarkupType(UErrorCode&) const;
        /**
         * Gets the name of a markup part.
         *
         * @param errorCode Input/output error code; set to U_ILLEGAL_ARGUMENT_ERROR
         *         if this is not a markup part
         * @internal ICU 76 technology preview
         * @deprecated This API is for technology preview only.
         */
        const UnicodeString& getMarkupName(UErrorCode&) const;
        /**
         * Gets the options of a markup part.
         *
         * @param errorCode Input/output error code; set to U_ILLEGAL_ARGUMENT_ERROR
         *         if this is not a markup part
         * @internal ICU 76 technology preview
         * @deprecated This API is for technology preview only.
         */
        std::map<std::string, std::string> getMarkupOptions(UErrorCode& errorCode) const {
            std::map<std::string, std::string> result;
            if (U_SUCCESS(errorCode)) {
                for (int32_t i = 0; i < markupOptionsCount; i++) {
                    std::string nameStr;
                    std::string valueStr;
                    nameStr = markupOptions[i].name.toUTF8String(nameStr);
                    valueStr = markupOptions[i].value.toUTF8String(valueStr);
                    result[nameStr] = valueStr;
                }
            }
            return result;
        }
        /**
         * Gets the source of a fallback part
         *
         * @param errorCode Input/output error code; set to U_ILLEGAL_ARGUMENT_ERROR
         *         if this is not a fallback part
         * @internal ICU 76 technology preview
         * @deprecated This API is for technology preview only.
         */
        const UnicodeString& getFallbackSource(UErrorCode&) const;
        /**
         * Gets the value of a number part
         *
         * @param errorCode Input/output error code; set to U_ILLEGAL_ARGUMENT_ERROR
         *         if this is not a number part
         * @internal ICU 76 technology preview
         * @deprecated This API is for technology preview only.
         */
        const number::FormattedNumber& getNumberValue(UErrorCode&) const;
        /**
         * Gets the value of a string part
         *
         * @param errorCode Input/output error code; set to U_ILLEGAL_ARGUMENT_ERROR
         *         if this is not a string part
         * @internal ICU 76 technology preview
         * @deprecated This API is for technology preview only.
         */
        const UnicodeString& getStringValue(UErrorCode&) const;
        /*
         * Literal part factory method.
         *
         * @param s A string to wrap as a MessagePart.
         * @internal ICU 76 technology preview
         * @deprecated This API is for technology preview only.
         */
        static MessagePart literal(const UnicodeString&);
        /**
         * Markup part factory method.
         *
         * @param name The name of the markup placeholder.
         * @param kind The kind of markup (open, close, or standalone)
         * @param options The options attached to this markup placeholder
         *                (a Hashtable whose values are UnicodeStrings).
         * @param errorCode Input/output error code
         * @internal ICU 76 technology preview
         * @deprecated This API is for technology preview only.
         */
        static MessagePart markup(const UnicodeString& name,
                                  UMarkupType kind,
                                  Hashtable&& options,
                                  UErrorCode& errorCode);
        /**
         * Fallback part factory method.
         *
         * @param source The source of this fallback value.
         * @internal ICU 76 technology preview
         * @deprecated This API is for technology preview only.
         */
        static MessagePart fallback(const UnicodeString& source);
        /**
         * String part factory method.
         *
         * @param s A string to wrap as an expression MessagePart.
         * @internal ICU 76 technology preview
         * @deprecated This API is for technology preview only.
         */
        static MessagePart string(const UnicodeString& s);
        /**
         * Number part factory method.
         *
         * @param n A formatted number to wrap as an expression MessagePart.
         * @internal ICU 76 technology preview
         * @deprecated This API is for technology preview only.
         */
        static MessagePart number(number::FormattedNumber&& n);
        /**
         * Destructor.
         *
         * @internal ICU 76 technology preview
         * @deprecated This API is for technology preview only.
         */
        virtual ~MessagePart();
        /**
         * Non-member swap function.
         * @param p1 will get p2's contents
         * @param p2 will get p1's contents
         *
         * @internal ICU 76 technology preview
         * @deprecated This API is for technology preview only.
         */
        friend inline void swap(MessagePart& p1, MessagePart& p2) noexcept {
            using std::swap;

            swap(p1.type, p2.type);
            swap(p1.markupKind, p2.markupKind);
            swap(p1.markupName, p2.markupName);
            swap(p1.markupOptions, p2.markupOptions);
            swap(p1.source, p2.source);
            swap(p1.numberValue, p2.numberValue);
        }
        MessagePart& operator=(MessagePart) noexcept;
        MessagePart(MessagePart&&);
    private:
        UMessagePartType type;

        // This should be non-empty for literal and string parts;
        // empty for number, markup and fallback parts
        UnicodeString value;

        // Only applies to markup parts
        data_model::UMarkupType markupKind;
        UnicodeString markupName;
        LocalArray<MarkupOption> markupOptions;
        int32_t markupOptionsCount = 0;

        // Should be non-empty for fallback parts; ignored otherwise
        UnicodeString source;

        // Ignored for non-number parts
        number::FormattedNumber numberValue;

        // Literal, fallback, and string constructor
        MessagePart(UMessagePartType, const UnicodeString&);
        // Markup constructor
        MessagePart(const UnicodeString&, UMarkupType, Hashtable&&, UErrorCode&);
        // Number constructor
        explicit MessagePart(number::FormattedNumber&&);
    }; // class MessagePart

} // namespace message2

U_NAMESPACE_END

#endif // U_HIDE_DEPRECATED_API

#endif /* #if !UCONFIG_NO_MF2 */

#endif /* #if !UCONFIG_NO_FORMATTING */

#endif /* U_SHOW_CPLUSPLUS_API */

#endif // MESSAGEFORMAT2_MESSAGEPART_H

// eof
