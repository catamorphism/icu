// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
/********************************************************************
 * COPYRIGHT:
 * Copyright (c) 1997-2015, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************
 *
 * File MSGFMT2.CPP
 *
 ********************************************************************/

#include "unicode/rep.h"
#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/msgfmt2.h"
#include "uhash.h"
#include "uvector.h"

// *****************************************************************************
// class MessageFormat2
// *****************************************************************************

#define SINGLE_QUOTE      ((char16_t)0x0027)
#define COMMA             ((char16_t)0x002C)
#define LEFT_CURLY_BRACE  ((char16_t)0x007B)
#define RIGHT_CURLY_BRACE ((char16_t)0x007D)
#define SPACE             ((char16_t)0x0020)
#define HTAB              ((char16_t)0x0009)
#define CR                ((char16_t)0x000D)
#define LF                ((char16_t)0x000A)
#define BACKSLASH         ((char16_t)0x005C)

/*
static const char16_t ID_LET[] = {
  0x6C, 0x65, 0x74
};
static const char16_t ID_MATCH[] = {
  0x6D, 0x61, 0x74, 0x63, 0x68
};
static const char16_t ID_WHEN[] = {
  0x77, 0x68, 0x65, 0x6E
};
*/

#define RETURN_NULL_IF_HELPER_FAILS(call) \
  call; \
  if (U_FAILURE(errorCode)) \
    return nullptr;

#define RETURN_IF_HELPER_FAILS(call) \
  call; \
  if (U_FAILURE(errorCode)) \
    return;

#define ALLOCATION_ERROR_IF_HELPER_FAILS(call) call; if (U_FAILURE(errorCode)) { setParseError(parseError, index); errorCode = U_MEMORY_ALLOCATION_ERROR; return; }

#define ERROR(parseError, errorCode, index) \
  setParseError(parseError, index); \
  errorCode = U_MESSAGE_PARSE_ERROR;

// TODO: use a more specific error message
#define MISMATCHED_MARKUP(parseError, errorCode, index) ERROR(parseError, errorCode, index)

U_NAMESPACE_BEGIN
/*
static const char16_t _ESCAPED_BACKSLASH[] = {
    0x5C, 0x5C
};
*/

#define ESCAPED_BACKSLASH UNICODE_STRING_SIMPLE("\\\\")
#define ESCAPED_PIPE UNICODE_STRING_SIMPLE("\\|")
#define ESCAPED_LBRACE UNICODE_STRING_SIMPLE("\\{")
#define ESCAPED_RBRACE UNICODE_STRING_SIMPLE("\\}")
#define PIPE UNICODE_STRING_SIMPLE("|")
#define EQUALS UNICODE_STRING_SIMPLE("=")
#define LBRACE UNICODE_STRING_SIMPLE("{")
#define RBRACE UNICODE_STRING_SIMPLE("}")    
#define ID_LET UNICODE_STRING_SIMPLE("let")
#define ID_WHEN UNICODE_STRING_SIMPLE("when")
#define ID_MATCH UNICODE_STRING_SIMPLE("match")        
        
using namespace MessageFormatData;
/*
using Literal = MessageFormat2::MessageFormatDataModel::Literal;
using Annotation = MessageFormat2::MessageFormatDataModel::Annotation;
using Option = MessageFormat2::MessageFormatDataModel::Option;
using OPTION_KIND = MessageFormat2::MessageFormatDataModel::OPTION_KIND;
*/

// -------------------------------------
// UOBJECT_DEFINE_RTTI_IMPLEMENTATION(MessageFormat2)

// -------------------------------------
// Creates a MessageFormat instance based on the pattern.

MessageFormat2::MessageFormat2(const UnicodeString& pattern,
                               UErrorCode& success)
/*
: fLocale(Locale::getDefault()),  // Uses the default locale
  msgPattern(success),
  formatAliases(nullptr),
  formatAliasesCapacity(0),
  argTypes(nullptr),
  argTypeCount(0),
  argTypeCapacity(0),
  hasArgTypeConflicts(false),
  defaultNumberFormat(nullptr),
  defaultDateFormat(nullptr),
  cachedFormatters(nullptr),
  customFormatArgStarts(nullptr),
  pluralProvider(*this, UPLURAL_TYPE_CARDINAL),
  ordinalProvider(*this, UPLURAL_TYPE_ORDINAL)
*/
{
//    setLocaleIDs(fLocale.getName(), fLocale.getName());
/*
See common/unicode/messagepattern.h
common/messagepattern.cpp

icu4j/main/classes/core/src/com/ibm/icu/message2
*/
    UParseError parseError;
    dataModel = parse(pattern, &parseError, success);
   // applyPattern(pattern, success);
}

/*
MessageFormat2::~MessageFormat2()
{
//    uhash_close(args);
}
*/


UnicodeString&
MessageFormat2::format(const UHashtable* arguments,
                       UnicodeString& appendTo,
                       UErrorCode& success) const {
    // TODO
    U_ASSERT(arguments);
//    appendTo.append(reinterpret_cast<const char16_t*>(""), 0, -1);
    success = U_ZERO_ERROR;
    return appendTo;
}

// -------------------------------------
// Parses the source pattern

MessageFormatDataModel*
MessageFormat2::parse(const UnicodeString& source) const {
    UErrorCode ec = U_ZERO_ERROR;
    UParseError parseError;
    return parse(source, &parseError, ec);
}

void setParseError(UParseError *parseError, uint32_t index) {
    // See MessagePattern::setParseError(UParseError *parseError, int32_t index) {
    // TODO: fill this in with more useful information
    parseError->offset = index;
    parseError->line = 0;
    parseError->preContext[0] = 0;
    parseError->postContext[0] = 0;    
}

bool isWhitespace(char16_t c) {
  switch(c) {
    case SPACE:
    case HTAB:
    case CR:
    case LF:
      return true;
    default:
      return false;
  }
}

bool inRange(char16_t c, char16_t first, char16_t last) {
  U_ASSERT(first < last);
  return(c >= first && c <= last);
}

void parseWhitespaceMaybeRequired(bool required,
                                  const UnicodeString& source,
                                  uint32_t &index,
                                  UParseError* parseError,
                                  UErrorCode& errorCode) {
  bool sawWhitespace = false;

  while (isWhitespace(source[index++])) { sawWhitespace = true; }

  if (!sawWhitespace && required) {
    ERROR(parseError, errorCode, index);
  }
}

void parseRequiredWhitespace(const UnicodeString& source,
                             uint32_t &index,
                             UParseError* parseError,
                             UErrorCode& errorCode) {
  parseWhitespaceMaybeRequired(true, source, index, parseError, errorCode);
}

void parseWhitespace(const UnicodeString& source,
                     uint32_t &index,
                     UParseError* parseError,
                     UErrorCode& errorCode) {
  parseWhitespaceMaybeRequired(false, source, index, parseError, errorCode);
}

bool nextTokenIs(const UnicodeString& token, const UnicodeString& source, uint32_t index) {
  uint32_t tokenPos = 0;
  uint32_t strPos = index;

  while (((int32_t) tokenPos) < token.length()) {
    if (source[strPos] != token[tokenPos]) {
      return false;
    }
    strPos++;
    tokenPos++;
  }
  return true;
}

void parseToken(const UnicodeString& token,
                const UnicodeString& source,
                uint32_t &index,
                UParseError *parseError,
                UErrorCode& errorCode) {
  uint32_t tokenPos = 0;

  while (((int32_t) tokenPos) < token.length()) {
    if (source[index] != token[tokenPos]) {
      ERROR(parseError, errorCode, index);
      return;
    }
    index++;
    tokenPos++;
  }
}

void parseTokenWithWhitespace(const UnicodeString& token,
                              const UnicodeString& source,
                              uint32_t &index,
                              UParseError *parseError,
                              UErrorCode& errorCode) {
  RETURN_IF_HELPER_FAILS(parseWhitespace(source, index, parseError, errorCode));
  RETURN_IF_HELPER_FAILS(parseToken(token, source, index, parseError, errorCode));
  RETURN_IF_HELPER_FAILS(parseWhitespace(source, index, parseError, errorCode));
}

bool isAlpha(char16_t c) {
  return (inRange(c, 0x0041, 0x005A)
          || inRange(c, 0x0061, 0x007A));
}

bool isDigit(char16_t c) {
  return (inRange(c, 0x0030, 0x0039));
}

bool isNameStart(char16_t c) {
  return (isAlpha(c)
          || c == '_'
          || inRange(c, 0x00C0, 0x00D6)
          || inRange(c, 0x00D8, 0x00F6)
          || inRange(c, 0x00F8, 0x02FF)
          || inRange(c, 0x0370, 0x037D)
          || inRange(c, 0x037F, 0x1FFF)
          || inRange(c, 0x200C, 0x200D)
          || inRange(c, 0x2070, 0x218F)
          || inRange(c, 0x2C00, 0x2FEF)
          || inRange(c, 0x3001, 0xD7FF)
          || inRange(c, 0xF900, 0xFDCF)
          || inRange(c, 0xFDF0, 0xFFFD));
          // inRange(c, 0x10000, 0xEFFFF)); // Not sure about this -- spec says %x10000-EFFFF, but 0xEFFFF is more than the max char16_t
}

bool isNameChar(char16_t c) {
  return (isNameStart(c)
          || isDigit(c)
          || c == '-'
          || c == '.'
          || c == 0x00B7
          || inRange(c, 0x0300, 0x036F)
          || inRange(c, 0x203F, 0x2040));
}

void parseNmtoken(const UnicodeString& source,
                  uint32_t &index,
                  UParseError *parseError,
                  UErrorCode& errorCode,
                  UnicodeString& result) {

  if (!isNameChar(source[index])) {
    ERROR(parseError, errorCode, index);        
    return;
  }

  while (isNameChar(source[index])) {
    result += source[index++];
  }  
}

void parseName(const UnicodeString& source,
               uint32_t &index,
               UParseError *parseError,
               UErrorCode& errorCode,
               UnicodeString& result) {
  if (!isNameStart(source[index])) {
    errorCode = U_MESSAGE_PARSE_ERROR;
    return;
  }

  RETURN_IF_HELPER_FAILS(parseNmtoken(source, index, parseError, errorCode, result));
}

void parseVariableName(const UnicodeString& source,
                       uint32_t &index,
                       UParseError *parseError,
                       UErrorCode& errorCode,
                       UnicodeString& result) {
  if (source[index++] != '$') {
    errorCode = U_MESSAGE_PARSE_ERROR;
    return;
  }

 parseName(source, index, parseError, errorCode, result);
}

void parseFunction(const UnicodeString& source,
                       uint32_t &index,
                       UParseError *parseError,
                       UErrorCode& errorCode,
                       UnicodeString& result) {
  if (source[index++] != ':') {
    ERROR(parseError, errorCode, index);
    return;
  }
  
  parseName(source, index, parseError, errorCode, result);
}

void parseLiteralEscape(const UnicodeString& source,
                        uint32_t &index,
                        UParseError *parseError,
                        UErrorCode& errorCode,
                        UnicodeString& s) {
  U_ASSERT(source[index] == BACKSLASH);
  index++; // Skip the initial backslash
  switch(source[index++]) {
  case BACKSLASH: {
    s += ESCAPED_BACKSLASH;
    break;
  }
  case '|': {
    s += ESCAPED_PIPE;
    break;
  }
  default: {
    ERROR(parseError, errorCode, index);
    return;
  }
  }
  
}

bool isLiteralChar(char16_t c) {
    return (inRange(c, 0x0000, 0x005B)    // Omit backslash
            || inRange(c, 0x005D, 0x007B) // Omit pipe
            || inRange(c, 0x007D, 0xD7FF) // Omit surrogates
            || c >= 0xE000);
//            || inRange(c, 0xE000, 0x10FFFF)); see comment in isNameStart()
}

void parseLiteralString(const UnicodeString &source, uint32_t &index, UParseError *parseError,
                        UErrorCode &errorCode, UnicodeString &result) { 
  RETURN_IF_HELPER_FAILS(parseToken(PIPE, source, index, parseError, errorCode));

  while (isLiteralChar(source[index]) || source[index] == BACKSLASH) {
    if(source[index] == BACKSLASH) {
      RETURN_IF_HELPER_FAILS(parseLiteralEscape(source, index, parseError, errorCode, result));
    } else {
      result += source[index++];
    }
  }

  RETURN_IF_HELPER_FAILS(parseToken(PIPE, source, index, parseError, errorCode));
}


void parseOption(const UnicodeString &source, uint32_t &index, UParseError *parseError,
                 UErrorCode &errorCode, UnicodeString& lhs, OPTION_KIND& kind, UnicodeString& rhs) {
  RETURN_IF_HELPER_FAILS(parseName(source, index, parseError, errorCode, lhs));

  RETURN_IF_HELPER_FAILS(parseTokenWithWhitespace(EQUALS, source, index, parseError, errorCode));

  // literal | nmtoken | variable
  switch(source[index]) {
    case '|': {
      RETURN_IF_HELPER_FAILS(parseLiteralString(source, index, parseError, errorCode, rhs));
      kind = O_LITERAL;
      return;
    }
    case '$': {
      RETURN_IF_HELPER_FAILS(parseVariableName(source, index, parseError, errorCode, rhs));
      kind = O_VARIABLE;
      return;
    }
    default: {
      // must be nmtoken
      RETURN_IF_HELPER_FAILS(parseNmtoken(source, index, parseError, errorCode, rhs));
      kind = O_NMTOKEN;
      return;
    }
  }
}

void parseOptions(const UnicodeString &source, uint32_t &index, UParseError *parseError,
                 UErrorCode &errorCode, UHashtable& options) {  
  while (isNameStart(source[index])) {
      UnicodeString lhs;
      OPTION_KIND kind;
      UnicodeString rhs;
      RETURN_IF_HELPER_FAILS(parseOption(source, index, parseError, errorCode, lhs, kind, rhs));
      Option opt(kind, rhs);
      uhash_put(&options, &lhs, &opt, &errorCode);
      if (U_FAILURE(errorCode)) {
        ERROR(parseError, errorCode, index);
      }
  }
}

bool annotationFollows(const UnicodeString &source, uint32_t index) { return (source[index] == ':'); }

Annotation *parseAnnotation(const UnicodeString &source, uint32_t &index, UParseError *parseError,
                            UErrorCode &errorCode) {
  UnicodeString functionName;
  RETURN_NULL_IF_HELPER_FAILS(parseFunction(source, index, parseError, errorCode, functionName));

  if (!isWhitespace(source[index])) {
    // Options, if present, must be preceded by whitespace
    ERROR(parseError, errorCode, index);
    return nullptr;
  }

  RETURN_NULL_IF_HELPER_FAILS(parseRequiredWhitespace(source, index, parseError, errorCode));

  UHashtable *options;
  RETURN_NULL_IF_HELPER_FAILS(options = uhash_open(uhash_hashChars, uhash_compareChars, nullptr, &errorCode));
  RETURN_NULL_IF_HELPER_FAILS(parseOptions(source, index, parseError, errorCode, *options));
  
  Annotation *a = createAnnotation(functionName, options, errorCode);
  if (!a) {
      errorCode = U_MEMORY_ALLOCATION_ERROR;
      delete options;
      return nullptr;
  }
  return a;
}

Literal* parseLiteralWithAnnotation(const UnicodeString& source,
                                uint32_t &index,
                                UParseError *parseError,
                                UErrorCode& errorCode) {
  UnicodeString s;
  RETURN_NULL_IF_HELPER_FAILS(parseLiteralString(source, index, parseError, errorCode, s));

  RETURN_NULL_IF_HELPER_FAILS(parseWhitespace(source, index, parseError, errorCode));

  if (annotationFollows(source, index)) {
    Annotation* a;
    RETURN_NULL_IF_HELPER_FAILS(a = parseAnnotation(source, index, parseError, errorCode));
    if (!a) {
      errorCode = U_MEMORY_ALLOCATION_ERROR;
      return nullptr;
    }
    return createLiteral(s, a, errorCode);
  }
  return createLiteral(s, errorCode);
}

Variable* parseVariableWithAnnotation(const UnicodeString &source, uint32_t &index, UParseError *parseError,
                                 UErrorCode &errorCode) {
  UnicodeString varName;
  RETURN_NULL_IF_HELPER_FAILS(parseVariableName(source, index, parseError, errorCode, varName));

  RETURN_NULL_IF_HELPER_FAILS(parseWhitespace(source, index, parseError, errorCode));

  if (annotationFollows(source, index)) {
      Annotation *a = parseAnnotation(source, index, parseError, errorCode);
      if (U_FAILURE(errorCode)) {
          return nullptr;
      }
      Variable* v = createVariable(varName, a, errorCode);
      if (!v) {
          errorCode = U_MEMORY_ALLOCATION_ERROR;
          delete a;
          return nullptr;
      }
      return v;
  }

  Variable* v = createVariable(varName, errorCode);
  if (!v) {
      errorCode = U_MEMORY_ALLOCATION_ERROR;
      return nullptr;
  }
  return v;
}

void parseExpression(const UnicodeString& source,
                     uint32_t &index,
                     UParseError *parseError,
                     UErrorCode& errorCode,
                     Expression& result) {
  
  // literal '|', variable '$' or annotation ':'
  switch(source[index]) {
    case '|': {
      RETURN_IF_HELPER_FAILS(result = *parseLiteralWithAnnotation(source, index, parseError, errorCode));
    }
    case '$': {
      RETURN_IF_HELPER_FAILS(result = *parseVariableWithAnnotation(source, index, parseError, errorCode));
    }
    case ':': {
      RETURN_IF_HELPER_FAILS(result = *parseAnnotation(source, index, parseError, errorCode));
    }
    default: {
      errorCode = U_MESSAGE_PARSE_ERROR;
    }
  }
}


// MessageFormatDeclarations* = UHashTable* variable expression
Declarations*
parseDeclarations(const UnicodeString& source,
                                  uint32_t &index,
                                  UParseError *parseError,
                                  UErrorCode& errorCode) {
  RETURN_NULL_IF_HELPER_FAILS(UHashtable* t = uhash_open(uhash_hashChars, uhash_compareChars, nullptr, &errorCode));

  RETURN_NULL_IF_HELPER_FAILS(Declarations* decls = Declarations::create(t, errorCode));
  while (nextTokenIs(ID_LET, source, index)) {
    RETURN_NULL_IF_HELPER_FAILS(parseToken(ID_LET, source, index, parseError, errorCode));
    
    RETURN_NULL_IF_HELPER_FAILS(parseRequiredWhitespace(source, index, parseError, errorCode));
    UnicodeString variableName; 
    RETURN_NULL_IF_HELPER_FAILS(parseVariableName(source, index, parseError, errorCode, variableName));
    RETURN_NULL_IF_HELPER_FAILS(parseTokenWithWhitespace(EQUALS, source, index, parseError, errorCode));
    RETURN_NULL_IF_HELPER_FAILS(parseToken(LBRACE, source, index, parseError, errorCode));
    Expression expression;
    RETURN_NULL_IF_HELPER_FAILS(parseExpression(source, index, parseError, errorCode, expression));
    RETURN_NULL_IF_HELPER_FAILS(parseToken(RBRACE, source, index, parseError, errorCode));
    
    RETURN_NULL_IF_HELPER_FAILS(decls->addDeclaration(variableName, expression, errorCode));
  }

  return decls;  
}

bool isTextChar(char16_t c) {
    return (inRange(c, 0x0000, 0x005B)    // Omit backslash
            || inRange(c, 0x005D, 0x007A) // Omit {
            || c == 0x007C                // }
            || inRange(c, 0x007E, 0xD7FF) // Omit surrogates
            || c >= 0xE000); 
 //           || inRange(c, 0xE000, 0x10FFFF));                                      
}

void parseTextEscape(const UnicodeString &source, uint32_t &index,
                     UParseError* parseError,
                     UErrorCode& errorCode,
                     UnicodeString& text) {
  U_ASSERT(source[index] == BACKSLASH);
  index++; // Skip the initial backslash
  switch(source[index++]) {
  case BACKSLASH: {
    text += ESCAPED_BACKSLASH;
    break;
  }
  case '{': {
    text += ESCAPED_LBRACE;
    break;
  }
  case '}': {
    text += ESCAPED_RBRACE;
    break;
  }
  default: {
    ERROR(parseError, errorCode, index);
    break;
  }
  }
}

void parseText(const UnicodeString &source, uint32_t &index,
               UParseError* parseError,
               UErrorCode& errorCode,
               UnicodeString& text) {
  while(isTextChar(source[index]) || source[index] == BACKSLASH) {
    if (source[index] == BACKSLASH) {
      RETURN_IF_HELPER_FAILS(parseTextEscape(source, index, parseError, errorCode, text));
    } else {
      text += source[index++];
    }
  }
  if (text.length() == 0) {
    // text must be non-empty
    errorCode = U_MESSAGE_PARSE_ERROR;
  }
}


/*
void parsePatternParts(const UnicodeString& source,
                  uint32_t &index,
                  UParseError *parseError,
                  UErrorCode& errorCode,
                  UVector& parts) {
    // Parse the *(text / placeholder) sequence in a pattern
    while (!(source[index] == '}')) {
        switch (source[index]) {
        case '{': {
            PlaceholderPattern p;
            RETURN_IF_HELPER_FAILS(parsePlaceholder(source, index++, parseError, errorCode, p));
            parts.append(p);
            break;
        }
        default: {
            TextPattern p;
            RETURN_IF_HELPER_FAILS(parseTextPattern(source, index, parseError, errorCode, p));
            parts.append(p);
            break;
        }
        }
    }
    // Consume the remaining '}'
    index++;
}
*/

void parseKey(const UnicodeString& source,
              uint32_t &index,
              UParseError *parseError,
              UErrorCode& errorCode,
              Key* &key) {
  // Literal | nmtoken | '*'
  switch (source[index]) {
    case '|': {
      UnicodeString literalStr;
      RETURN_IF_HELPER_FAILS(parseLiteralString(source, index, parseError, errorCode, literalStr));
      key = new KeyLiteral(literalStr);
      break;
    }
    case '*': {
      index++;
      key = new KeyWildcard();
      break;
    }
    default: {
      // nmtoken
      UnicodeString nameStr;
      RETURN_IF_HELPER_FAILS(parseNmtoken(source, index, parseError, errorCode, nameStr));
      key = new KeyName(nameStr);
      break;
    }
  }
}

void parseNonEmptyKeys(const UnicodeString& source,
                       uint32_t &index,
                       UParseError *parseError,
                       UErrorCode& errorCode,
                       Keys& keys) {

  // Need parseRequiredWhitespace before the first key...
  RETURN_IF_HELPER_FAILS(parseRequiredWhitespace(source, index, parseError, errorCode));
  Key* k;
  RETURN_IF_HELPER_FAILS(parseKey(source, index, parseError, errorCode, k));
  ALLOCATION_ERROR_IF_HELPER_FAILS(keys.addKey(k, errorCode));

  // Before all other keys, it's ambiguous whether whitespace is the required whitespace
  // before `key` or the optional whitespace before `pattern`. So instead, we exit from
  // the loop early if the whitespace is missing, possibly causing the parse error to
  // be reported later
  while (isWhitespace(source[index])) {
    RETURN_IF_HELPER_FAILS(parseWhitespace(source, index, parseError, errorCode));
    if (source[index] == '{') {
      // then we're done parsing the keys and a pattern follows -- break
      return;
    }
    RETURN_IF_HELPER_FAILS(parseKey(source, index, parseError, errorCode, k));
    ALLOCATION_ERROR_IF_HELPER_FAILS(keys.addKey(k, errorCode));
  }
}


bool matchesMarkup(PatternToken* tok, UnicodeString& name) {
  switch(tok->kind) {
    case PT_PLACEHOLDER: {
      PlaceholderPatternToken* pt = (PlaceholderPatternToken*) tok;
      switch(pt->kind) {
      case P_MARKUP_START:
        return ((PlaceholderTokens::MarkupStart*) tok)->markupName == name;
      default:
        return false;
      }
    }
  default:
    return false;
  }
}

// UVector<PatternToken>
void constructPatternTree(uint32_t index, UParseError *parseError, UErrorCode &errorCode, const UVector &tokens, Pattern* &p) {
    ALLOCATION_ERROR_IF_HELPER_FAILS(UStack markup(errorCode)); // PlaceholderPatternToken

    p = Pattern::createPattern(errorCode);

    for (int32_t i = 0; i < tokens.size(); i++) {
        switch (((PatternToken *)tokens[i])->kind) {
        case PT_TEXT: {
          ALLOCATION_ERROR_IF_HELPER_FAILS(p->addPart(new TextPattern(*((TextPatternToken*) tokens[i])), errorCode));
          break;
        }
        case PT_PLACEHOLDER: {
          switch (((PlaceholderPatternToken*)tokens[i])->kind) {
          case P_MARKUP_START: {
            ALLOCATION_ERROR_IF_HELPER_FAILS(markup.push(tokens[i], errorCode));
            break;
          }
          case P_MARKUP_END: {
            ALLOCATION_ERROR_IF_HELPER_FAILS(UVector* markupChildren = new UVector(errorCode)); // UVector<PatternPart>
            markupChildren->setDeleter(uprv_deleteUObject);
            PlaceholderTokens::MarkupEnd* currentToken = (PlaceholderTokens::MarkupEnd*) tokens[i];  
            while (!matchesMarkup(((PatternToken*) markup.peek()), currentToken->markupName)) {
              PatternToken* child = (PatternToken*) markup.pop();
              ALLOCATION_ERROR_IF_HELPER_FAILS(markupChildren->insertElementAt(child, 0, errorCode));
            }
            // Top of stack should match this tag's name
            PatternToken* possibleOpener = (PatternToken*) markup.pop();
            if (!matchesMarkup(possibleOpener, currentToken->markupName)) {
              MISMATCHED_MARKUP(parseError, errorCode, currentToken->index);
              return;
            }
            // There's a match -- push the new node onto the stack
            PlaceholderTokens::MarkupStart* startToken = (PlaceholderTokens::MarkupStart*) possibleOpener;
            ALLOCATION_ERROR_IF_HELPER_FAILS(p->addPart(new Markup(startToken->markupName, startToken->options, markupChildren), errorCode));
            break;
          }
          case P_PLACEHOLDER_EXPRESSION: {
            // If we're inside a markup tag, push this to the markup stack; it will be
            // converted when we see the close tag
            if (!markup.empty()) {
              ALLOCATION_ERROR_IF_HELPER_FAILS(markup.push(tokens[i], errorCode));
            } else {
              // push directly to the result vector
              ALLOCATION_ERROR_IF_HELPER_FAILS(p->addPart(new PlaceholderExpression(*((PlaceholderTokens::PlaceholderExpression*) tokens[i])), errorCode));
            }
            break;
          }
          } // switch(((PlaceholderPatternToken*)tokens[i])->kind)
        } // case PT_PLACEHOLDER
        } // switch (((PatternToken *)tokens[i])->kind) {
    } // for
    if (!markup.empty()) {
        ERROR(parseError, errorCode, index);
        return;
    }
}

void parsePatternParts(const UnicodeString &source, uint32_t &index, UParseError *parseError,
                       UErrorCode &errorCode, UVector& parts) {
  // parts: UVector<PatternToken>
  while(source[index] != '}') {  
    // Text or placeholder
    switch(source[index]) {
    case '{': {
      // Placeholder
      RETURN_IF_HELPER_FAILS(parseToken(LBRACE, source, index, parseError, errorCode));
      RETURN_IF_HELPER_FAILS(parseWhitespace(source, index, parseError, errorCode));
      // Expression or markup
      switch(source[index++]) {
        case '+': {
          UnicodeString markupName;
          uint32_t markupStartIndex = index;
          RETURN_IF_HELPER_FAILS(parseName(source, index, parseError, errorCode, markupName));
          UHashtable *options;
          RETURN_IF_HELPER_FAILS(options = uhash_open(uhash_hashChars, uhash_compareChars, nullptr, &errorCode));
          RETURN_IF_HELPER_FAILS(parseOptions(source, index, parseError, errorCode, *options));
          RETURN_IF_HELPER_FAILS(parseWhitespace(source, index, parseError, errorCode));
          RETURN_IF_HELPER_FAILS(parseToken(RBRACE, source, index, parseError, errorCode));
          ALLOCATION_ERROR_IF_HELPER_FAILS(parts.adoptElement(new PlaceholderTokens::MarkupStart(markupName, options, markupStartIndex), errorCode));
          break;
        }
        case '-': {
          UnicodeString markupName;
          uint32_t markupStartIndex = index;
          RETURN_IF_HELPER_FAILS(parseName(source, index, parseError, errorCode, markupName));
          RETURN_IF_HELPER_FAILS(parseWhitespace(source, index, parseError, errorCode));
          RETURN_IF_HELPER_FAILS(parseToken(RBRACE, source, index, parseError, errorCode));
          ALLOCATION_ERROR_IF_HELPER_FAILS(parts.adoptElement(new PlaceholderTokens::MarkupEnd(markupName, markupStartIndex), errorCode));
          break;
        }          
      default: {
        // Must be expression
        Expression expr;
        uint32_t exprStartIndex = index;
        RETURN_IF_HELPER_FAILS(parseExpression(source, index, parseError, errorCode, expr));
        RETURN_IF_HELPER_FAILS(parseWhitespace(source, index, parseError, errorCode));
        RETURN_IF_HELPER_FAILS(parseToken(RBRACE, source, index, parseError, errorCode));
        ALLOCATION_ERROR_IF_HELPER_FAILS(parts.adoptElement(new PlaceholderTokens::PlaceholderExpression(expr, exprStartIndex), errorCode));
        break;
      } // default
      } // switch(source[index])
      break; // case '{'
    }
  default: {
    // Text
    UnicodeString text;
    uint32_t textStartIndex = index;
    RETURN_IF_HELPER_FAILS(parseText(source, index, parseError, errorCode, text));
    ALLOCATION_ERROR_IF_HELPER_FAILS(parts.adoptElement(new TextPatternToken(text, textStartIndex), errorCode));
    break;
  } // default
  } // switch(source[index])
}  // while
} // parsePatternParts()

void parsePattern(const UnicodeString& source,
                  uint32_t &index,
                  UParseError *parseError,
                  UErrorCode& errorCode,
                  Pattern*& p) {
  RETURN_IF_HELPER_FAILS(parseToken(LBRACE, source, index, parseError, errorCode));
  RETURN_IF_HELPER_FAILS(parseWhitespace(source, index, parseError, errorCode));

  // For clarity, parse the pattern in two stages:
  // First, tokenize it into text and placeholders
  // Then create the tree representing nested markups

  ALLOCATION_ERROR_IF_HELPER_FAILS(UVector patternParts(errorCode));
  patternParts.setDeleter(uprv_deleteUObject);
  RETURN_IF_HELPER_FAILS(parsePatternParts(source, index, parseError, errorCode, patternParts));
  RETURN_IF_HELPER_FAILS(parseToken(RBRACE, source, index, parseError, errorCode));

  RETURN_IF_HELPER_FAILS(constructPatternTree(index, parseError, errorCode, patternParts, p));
}


void parseSelectors(const UnicodeString& source,
                    uint32_t &index,
                    UParseError *parseError,
                    UErrorCode& errorCode,
                    Selectors* & s) {
  RETURN_IF_HELPER_FAILS(parseToken(ID_MATCH, source, index, parseError, errorCode));
  RETURN_IF_HELPER_FAILS(parseWhitespace(source, index, parseError, errorCode));

  // At least one selector is required
  if (source[index] != '{') {
    errorCode = U_MESSAGE_PARSE_ERROR;
    return;
  }

  ALLOCATION_ERROR_IF_HELPER_FAILS(s = Selectors::create(errorCode));

  // Parse selectors
  while(source[index] == '{') {
    // Consume the '{'
    index++;
    RETURN_IF_HELPER_FAILS(parseWhitespace(source, index, parseError, errorCode));
    Expression* expr = new Expression();
    RETURN_IF_HELPER_FAILS(parseExpression(source, index, parseError, errorCode, *expr));
    ALLOCATION_ERROR_IF_HELPER_FAILS(s->addSelector(expr, errorCode));
    RETURN_IF_HELPER_FAILS(parseWhitespace(source, index, parseError, errorCode));
    RETURN_IF_HELPER_FAILS(parseToken(RBRACE, source, index, parseError, errorCode));
  }

  // Parse variants
  RETURN_IF_HELPER_FAILS(parseWhitespace(source, index, parseError, errorCode));

  // At least one variant is required
  if (!nextTokenIs(ID_WHEN, source, index)) {
    errorCode = U_MESSAGE_PARSE_ERROR;
    return;
  }

  while (nextTokenIs(ID_WHEN, source, index)) {
    // At least one key is required
    ALLOCATION_ERROR_IF_HELPER_FAILS(Keys* keys = Keys::createKeys(errorCode));
    RETURN_IF_HELPER_FAILS(parseNonEmptyKeys(source, index, parseError, errorCode, *keys));
    RETURN_IF_HELPER_FAILS(parseWhitespace(source, index, parseError, errorCode));
    Pattern* p;
    RETURN_IF_HELPER_FAILS(parsePattern(source, index, parseError, errorCode, p));
    ALLOCATION_ERROR_IF_HELPER_FAILS(s->addVariant(new Variant(keys, p), errorCode));
  }
  
  RETURN_IF_HELPER_FAILS(parseWhitespace(source, index, parseError, errorCode));
}
  
MessageFormatData::Body* parseBody(const UnicodeString& source,
                       uint32_t &index,
                       UParseError *parseError,
                       UErrorCode& errorCode) {
  // pattern or selectors
  switch (source[index]) {
    case '{': {
      // Pattern
      Pattern* p;
      RETURN_NULL_IF_HELPER_FAILS(parsePattern(source, index, parseError, errorCode, p));
      return new Message2Pattern(*p);
    }
    default: {
      if (nextTokenIs(ID_MATCH, source, index)) {
        Selectors* s = Selectors::create(errorCode);
        if (U_FAILURE(errorCode)) {
          return nullptr;
        }
        RETURN_NULL_IF_HELPER_FAILS(parseSelectors(source, index, parseError, errorCode, s));
        return s;
      } else {
        ERROR(parseError, errorCode, index);
        return nullptr;
      }
    }
  }
}

// -------------------------------------
// Parses the source string and returns the parsed representation,
// which is owned by the caller.

MessageFormatDataModel*
MessageFormat2::parse(const UnicodeString& source,
                      UParseError *parseError,
                      UErrorCode& errorCode) const
{

  uint32_t index = 0;
  RETURN_NULL_IF_HELPER_FAILS(parseWhitespace(source, index, parseError, errorCode));
  RETURN_NULL_IF_HELPER_FAILS(Declarations* declarations = parseDeclarations(source, index, parseError, errorCode));
  RETURN_NULL_IF_HELPER_FAILS(Body* body = parseBody(source, index, parseError, errorCode));
  RETURN_NULL_IF_HELPER_FAILS(parseWhitespace(source, index, parseError, errorCode));

  // Ensure that the entire input has been consumed
  if (((int32_t) index) != source.length() - 1) {
    errorCode = U_MESSAGE_PARSE_ERROR;
    return nullptr;
  }

  return new MessageFormatDataModel(*declarations, *body);
}


U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */
