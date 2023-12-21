// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#ifndef U_HIDE_DEPRECATED_API

#ifndef MESSAGEFORMAT_PARSER_H
#define MESSAGEFORMAT_PARSER_H

#include "unicode/messageformat2.h"

#if U_SHOW_CPLUSPLUS_API

#if !UCONFIG_NO_FORMATTING

U_NAMESPACE_BEGIN

namespace message2 {

    using namespace data_model;

    // Parser class (private)
    class Parser : public UMemory {
    public:
	virtual ~Parser();
    private:
        friend class MessageFormatter;

        void parse(UParseError&, UErrorCode&);

	/*
	  Use an internal "parse error" structure to make it easier to translate
	  absolute offsets to line offsets.
	  This is translated back to a `UParseError` at the end of parsing.
	*/
	typedef struct MessageParseError {
	    // The line on which the error occurred
	    uint32_t line;
	    // The offset, relative to the erroneous line, on which the error occurred
	    uint32_t offset;
	    // The total number of characters seen before advancing to the current line. It has a value of 0 if line == 0.
	    // It includes newline characters, because the index does too.
	    uint32_t lengthBeforeCurrentLine;

	    // This parser doesn't yet use the last two fields.
	    UChar   preContext[U_PARSE_CONTEXT_LEN];
	    UChar   postContext[U_PARSE_CONTEXT_LEN];
	} MessageParseError;

	Parser(const UnicodeString &input, MessageFormatDataModel::Builder& dataModelBuilder, StaticErrors& e, UnicodeString& normalizedInputRef)
	  : source(input), index(0), errors(e), normalizedInput(normalizedInputRef), dataModel(dataModelBuilder) {
	  parseError.line = 0;
	  parseError.offset = 0;
	  parseError.lengthBeforeCurrentLine = 0;
	  parseError.preContext[0] = '\0';
	  parseError.postContext[0] = '\0';
	}

	// Used so `parseEscapeSequence()` can handle all types of escape sequences
	// (literal, text, and reserved)
	typedef enum { LITERAL, TEXT, RESERVED } EscapeKind;

	static void translateParseError(const MessageParseError&, UParseError&);
	static void setParseError(MessageParseError&, uint32_t);
	void maybeAdvanceLine();
	void parseBody(UErrorCode&);
	void parseDeclarations(UErrorCode&);
	void parseSelectors(UErrorCode&);

	void parseWhitespaceMaybeRequired(bool);
	void parseRequiredWhitespace();
	void parseOptionalWhitespace();
	void parseToken(UChar32);
	void parseTokenWithWhitespace(UChar32);
	template <int32_t N>
	void parseToken(const UChar32 (&)[N]);
	template <int32_t N>
	void parseTokenWithWhitespace(const UChar32 (&)[N]);
	UnicodeString parseName();
	VariableName parseVariableName();
	FunctionName parseFunction();
	void parseEscapeSequence(EscapeKind, UnicodeString&);
	void parseLiteralEscape(UnicodeString&);
	Literal parseLiteral();
	void parseOption(Operator::Builder&);
	void parseOptions(Operator::Builder&);
	void parseReservedEscape(UnicodeString&);
	void parseReservedChunk(Reserved::Builder&, UErrorCode&);
	Reserved parseReserved(UErrorCode&);
	Operator parseAnnotation(UErrorCode&);
	void parseLiteralOrVariableWithAnnotation(bool, Expression::Builder&, UErrorCode&);
	Expression parseExpression(bool&, UErrorCode&);
	void parseTextEscape(UnicodeString&);
	UnicodeString parseText();
	Key parseKey();
	SelectorKeys parseNonEmptyKeys(UErrorCode&);
	void errorPattern(UErrorCode& status);
	Pattern parsePattern(UErrorCode&);

	// The input string
	const UnicodeString &source;
	// The current position within the input string
	uint32_t index;
	// Represents the current line (and when an error is indicated),
	// character offset within the line of the parse error
	MessageParseError parseError;

	// The structure to use for recording errors
	StaticErrors& errors;

	// Normalized version of the input string (optional whitespace removed)
	UnicodeString& normalizedInput;

	// The parent builder
	MessageFormatDataModel::Builder &dataModel;
    }; // class Parser

} // namespace message2
U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */

#endif /* U_SHOW_CPLUSPLUS_API */

#endif // MESSAGEFORMAT_PARSER_H

#endif // U_HIDE_DEPRECATED_API
// eof
