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
#include "unicode/umachine.h"
#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/msgfmt2.h"
#include "uhash.h"
#include "uvector.h"

// *****************************************************************************
// class MessageFormat2
// *****************************************************************************

// Syntactically significant characters
#define LEFT_CURLY_BRACE  ((UChar)0x007B)
#define RIGHT_CURLY_BRACE ((UChar)0x007D)
#define SPACE             ((UChar)0x0020)
#define HTAB              ((UChar)0x0009)
#define CR                ((UChar)0x000D)
#define LF                ((UChar)0x000A)
#define BACKSLASH         ((UChar)0x005C)
#define PIPE              ((UChar)0x007C)
#define EQUALS            ((UChar)0x003D)
#define DOLLAR            ((UChar)0x0024)
#define COLON             ((UChar)0x003A)
#define PLUS              ((UChar)0x002B)
#define HYPHEN            ((UChar)0x002D)
#define PERIOD            ((UChar)0x002E)
#define UNDERSCORE        ((UChar)0x005F)

// Both used (in a `key` context) and reserved (in an annotation context)
#define ASTERISK          ((UChar)0x002A)

// Reserved sigils
#define BANG              ((UChar)0x0021)
#define AT                ((UChar)0x0040)
#define POUND             ((UChar)0x0023)
#define PERCENT           ((UChar)0x0025)
#define CARET             ((UChar)0x005E)
#define AMPERSAND         ((UChar)0x0026)
#define LESS_THAN         ((UChar)0x003C)
#define GREATER_THAN      ((UChar)0x003E)
#define QUESTION          ((UChar)0x003F)
#define TILDE             ((UChar)0x007E)


// TODO: indent to 4 spaces

// TODO: eliminate this and instead make each helper check the error code at the beginning
#define RETURN_IF_HELPER_FAILS(call) \
  call; \
  if (U_FAILURE(errorCode)) \
    return;

// TODO: for now we're only using U_MESSAGE_PARSE_ERROR.
// There are some other formatting error codes defined in common/unicode/utypes.h,
// but they don't really fit MessageFormat2.
#define ERROR(parseError, errorCode, index) \
  setParseError(parseError, index); \
  errorCode = U_MESSAGE_PARSE_ERROR;

#define CHECK_BOUNDS(source, index, parseError, errorCode) \
  if (!inBounds(source, index)) {\
    ERROR(parseError, errorCode, index);\
    return;\
  }

U_NAMESPACE_BEGIN

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(MessageFormat2)

static const UChar ID_LET[] = {
  0x6C, 0x65, 0x74, 0 /* "let" */
};

static const UChar ID_WHEN[] = {
  0x77, 0x68, 0x65, 0x6E, 0 /* "when" */
};

static const UChar ID_MATCH[] = {
  0x6D, 0x61, 0x74, 0x63, 0x68, 0 /* "match" */
};

// -------------------------------------
// Creates a MessageFormat instance based on the pattern.

MessageFormat2::MessageFormat2(const UnicodeString &pattern,
                               UParseError& parseError,
                               UErrorCode& success)
{
/**
TODO: For now, all this does is validate the pattern
**/
  parse(pattern, parseError, success);
}

MessageFormat2::~MessageFormat2() {}

// -------------------------------------
// Parses the source pattern

static void setParseError(UParseError& parseError, uint32_t index) {
    // See MessagePattern::setParseError(UParseError *parseError, int32_t index) {
    parseError.offset = index;
    parseError.line = 0;
    // TODO: fill this in with actual pre and post-context
    parseError.preContext[0] = 0;
    parseError.postContext[0] = 0;
}

static bool isWhitespace(UChar c) {
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

static bool inRange(UChar c, UChar first, UChar last) {
  U_ASSERT(first < last);
  return(c >= first && c <= last);
}

static bool inBounds(const UnicodeString& source, uint32_t index) {
  return(((int32_t) index) < source.length());
}

/*
  No pre, no post. (index == source.length()) may be true on exit
  This is because a message may end with whitespace
*/
static void parseWhitespaceMaybeRequired(bool required,
                                  const UnicodeString& source,
                                  uint32_t &index,
                                  UParseError& parseError,
                                  UErrorCode& errorCode) {
  bool sawWhitespace = false;

  // Use while(true) to make the bounds checking macro (which causes an
  // early return in the error case) usable here
  while(true) {
    if (!inBounds(source, index)) {
      // If whitespace isn't required -- or if we saw it already --
      // then the caller is responsible
      // for determining if we've used up all the input or not.
      if (!required || sawWhitespace) {
        // Not an error.
        return;
      }
      // Otherwise, whitespace is required; we haven't seen it yet;
      // and we're at the end of the input. Signal an error.
      ERROR(parseError, errorCode, index);
      return;
    }

    if (isWhitespace(source[index])) {
      sawWhitespace = true;
      // Increment line number in parse error if we consume a newline
      if (source[index] == LF) {
        (parseError.line)++;
      }
      index++;
    } else {
      break;
    }
  }

  if (!sawWhitespace && required) {
    ERROR(parseError, errorCode, index);
  }
}

/*
  No pre, no post
*/
static void parseRequiredWhitespace(const UnicodeString& source,
                             uint32_t &index,
                             UParseError& parseError,
                             UErrorCode& errorCode) {
  parseWhitespaceMaybeRequired(true, source, index, parseError, errorCode);
}

/*
  No pre, no post
*/
static void parseWhitespace(const UnicodeString& source,
                     uint32_t &index,
                     UParseError& parseError,
                     UErrorCode& errorCode) {
  parseWhitespaceMaybeRequired(false, source, index, parseError, errorCode);
}

/*
  No pre, no post (returns false when out of bounds)
*/
template<size_t N>
static bool nextTokenIs(const UChar (&token) [N], const UnicodeString& source, uint32_t index) {
  size_t tokenPos = 0;
  uint32_t strPos = index;

  while (tokenPos < N - 1) {
    if (!inBounds(source, strPos) || source[strPos] != token[tokenPos]) {
      return false;
    }
    strPos++;
    tokenPos++;
  }
  return true;
}

/*
  pre: index < source.length()
  post: none
*/
static void parseToken(UChar c, const UnicodeString &source, uint32_t &index, UParseError &parseError,
                UErrorCode &errorCode) {
    U_ASSERT(inBounds(source, index));
    if (source[index] == c) {
        index++;
        return;
    }
    ERROR(parseError, errorCode, index);
}

/*
  No pre, no post
*/
template<size_t N>
static void parseToken(const UChar (&token) [N],
                const UnicodeString& source,
                uint32_t &index,
                UParseError& parseError,
                UErrorCode& errorCode) {
  size_t tokenPos = 0;

  while (tokenPos < N - 1) {
    if (!inBounds(source, index) || source[index] != token[tokenPos]) {
      ERROR(parseError, errorCode, index);
      return;
    }
    index++;
    tokenPos++;
  }
}

/*
  No pre, no post
*/
template<size_t N>
static void parseTokenWithWhitespace(const UChar (&token) [N],
                              const UnicodeString& source,
                              uint32_t &index,
                              UParseError& parseError,
                              UErrorCode& errorCode) {
  RETURN_IF_HELPER_FAILS(parseWhitespace(source, index, parseError, errorCode));
  RETURN_IF_HELPER_FAILS(parseToken(token, source, index, parseError, errorCode));
  RETURN_IF_HELPER_FAILS(parseWhitespace(source, index, parseError, errorCode));
}

/*
  No pre, no post
*/
static void parseTokenWithWhitespace(UChar c,
                              const UnicodeString& source,
                              uint32_t &index,
                              UParseError& parseError,
                              UErrorCode& errorCode) {
  RETURN_IF_HELPER_FAILS(parseWhitespace(source, index, parseError, errorCode));
  RETURN_IF_HELPER_FAILS(parseToken(c, source, index, parseError, errorCode));
  RETURN_IF_HELPER_FAILS(parseWhitespace(source, index, parseError, errorCode));
}

static bool isAlpha(UChar c) {
  return (inRange(c, 0x0041, 0x005A)
          || inRange(c, 0x0061, 0x007A));
}

static bool isDigit(UChar c) {
  return (inRange(c, 0x0030, 0x0039));
}

static bool isNameStart(UChar c) {
  return (isAlpha(c)
          || c == UNDERSCORE
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
          // inRange(c, 0x10000, 0xEFFFF)); // Not sure about this -- spec says %x10000-EFFFF, but 0xEFFFF is more than the max UChar
}

static bool isNameChar(UChar c) {
  return (isNameStart(c)
          || isDigit(c)
          || c == HYPHEN
          || c == PERIOD
          || c == COLON
          || c == 0x00B7
          || inRange(c, 0x0300, 0x036F)
          || inRange(c, 0x203F, 0x2040));
}

/*
  Pre: index < source.length()
  Post: index < source.length()
    A message can't end with an nmtoken.
    So it's an error if parsing the nmtoken
    consumes all the input.
*/
static void parseNmtoken(const UnicodeString& source,
                  uint32_t &index,
                  UParseError& parseError,
                  UErrorCode& errorCode) {
  U_ASSERT(inBounds(source, index));
  if (!isNameChar(source[index])) {
    ERROR(parseError, errorCode, index);        
    return;
  }

  while (isNameChar(source[index])) {
    index++;
    CHECK_BOUNDS(source, index, parseError, errorCode);
  }
}

/*
  Pre: index < source.length()
  Post: index < source.length()
    A message can't end with an name.
    So it's an error if parsing the name
    consumes all the input.
*/
static void parseName(const UnicodeString& source,
               uint32_t &index,
               UParseError& parseError,
               UErrorCode& errorCode) {
  U_ASSERT(inBounds(source, index));

  if (!isNameStart(source[index])) {
    ERROR(parseError, errorCode, index);
    return;
  }

  RETURN_IF_HELPER_FAILS(parseNmtoken(source, index, parseError, errorCode));
}

/*
  Pre: index < source.length()
  Post: index < source.length()
    A message can't end with a variable name.
    So it's an error if parsing the variable name
    consumes all the input.
*/
static void parseVariableName(const UnicodeString& source,
                       uint32_t &index,
                       UParseError& parseError,
                       UErrorCode& errorCode) {
  U_ASSERT(inBounds(source, index));
  if (source[index] != DOLLAR) {
    ERROR(parseError, errorCode, index);
    return;
  }

  index++; // Consume the '$'
  CHECK_BOUNDS(source, index, parseError, errorCode);
  parseName(source, index, parseError, errorCode);
}

/*
  Pre: index < source.length()
*/
static bool isReservedStart(const UnicodeString &source, uint32_t index) {
    U_ASSERT(inBounds(source, index));
    switch (source[index]) {
    case BANG:
    case AT:
    case POUND:
    case PERCENT:
    case CARET:
    case AMPERSAND:
    case ASTERISK:
    case LESS_THAN:
    case GREATER_THAN:
    case QUESTION:
    case TILDE:
      return true;
    default:
      return false;
    }
}

/*
  Pre: index < source.length()
*/
static bool functionFollows(const UnicodeString &source, uint32_t index) {
  U_ASSERT(inBounds(source, index));
  switch (source[index]) {
  case COLON:
  case PLUS:
  case HYPHEN: {
    return true;
  }
  default: {
    return false;
  }
  }
}

/*
  Pre: index < source.length()
*/
static bool annotationFollows(const UnicodeString &source, uint32_t index) {
  return (functionFollows(source, index) || isReservedStart(source, index));
}

/*
  Pre: index < source.length()
  Post: index < source.length()
    A message can't end with a function name.
    So it's an error if parsing the function name
    consumes all the input.
*/
static void parseFunction(const UnicodeString& source,
                       uint32_t &index,
                       UParseError& parseError,
                       UErrorCode& errorCode) {
  U_ASSERT(inBounds(source, index));
  if (!functionFollows(source, index)) {
    ERROR(parseError, errorCode, index);
    return;
  }

  index++; // Consume the function start token
  CHECK_BOUNDS(source, index, parseError, errorCode);
  RETURN_IF_HELPER_FAILS(parseName(source, index, parseError, errorCode));
}

/*
  Pre: index < source.length()
  Post: index < source.length()
    Literals are delimited with '|'s.
    So it's an error if parsing a literal
    consumes all the input.
*/
static void parseLiteralEscape(const UnicodeString& source,
                        uint32_t &index,
                        UParseError& parseError,
                        UErrorCode& errorCode) {
  U_ASSERT(inBounds(source, index));
  U_ASSERT(source[index] == BACKSLASH);
  index++; // Skip the initial backslash
  CHECK_BOUNDS(source, index, parseError, errorCode);
  switch(source[index++]) {
  case BACKSLASH:
  case PIPE: {
    break;
  }
  default: {
    ERROR(parseError, errorCode, index);
    return;
  }
  }
  // Guarantee postcondition (this is useful because
  // we know we're expecting the closing '|')
  CHECK_BOUNDS(source, index, parseError, errorCode);
}

static bool isLiteralChar(UChar c) {
    return (inRange(c, 0x0000, 0x005B)    // Omit backslash
            || inRange(c, 0x005D, 0x007B) // Omit pipe
            || inRange(c, 0x007D, 0xD7FF) // Omit surrogates
            || c >= 0xE000);
//            || inRange(c, 0xE000, 0x10FFFF)); see comment in isNameStart()
}

/*
  Pre: index < source.length()
  Post: index < source.length()
    Literals are delimited with '|'s.
    So it's an error if parsing the contents of a literal
    consumes all the input.
*/
static void parseLiteralString(const UnicodeString &source, uint32_t &index, UParseError& parseError,
                        UErrorCode &errorCode) {
  U_ASSERT(inBounds(source, index));

  // while(true) makes it easier to use error checking macros
  while (true) {
    if(source[index] == BACKSLASH) {
      RETURN_IF_HELPER_FAILS(parseLiteralEscape(source, index, parseError, errorCode));
    } else if (!isLiteralChar(source[index])) {
        break;
    }
    index++;
    CHECK_BOUNDS(source, index, parseError, errorCode);
  }
}

/*
  Pre: index < source.length()
  Post: index < source.length()
    A message can't end with a literal.
    So it's an error if parsing a literal
    consumes all the input.
*/
static void parseLiteral(const UnicodeString &source, uint32_t &index, UParseError& parseError,
                  UErrorCode &errorCode) {
  U_ASSERT(inBounds(source, index));
  RETURN_IF_HELPER_FAILS(parseToken(PIPE, source, index, parseError, errorCode));

  CHECK_BOUNDS(source, index, parseError, errorCode);
  RETURN_IF_HELPER_FAILS(parseLiteralString(source, index, parseError, errorCode));

  CHECK_BOUNDS(source, index, parseError, errorCode);
  RETURN_IF_HELPER_FAILS(parseToken(PIPE, source, index, parseError, errorCode));

  // Guarantee postcondition
  CHECK_BOUNDS(source, index, parseError, errorCode);
}

/*
  Pre: index < source.length()
  Post: index < source.length()
    A message can't end with an option.
    So it's an error if parsing an option.
    consumes all the input.
*/
static void parseOption(const UnicodeString &source, uint32_t &index, UParseError& parseError,
                 UErrorCode &errorCode) {
  U_ASSERT(inBounds(source, index));
  RETURN_IF_HELPER_FAILS(parseName(source, index, parseError, errorCode));

  RETURN_IF_HELPER_FAILS(parseTokenWithWhitespace(EQUALS, source, index, parseError, errorCode));
  // Restore precondition
  CHECK_BOUNDS(source, index, parseError, errorCode);

  // literal | nmtoken | variable
  switch(source[index]) {
    case PIPE: {
      RETURN_IF_HELPER_FAILS(parseLiteral(source, index, parseError, errorCode));
      return;
    }
    case DOLLAR: {
      RETURN_IF_HELPER_FAILS(parseVariableName(source, index, parseError, errorCode));
      return;
    }
    default: {
      // must be nmtoken
      RETURN_IF_HELPER_FAILS(parseNmtoken(source, index, parseError, errorCode));
      return;
    }
  }
}

/*
  Pre: index < source.length()
  Post: index < source.length()
    A message can't end with an option.
    So it's an error if parsing an option.
    consumes all the input.
*/
static void parseOptions(const UnicodeString &source, uint32_t &index, UParseError& parseError,
                 UErrorCode &errorCode) {
  U_ASSERT(inBounds(source, index));
  // while(true) makes it easier to use the error checking macros
  while (true) {
    if (!isNameStart(source[index])) {
      break;
    }
    RETURN_IF_HELPER_FAILS(parseOption(source, index, parseError, errorCode));
    RETURN_IF_HELPER_FAILS(parseWhitespace(source, index, parseError, errorCode));
    // Restore precondition
    CHECK_BOUNDS(source, index, parseError, errorCode);
  }
}

/*
  Pre: index < source.length()
  Post: index < source.length()
    A message can't end with an annotation.
    So it's an error if parsing a `reserved` annotation
    consumes all the input.
*/
static void parseReservedEscape(const UnicodeString &source, uint32_t index, UParseError &parseError,
                         UErrorCode &errorCode) {
  U_ASSERT(inBounds(source, index));
  // Begins with two backslashes
  parseToken(BACKSLASH, source, index, parseError, errorCode);
  CHECK_BOUNDS(source, index, parseError, errorCode);
  parseToken(BACKSLASH, source, index, parseError, errorCode);
  CHECK_BOUNDS(source, index, parseError, errorCode);
  // Expect a '{', '|' or '}'
  switch (source[index]) {
  case LEFT_CURLY_BRACE:
  case RIGHT_CURLY_BRACE:
  case PIPE: {
        // Consume the character
      index++;
      // Guarantee postcondition
      CHECK_BOUNDS(source, index, parseError, errorCode);
      return;
    }
    default: {
      // No other characters are allowed here
      ERROR(parseError, errorCode, index);
      return;
    }
  }
}

static bool isReservedChar(UChar c) {
    return (inRange(c, 0x0000, 0x0008)     // Omit HTAB and LF
            || inRange(c, 0x000B, 0x000C)  // Omit CR
            || inRange(c, 0x000E, 0x0019)  // Omit SP
            || inRange(c, 0x0021, 0x005B)  // Omit backslash
            || inRange(c, 0x005D, 0x007A) // Omit { | }
            || inRange(c, 0x007E, 0xD7FF) // Omit surrogates
            || c >= 0xE000);
}

/*
  Pre: index < source.length()
  Post: index < source.length()
    A message can't end with an annotation.
    So it's an error if parsing a `reserved` annotation
    consumes all the input.
*/
static void parseReserved(const UnicodeString &source, uint32_t &index,
                   UParseError &parseError, UErrorCode &errorCode) {
    U_ASSERT(inBounds(source, index));
    if (!isReservedStart(source, index)) {
        ERROR(parseError, errorCode, index);
        return;
    }
    // Consume reservedStart
    index++;
    // Restore precondition
    CHECK_BOUNDS(source, index, parseError, errorCode);

    // Use while(true) to make it easier to use error checking macros
    while(true) {
      if (isReservedChar(source[index])) {
        // consume the char
        index++;
        // Guarantee postcondition
        CHECK_BOUNDS(source, index, parseError, errorCode);
      } else if (source[index] == BACKSLASH) {
        RETURN_IF_HELPER_FAILS(parseReservedEscape(source, index, parseError, errorCode));
      } else if (source[index] == PIPE) {
        RETURN_IF_HELPER_FAILS(parseLiteral(source, index, parseError, errorCode));
      } else {
        break;
      }
    }
}

/*
  Pre: index < source.length()
  Post: index < source.length()
    A message can't end with an annotation.
    So it's an error if parsing an annotation
    consumes all the input.
*/
static void parseAnnotation(const UnicodeString &source, uint32_t &index, UParseError &parseError,
                     UErrorCode &errorCode) {
    U_ASSERT(inBounds(source, index));
    if (functionFollows(source, index)) {
        // Function call
        RETURN_IF_HELPER_FAILS(parseFunction(source, index, parseError, errorCode));

        // Options, if present, must be preceded by whitespace
        if (isWhitespace(source[index])) {
          RETURN_IF_HELPER_FAILS(parseWhitespace(source, index, parseError, errorCode));
          // Restore precondition
          CHECK_BOUNDS(source, index, parseError, errorCode);
          RETURN_IF_HELPER_FAILS(parseOptions(source, index, parseError, errorCode));
          return;
        }
        // Otherwise, assume there are no options and return
        return;
    }
    // Must be reserved
    RETURN_IF_HELPER_FAILS(parseReserved(source, index, parseError, errorCode));
    return;
}

/*
  Pre: index < source.length()
  Post: index < source.length()
    A message can't end with an annotation or a literal.
    So it's an error if parsing a possibly-annotated literal
    consumes all the input.
*/
static void parseLiteralWithAnnotation(const UnicodeString& source,
                                uint32_t &index,
                                UParseError& parseError,
                                UErrorCode& errorCode) {
  U_ASSERT(inBounds(source, index));
  RETURN_IF_HELPER_FAILS(parseLiteral(source, index, parseError, errorCode));

  RETURN_IF_HELPER_FAILS(parseWhitespace(source, index, parseError, errorCode));
  // Restore precondition
  CHECK_BOUNDS(source, index, parseError, errorCode);

  if (annotationFollows(source, index)) {
    parseAnnotation(source, index, parseError, errorCode);
  }
}

/*
  Pre: index < source.length()
  Post: index < source.length()
    A message can't end with an annotation or a variable.
    So it's an error if parsing a possibly-annotated variable
    consumes all the input.
*/
static void parseVariableWithAnnotation(const UnicodeString &source, uint32_t &index, UParseError& parseError,
                                 UErrorCode &errorCode) {
  U_ASSERT(inBounds(source, index));
  RETURN_IF_HELPER_FAILS(parseVariableName(source, index, parseError, errorCode));

  RETURN_IF_HELPER_FAILS(parseWhitespace(source, index, parseError, errorCode));
  // Restore precondition
  CHECK_BOUNDS(source, index, parseError, errorCode);

  if (annotationFollows(source, index)) {
    parseAnnotation(source, index, parseError, errorCode);
  }
}

/*
  Pre: index < source.length()
  Post: index < source.length()
    A message can't end with an expression.
    So it's an error if parsing an expression
    consumes all the input.
*/
static void parseExpression(const UnicodeString &source, uint32_t &index, UParseError &parseError,
                     UErrorCode &errorCode) {
  U_ASSERT(inBounds(source, index));
  // Parse opening brace
  RETURN_IF_HELPER_FAILS(parseToken(LEFT_CURLY_BRACE, source, index, parseError, errorCode));
  // Optional whitespace after opening brace
  RETURN_IF_HELPER_FAILS(parseWhitespace(source, index, parseError, errorCode));
  // Restore precondition
  CHECK_BOUNDS(source, index, parseError, errorCode);
  // literal '|', variable '$' or annotation
  switch(source[index]) {
  case PIPE: {
    // Literal
    RETURN_IF_HELPER_FAILS(parseLiteralWithAnnotation(source, index, parseError, errorCode));
    break;
  }
  case DOLLAR: {
    // Variable
    RETURN_IF_HELPER_FAILS(parseVariableWithAnnotation(source, index, parseError, errorCode));
    break;
  }
  default: {
    if (annotationFollows(source, index)) {
      RETURN_IF_HELPER_FAILS(parseAnnotation(source, index, parseError, errorCode));
      break;
    }
    // Not a literal, variable or annotation -- error out
    ERROR(parseError, errorCode, index);
    return;
  }
  }
  // Optional whitespace before closing brace
  RETURN_IF_HELPER_FAILS(parseWhitespace(source, index, parseError, errorCode));
  // Restore precondition
  CHECK_BOUNDS(source, index, parseError, errorCode);
  // Parse closing brace
  RETURN_IF_HELPER_FAILS(parseToken(RIGHT_CURLY_BRACE, source, index, parseError, errorCode));
  // Guarantee postcondition
  CHECK_BOUNDS(source, index, parseError, errorCode);
}

/*
  Pre: none (declarations may be empty)
  Post: none (declarations may be empty)
*/
static void
parseDeclarations(const UnicodeString& source,
                  uint32_t &index,
                  UParseError& parseError,
                  UErrorCode& errorCode) {
  U_ASSERT(inBounds(source, index));

  while (nextTokenIs(ID_LET, source, index)) {
    RETURN_IF_HELPER_FAILS(parseToken(ID_LET, source, index, parseError, errorCode));
    // Restore precondition
    CHECK_BOUNDS(source, index, parseError, errorCode);
    RETURN_IF_HELPER_FAILS(parseRequiredWhitespace(source, index, parseError, errorCode));
    // Restore precondition
    CHECK_BOUNDS(source, index, parseError, errorCode);
    RETURN_IF_HELPER_FAILS(parseVariableName(source, index, parseError, errorCode));
    RETURN_IF_HELPER_FAILS(parseTokenWithWhitespace(EQUALS, source, index, parseError, errorCode));
    // Restore precondition
    CHECK_BOUNDS(source, index, parseError, errorCode);
    RETURN_IF_HELPER_FAILS(parseExpression(source, index, parseError, errorCode));
    RETURN_IF_HELPER_FAILS(parseWhitespace(source, index, parseError, errorCode));
    // Restore precondition
    CHECK_BOUNDS(source, index, parseError, errorCode);
  }
}

static bool isTextChar(UChar c) {
    return (inRange(c, 0x0000, 0x005B)    // Omit backslash
            || inRange(c, 0x005D, 0x007A) // Omit {
            || c == 0x007C                // }
            || inRange(c, 0x007E, 0xD7FF) // Omit surrogates
            || c >= 0xE000); 
 //           || inRange(c, 0xE000, 0x10FFFF));                                      
}

/*
  Pre: index < source.length()
  Post: index < source.length()
    A message can't end with a text string.
    So it's an error if parsing a text string
    consumes all the input.
*/
static void parseTextEscape(const UnicodeString &source, uint32_t &index,
                     UParseError& parseError,
                     UErrorCode& errorCode) {
  U_ASSERT(inBounds(source, index));
  U_ASSERT(source[index] == BACKSLASH);
  index++; // Skip the initial backslash
  CHECK_BOUNDS(source, index, parseError, errorCode);
  switch(source[index++]) {
  case BACKSLASH:
  case LEFT_CURLY_BRACE:
  case RIGHT_CURLY_BRACE: {
    break;
  }
  default: {
    ERROR(parseError, errorCode, index);
    break;
  }
  }
  // Guarantee postcondition
  CHECK_BOUNDS(source, index, parseError, errorCode);
}

/*
  Pre: index < source.length()
  Post: index < source.length()
    A message can't end with a text string.
    So it's an error if parsing a text string
    consumes all the input.
*/
static void parseText(const UnicodeString &source, uint32_t &index,
               UParseError& parseError,
               UErrorCode& errorCode) {
  U_ASSERT(inBounds(source, index));
  bool empty = true;

  // while(true) makes the error handling macros easier
  while(true) {
    if (source[index] == BACKSLASH) {
      RETURN_IF_HELPER_FAILS(parseTextEscape(source, index, parseError, errorCode));
    } else if (isTextChar(source[index])) {
      // Restore precondition
      index++;
      CHECK_BOUNDS(source, index, parseError, errorCode);
    } else {
      break;
    }
    empty = false;
  }

  if (empty) {
    // text must be non-empty
    ERROR(parseError, errorCode, index);
  }
}

/*
  Pre: index < source.length()
  Post: index < source.length()
    A message can't end with a key.
    So it's an error if parsing a key.
    consumes all the input.
*/
static void parseKey(const UnicodeString& source,
              uint32_t &index,
              UParseError& parseError,
              UErrorCode& errorCode) {
  U_ASSERT(inBounds(source, index));

  // Literal | nmtoken | '*'
  switch (source[index]) {
    case PIPE: {
      RETURN_IF_HELPER_FAILS(parseLiteral(source, index, parseError, errorCode));
      break;
    }
    case ASTERISK: {
      index++;
      // Guarantee postcondition
      CHECK_BOUNDS(source, index, parseError, errorCode);
      break;
    }
    default: {
      // nmtoken
      RETURN_IF_HELPER_FAILS(parseNmtoken(source, index, parseError, errorCode));
      break;
    }
  }
}

/*
  Pre: index < source.length()
  Post: index < source.length()
    A message can't end with a key.
    So it's an error if parsing a key.
    consumes all the input.
*/
static void parseNonEmptyKeys(const UnicodeString& source,
                       uint32_t &index,
                       UParseError& parseError,
                       UErrorCode& errorCode) {
  U_ASSERT(inBounds(source, index));

  // Need parseRequiredWhitespace before the first key...
  RETURN_IF_HELPER_FAILS(parseRequiredWhitespace(source, index, parseError, errorCode));
  // Restore precondition
  CHECK_BOUNDS(source, index, parseError, errorCode);
  RETURN_IF_HELPER_FAILS(parseKey(source, index, parseError, errorCode));

  // Before all other keys, it's ambiguous whether whitespace is the required whitespace
  // before `key` or the optional whitespace before `pattern`. So instead, we exit from
  // the loop early if the whitespace is missing, possibly causing the parse error to
  // be reported later
  while (isWhitespace(source[index])) {
    RETURN_IF_HELPER_FAILS(parseWhitespace(source, index, parseError, errorCode));
    // Restore precondition
    CHECK_BOUNDS(source, index, parseError, errorCode);

    if (source[index] == LEFT_CURLY_BRACE) {
      // then we're done parsing the keys and a pattern follows -- break
      return;
    }
    RETURN_IF_HELPER_FAILS(parseKey(source, index, parseError, errorCode));
  }
}

/*
  Pre: index < source.length()
  Post: none (a message can end with a pattern)
*/
static void parsePattern(const UnicodeString& source,
                  uint32_t &index,
                  UParseError& parseError,
                  UErrorCode& errorCode) {
  U_ASSERT(inBounds(source, index));

  RETURN_IF_HELPER_FAILS(parseToken(LEFT_CURLY_BRACE, source, index, parseError, errorCode));
  while (source[index] != RIGHT_CURLY_BRACE) {
    switch (source[index]) {
    case LEFT_CURLY_BRACE: {
      // Must be expression
      RETURN_IF_HELPER_FAILS(parseExpression(source, index, parseError, errorCode));
      break;
    }
    default: {
      // Must be text
      RETURN_IF_HELPER_FAILS(parseText(source, index, parseError, errorCode));
      break;
    }
    }
  }
  // Consume the closing brace
  index++;
}

/*
  Pre: index < source.length()
  Post: none
    A message can end with a `selectors`.
*/
static void parseSelectors(const UnicodeString& source,
                    uint32_t &index,
                    UParseError& parseError,
                    UErrorCode& errorCode) {
  U_ASSERT(inBounds(source, index));

  RETURN_IF_HELPER_FAILS(parseToken(ID_MATCH, source, index, parseError, errorCode));
  RETURN_IF_HELPER_FAILS(parseWhitespace(source, index, parseError, errorCode));
  // Restore precondition
  CHECK_BOUNDS(source, index, parseError, errorCode);


  bool selectorsEmpty = true;
  // Parse selectors
  while(source[index] == LEFT_CURLY_BRACE) {
    selectorsEmpty = false;
    RETURN_IF_HELPER_FAILS(parseWhitespace(source, index, parseError, errorCode));
    // Restore precondition
    CHECK_BOUNDS(source, index, parseError, errorCode);

    RETURN_IF_HELPER_FAILS(parseExpression(source, index, parseError, errorCode));
    RETURN_IF_HELPER_FAILS(parseWhitespace(source, index, parseError, errorCode));
    // Restore precondition
    CHECK_BOUNDS(source, index, parseError, errorCode);
  }

  // At least one selector is required
  if (selectorsEmpty) {
    ERROR(parseError, errorCode, index);
    return;
  }

  // Parse variants
  RETURN_IF_HELPER_FAILS(parseWhitespace(source, index, parseError, errorCode));
  // Restore precondition
  CHECK_BOUNDS(source, index, parseError, errorCode);

  bool empty = true;

  while (nextTokenIs(ID_WHEN, source, index)) {
    empty = false;
    // Consume the "when"
    parseToken(ID_WHEN, source, index, parseError, errorCode);
    // Restore precondition
    CHECK_BOUNDS(source, index, parseError, errorCode);

    // At least one key is required
    RETURN_IF_HELPER_FAILS(parseNonEmptyKeys(source, index, parseError, errorCode));
    RETURN_IF_HELPER_FAILS(parseWhitespace(source, index, parseError, errorCode));
    // Restore precondition
    CHECK_BOUNDS(source, index, parseError, errorCode);

    RETURN_IF_HELPER_FAILS(parsePattern(source, index, parseError, errorCode));
    RETURN_IF_HELPER_FAILS(parseWhitespace(source, index, parseError, errorCode));

    // Special case: we've consumed all the input
    if (((int32_t) index) >= source.length()) {
      break;
    }
    // Restore precondition
    CHECK_BOUNDS(source, index, parseError, errorCode);
  }

  // At least one variant is required
  if (empty) {
    ERROR(parseError, errorCode, index);
  }
}

/*
  Pre: index < source.length()
  Post: none
    A message can end with a body (whitespace is optional).
*/
static void parseBody(const UnicodeString& source,
               uint32_t &index,
               UParseError& parseError,
               UErrorCode& errorCode) {
  U_ASSERT(inBounds(source, index));

  // pattern or selectors
  switch (source[index]) {
    case LEFT_CURLY_BRACE: {
      // Pattern
      RETURN_IF_HELPER_FAILS(parsePattern(source, index, parseError, errorCode));
      return;
    }
    default: {
      if (nextTokenIs(ID_MATCH, source, index)) {
        RETURN_IF_HELPER_FAILS(parseSelectors(source, index, parseError, errorCode));
      } else {
        ERROR(parseError, errorCode, index);
      }
      return;
    }
  }
}

MessageFormat2::MessageFormat2(const MessageFormat2&) {}

// -------------------------------------
// Creates a copy of this MessageFormat2; the caller owns the copy.

MessageFormat2*
MessageFormat2::clone() const
{
    return new MessageFormat2(*this);
}



// Not yet implemented
bool MessageFormat2::operator==(const Format& other) const { return (this == &other); }
// Not yet implemented
bool MessageFormat2::operator!=(const Format& other) const { return (this != &other); }

// Not yet implemented
UnicodeString&
MessageFormat2::format(const Formattable&,
                      UnicodeString& appendTo,
                      FieldPosition&,
                      UErrorCode& status) const {
  status = U_UNSUPPORTED_ERROR;
  return appendTo;
}

// Not yet implemented
void
MessageFormat2::parseObject(const UnicodeString&,
                            Formattable&,
                            ParsePosition& status) const {
  status = U_UNSUPPORTED_ERROR;
}

// -------------------------------------
// Parses the source string and returns the parsed representation,
// which is owned by the caller.

/*** TODO

For now, this is only a validator (it doesn't build a data model).
***/
void MessageFormat2::parse(const UnicodeString& source,
                      UParseError& parseError,
                      UErrorCode& errorCode) const
{
  // Return immediately in the case of a previous error
  if (U_FAILURE(errorCode)) {
    return;
  }
  uint32_t index = 0;
  parseError.line = 0;
  // Whitespace is optional and declarations can be empty; don't check bounds
  // yet
  RETURN_IF_HELPER_FAILS(parseWhitespace(source, index, parseError, errorCode));
  RETURN_IF_HELPER_FAILS(parseDeclarations(source, index, parseError, errorCode));
  // Do check bounds here, because body must be non-empty
  CHECK_BOUNDS(source, index, parseError, errorCode);
  RETURN_IF_HELPER_FAILS(parseBody(source, index, parseError, errorCode));
  RETURN_IF_HELPER_FAILS(parseWhitespace(source, index, parseError, errorCode));

  // Ensure that the entire input has been consumed
  if (((int32_t) index) != source.length()) {
    ERROR(parseError, errorCode, index);
  }
}

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */
