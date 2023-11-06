// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#ifndef U_HIDE_DEPRECATED_API

#ifndef MESSAGEFORMAT_SERIALIZER_H
#define MESSAGEFORMAT_SERIALIZER_H

#if U_SHOW_CPLUSPLUS_API

#if !UCONFIG_NO_FORMATTING

U_NAMESPACE_BEGIN  namespace message2 {

    // Serializer class (private)
    // Converts a data model back to a string
    class Serializer : public UMemory {
    public:
        Serializer(const MessageFormatDataModel& m, UnicodeString& s) : dataModel(m), result(s) {}
        void serialize();

        const MessageFormatDataModel& dataModel;
        UnicodeString& result;

    private:
        using FunctionName     = MessageFormatDataModel::FunctionName;
        using VariableName     = MessageFormatDataModel::VariableName;

        void whitespace();
        void emit(UChar32);
        template <int32_t N>
        void emit(const UChar32 (&)[N]);
        void emit(const UnicodeString&);
        void emit(const FunctionName&);
        void emit(const VariableName&);
        void emit(const MessageFormatDataModel::Literal&);
        void emit(const MessageFormatDataModel::Key&);
        void emit(const MessageFormatDataModel::SelectorKeys&);
        void emit(const MessageFormatDataModel::Operand&);
        void emit(const MessageFormatDataModel::Expression&);
        void emit(const MessageFormatDataModel::PatternPart&);
        void emit(const MessageFormatDataModel::Pattern&);
        void emit(const MessageFormatDataModel::VariantMap&);
        void emit(const MessageFormatDataModel::OptionMap&);
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

