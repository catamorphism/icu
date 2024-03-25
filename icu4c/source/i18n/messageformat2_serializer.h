// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#ifndef U_HIDE_DEPRECATED_API

#ifndef MESSAGEFORMAT_SERIALIZER_H
#define MESSAGEFORMAT_SERIALIZER_H

#if U_SHOW_CPLUSPLUS_API

#if !UCONFIG_NO_FORMATTING

#include "unicode/messageformat2_data_model.h"

U_NAMESPACE_BEGIN

namespace message2 {

    using namespace data_model;

    // Serializer class (private)
    // Converts a data model back to a string
    class Serializer : public UMemory {
    public:
        Serializer(const MFDataModel& m, UnicodeString& s) : dataModel(m), result(s) {}
        void serialize();

        const MFDataModel& dataModel;
        UnicodeString& result;

    private:

        void whitespace();
        void emit(UChar32);
        template <int32_t N>
        void emit(const UChar32 (&)[N]);
        void emit(const UnicodeString&);
        void emit(const Literal&);
        void emit(const Key&);
        void emit(const SelectorKeys&);
        void emit(const Operand&);
        void emit(const Reserved&);
        void emit(const Expression&);
        void emit(const PatternPart&);
        void emit(const Pattern&);
        void emit(const Variant*);
        void emitAttributes(const OptionMap&);
        void emit(const OptionMap&);
        void serializeUnsupported();
        void serializeDeclarations();
        void serializeSelectors();
        void serializeVariants();
    }; // class Serializer

} // namespace message2

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */

#endif /* U_SHOW_CPLUSPLUS_API */

#endif // MESSAGEFORMAT_SERIALIZER_H

#endif // U_HIDE_DEPRECATED_API
// eof

