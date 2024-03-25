// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#ifndef U_HIDE_DEPRECATED_API

#ifndef MESSAGEFORMAT_SERIALIZER_H
#define MESSAGEFORMAT_SERIALIZER_H

#if U_SHOW_CPLUSPLUS_API

#if !UCONFIG_NO_FORMATTING

U_NAMESPACE_BEGIN

#if U_PF_WINDOWS <= U_PLATFORM && U_PLATFORM <= U_PF_CYGWIN
#if defined(_MSC_VER)
// Ignore warning 4661 as LocalPointerBase does not use operator== or operator!=
#pragma warning(push)
#pragma warning(disable: 4661)
#endif
#endif

namespace message2 {

    using namespace data_model;

    // Serializer class (private)
    // Converts a data model back to a string
    class Serializer : public UMemory {
    public:
        Serializer(const MessageFormatDataModel& m, UnicodeString& s) : dataModel(m), result(s) {}
        void serialize();

        const MessageFormatDataModel& dataModel;
        UnicodeString& result;

    private:

        void whitespace();
        void emit(UChar32);
        template <int32_t N>
        void emit(const UChar32 (&)[N]);
        void emit(const UnicodeString&);
        void emit(const FunctionName&);
        void emit(const VariableName&);
        void emit(const Literal&);
        void emit(const Key&);
        void emit(const SelectorKeys&);
        void emit(const Operand&);
        void emit(const Expression&);
        void emit(const PatternPart&);
        void emit(const Pattern&);
        void emit(const Variant*);
        void emit(const OptionMap&);
        void serializeDeclarations();
        void serializeSelectors();
        void serializeVariants();
    }; // class Serializer

} // namespace message2

#if U_PF_WINDOWS <= U_PLATFORM && U_PLATFORM <= U_PF_CYGWIN
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
#endif

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */

#endif /* U_SHOW_CPLUSPLUS_API */

#endif // MESSAGEFORMAT_SERIALIZER_H

#endif // U_HIDE_DEPRECATED_API
// eof

