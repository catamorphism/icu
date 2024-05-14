// © 2024 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#if !UCONFIG_NO_MF2

#include "hash.h"
#include "unicode/messageformat2_messagepart.h"
#include "unicode/unistr.h"

U_NAMESPACE_BEGIN

namespace message2 {

    const UnicodeString& MessagePart::getValue(UErrorCode& errorCode) const {
        if (U_SUCCESS(errorCode)) {
            if (type != UPART_LITERAL && type != UPART_STRING) {
                errorCode = U_ILLEGAL_ARGUMENT_ERROR;
            }
        }
        return value;
    }

    UMarkupType MessagePart::getMarkupType(UErrorCode& errorCode) const {
        if (U_SUCCESS(errorCode) && type != UPART_MARKUP) {
            errorCode = U_ILLEGAL_ARGUMENT_ERROR;
        }
        return markupKind;
    }

   const UnicodeString& MessagePart::getMarkupName(UErrorCode& errorCode) const {
        if (U_SUCCESS(errorCode) && type != UPART_MARKUP) {
            errorCode = U_ILLEGAL_ARGUMENT_ERROR;
        }
        return markupName;
    }

    const UnicodeString& MessagePart::getFallbackSource(UErrorCode& errorCode) const {
        if (U_SUCCESS(errorCode) && type != UPART_FALLBACK) {
            errorCode = U_ILLEGAL_ARGUMENT_ERROR;
        }
        return source;
    }

    const number::FormattedNumber& MessagePart::getNumberValue(UErrorCode& errorCode) const {
        if (U_SUCCESS(errorCode) && type != UPART_NUMBER) {
            errorCode = U_ILLEGAL_ARGUMENT_ERROR;
        }
        return numberValue;
    }

    const UnicodeString& MessagePart::getStringValue(UErrorCode& errorCode) const {
        if (U_SUCCESS(errorCode) && type != UPART_STRING) {
            errorCode = U_ILLEGAL_ARGUMENT_ERROR;
        }
        return value;
    }

    /* static */ MessagePart MessagePart::markup(const UnicodeString& name,
                                                 UMarkupType kind,
                                                 Hashtable&& options,
                                                 UErrorCode& errorCode) {
        return MessagePart(name, kind, std::move(options), errorCode);
    }

    /* static */ MessagePart MessagePart::fallback(const UnicodeString& source) {
        return MessagePart(UPART_FALLBACK, source);
    }

    /* static */ MessagePart MessagePart::string(const UnicodeString& val) {
        return MessagePart(UPART_STRING, val);
    }

    /* static */ MessagePart MessagePart::literal(const UnicodeString& val) {
        return MessagePart(UPART_LITERAL, val);
    }

    /* static */ MessagePart MessagePart::number(number::FormattedNumber&& n) {
        return MessagePart(std::move(n));
    }

    MessagePart::MessagePart(UMessagePartType t, const UnicodeString& val) : type(t) {
        if (type == UPART_FALLBACK) {
            source = val;
        } else {
            value = val;
        }
    }

    MessagePart::MessagePart(const UnicodeString& name,
                             UMarkupType markupType,
                             Hashtable&& options,
                             UErrorCode& errorCode) : type(UPART_MARKUP),
                                                      markupKind(markupType),
                                                      markupName(name) {
        if (U_FAILURE(errorCode)) {
            return;
        }
        markupOptionsCount = options.count();
        MarkupOption* arr = new MarkupOption[markupOptionsCount]();
        markupOptions.adoptInstead(arr);
        if (!markupOptions.isValid()) {
            errorCode = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
        // It may seem silly to unpack the hashtable into a LocalArray,
        // which is then reconstructed as a std::map when getMarkupOptions()
        // is called. However, getMarkupOptions() has to be header-only,
        // so it can't manipulate a Hashtable.
        int32_t pos = UHASH_FIRST;
        int32_t i = 0;
        const UHashElement* element = options.nextElement(pos);
        while (element != nullptr) {
            UnicodeString key = *static_cast<UnicodeString*>(element->key.pointer);
            UnicodeString value = *static_cast<UnicodeString*>(element->value.pointer);
            markupOptions[i].name = key;
            markupOptions[i].value = value;
            i++;
            element = options.nextElement(pos);
        }
    }

    MessagePart::MessagePart(number::FormattedNumber&& n) : type(UPART_NUMBER),
                                                            numberValue(std::move(n)) {}

    MessagePart::MessagePart(MessagePart&& other) : type(other.type), value(std::move(other.value)),
                                                    markupKind(other.markupKind),
                                                    markupName(std::move(other.markupName)),
                                                    markupOptions(std::move(other.markupOptions)),
                                                    markupOptionsCount(other.markupOptionsCount),
                                                    source(std::move(other.source)),
                                                    numberValue(std::move(other.numberValue)) {}
    MessagePart::~MessagePart() {}

} // namespace message2

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_MF2 */

#endif /* #if !UCONFIG_NO_FORMATTING */
