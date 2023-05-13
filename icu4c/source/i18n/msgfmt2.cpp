// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/msgfmt2.h"
#include "uvector.h" // U_ASSERT

// Syntactically significant characters
#define LEFT_CURLY_BRACE ((UChar)0x007B)
#define RIGHT_CURLY_BRACE ((UChar)0x007D)
#define SPACE ((UChar)0x0020)
#define HTAB ((UChar)0x0009)
#define CR ((UChar)0x000D)
#define LF ((UChar)0x000A)
#define BACKSLASH ((UChar)0x005C)
#define PIPE ((UChar)0x007C)
#define EQUALS ((UChar)0x003D)
#define DOLLAR ((UChar)0x0024)
#define COLON ((UChar)0x003A)
#define PLUS ((UChar)0x002B)
#define HYPHEN ((UChar)0x002D)
#define PERIOD ((UChar)0x002E)
#define UNDERSCORE ((UChar)0x005F)

// Both used (in a `key` context) and reserved (in an annotation context)
#define ASTERISK ((UChar)0x002A)

// Reserved sigils
#define BANG ((UChar)0x0021)
#define AT ((UChar)0x0040)
#define POUND ((UChar)0x0023)
#define PERCENT ((UChar)0x0025)
#define CARET ((UChar)0x005E)
#define AMPERSAND ((UChar)0x0026)
#define LESS_THAN ((UChar)0x003C)
#define GREATER_THAN ((UChar)0x003E)
#define QUESTION ((UChar)0x003F)
#define TILDE ((UChar)0x007E)


// TODO: rename files

U_NAMESPACE_BEGIN

// MessageFormat2 uses three keywords: `let`, `when`, and `match`.

static const UChar ID_LET[] = {
    0x6C, 0x65, 0x74, 0 /* "let" */
};

static const UChar ID_WHEN[] = {
    0x77, 0x68, 0x65, 0x6E, 0 /* "when" */
};

static const UChar ID_MATCH[] = {
    0x6D, 0x61, 0x74, 0x63, 0x68, 0 /* "match" */
};

UOBJECT_DEFINE_RTTI_IMPLEMENTATION(MessageFormat2)

/*
    The `ERROR()` macro sets `errorCode` to `U_MESSAGE_PARSE_ERROR
    and sets the offset in `parseError` to `index`. It does not alter control flow.

    For now, all parse errors are denoted by U_MESSAGE_PARSE_ERROR.
    common/unicode/utypes.h defines a broader set of formatting errors,
    but it doesn't capture all possible MessageFormat2 errors and until the
    spec is finalized, we'll just use the same error code for all parse errors.
*/
#define ERROR(parseError, errorCode, index)                                                             \
    setParseError(parseError, index);                                                                   \
    errorCode = U_MESSAGE_PARSE_ERROR;


// Returns immediately if `errorCode` indicates failure
#define CHECK_ERROR(errorCode)                                                                          \
    if (U_FAILURE(errorCode)) {                                                                         \
        return;                                                                                         \
    }

// Returns true iff `index` is a valid index for the string `source`
static bool inBounds(const UnicodeString &source, uint32_t index) {
    return (((int32_t)index) < source.length());
}

/*
    Signals an error and returns either if `parseError` already denotes an
    error, or `index` is out of bounds for the string `source`
*/
#define CHECK_BOUNDS(source, index, parseError, errorCode)                                              \
    if (U_FAILURE(errorCode)) {                                                                         \
        return;                                                                                         \
    }                                                                                                   \
    if (!inBounds(source, index)) {                                                                     \
        ERROR(parseError, errorCode, index);                                                            \
        return;                                                                                         \
    }

// -------------------------------------
// Creates a MessageFormat instance based on the pattern.

MessageFormat2::MessageFormat2(const UnicodeString &pattern, UParseError &parseError,
                               UErrorCode &success) {
    /**
    TODO: For now, all this constructor does is validate the pattern.
    **/
    CHECK_ERROR(success);

    parse(pattern, parseError, success);
}

MessageFormat2::~MessageFormat2() {}

// -------------------------------------
// Helper functions

static void setParseError(UParseError &parseError, uint32_t index) {
    parseError.offset = index;
    // TODO: fill this in with actual pre and post-context
    parseError.preContext[0] = 0;
    parseError.postContext[0] = 0;
}

// -------------------------------------
// Predicates

// Returns true if `c` is in the interval [`first`, `last`]
static bool inRange(UChar c, UChar first, UChar last) {
    U_ASSERT(first < last);
    return (c >= first && c <= last);
}

// See `s` in the MessageFormat2 grammar
static bool isWhitespace(UChar c) {
    switch (c) {
    case SPACE:
    case HTAB:
    case CR:
    case LF:
        return true;
    default:
        return false;
    }
}

/*
  The following helper predicates should exactly match nonterminals in the MessageFormat2 grammar:

  `isTextChar()`      : `text-char`
  `isReservedStart()` : `reserved-start`
  `isReservedChar()`  : `reserved-char`
  `isAlpha()`         : `ALPHA`
  `isDigit()`         : `DIGIT`
  `isNameStart()`     : `name-start`
  `isNameChar()`      : `name-char`
  `isLiteralChar()`   : `literal-char`
*/
static bool isTextChar(UChar c) {
    return (inRange(c, 0x0000, 0x005B)    // Omit backslash
            || inRange(c, 0x005D, 0x007A) // Omit {
            || c == 0x007C                // }
            || inRange(c, 0x007E, 0xD7FF) // Omit surrogates
            || c >= 0xE000);
    //           || inRange(c, 0xE000, 0x10FFFF));
}

static bool isReservedStart(UChar c) {
    switch (c) {
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

static bool isReservedChar(UChar c) {
    return (inRange(c, 0x0000, 0x0008)    // Omit HTAB and LF
            || inRange(c, 0x000B, 0x000C) // Omit CR
            || inRange(c, 0x000E, 0x0019) // Omit SP
            || inRange(c, 0x0021, 0x005B) // Omit backslash
            || inRange(c, 0x005D, 0x007A) // Omit { | }
            || inRange(c, 0x007E, 0xD7FF) // Omit surrogates
            || c >= 0xE000);
}

static bool isAlpha(UChar c) { return (inRange(c, 0x0041, 0x005A) || inRange(c, 0x0061, 0x007A)); }

static bool isDigit(UChar c) { return (inRange(c, 0x0030, 0x0039)); }

static bool isNameStart(UChar c) {
    return (isAlpha(c) || c == UNDERSCORE || inRange(c, 0x00C0, 0x00D6) || inRange(c, 0x00D8, 0x00F6) ||
            inRange(c, 0x00F8, 0x02FF) || inRange(c, 0x0370, 0x037D) || inRange(c, 0x037F, 0x1FFF) ||
            inRange(c, 0x200C, 0x200D) || inRange(c, 0x2070, 0x218F) || inRange(c, 0x2C00, 0x2FEF) ||
            inRange(c, 0x3001, 0xD7FF) || inRange(c, 0xF900, 0xFDCF) || inRange(c, 0xFDF0, 0xFFFD));
    // inRange(c, 0x10000, 0xEFFFF)); // Not sure about this -- spec says %x10000-EFFFF, but 0xEFFFF is
    // more than the max UChar
    // TODO
}

static bool isNameChar(UChar c) {
    return (isNameStart(c) || isDigit(c) || c == HYPHEN || c == PERIOD || c == COLON || c == 0x00B7 ||
            inRange(c, 0x0300, 0x036F) || inRange(c, 0x203F, 0x2040));
}

static bool isLiteralChar(UChar c) {
    return (inRange(c, 0x0000, 0x005B)    // Omit backslash
            || inRange(c, 0x005D, 0x007B) // Omit pipe
            || inRange(c, 0x007D, 0xD7FF) // Omit surrogates
            || c >= 0xE000);
    //            || inRange(c, 0xE000, 0x10FFFF)); see comment in isNameStart()
}

// Returns true iff `c` can begin a `function` nonterminal
static bool isFunctionStart(UChar c) {
    switch (c) {
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

// Returns true iff `c` can begin an `annotation` nonterminal
static bool isAnnotationStart(UChar c) {
    return isFunctionStart(c) || isReservedStart(c);
}

// -------------------------------------
// Parsing functions

/*
    Most functions match a non-terminal in the grammar, except as explained
    in comments.

    Unless otherwise noted in a comment, all helper functions that take
    a `source` string, an `index` unsigned int, and an `errorCode` `UErrorCode`
    have the precondition:
      `index` < `source.length()`
    and the postcondition:
      `U_FAILURE(errorCode)` || `index < `source.length()`
*/

/*
  No pre, no post.
  A message may end with whitespace, so `index` may equal `source.length()` on exit.
*/
static void parseWhitespaceMaybeRequired(bool required,
                                         const UnicodeString &source,
                                         uint32_t &index,
                                         UParseError &parseError,
                                         UErrorCode &errorCode) {
    CHECK_ERROR(errorCode);

    bool sawWhitespace = false;

    // The loop exits either when we consume all the input,
    // or when we see a non-whitespace character.
    while (true) {
        // Check if all input has been consumed
        if (!inBounds(source, index)) {
            // If whitespace isn't required -- or if we saw it already --
            // then the caller is responsible for checking this case and
            // setting an error if necessary.
            if (!required || sawWhitespace) {
                // Not an error.
                return;
            }
            // Otherwise, whitespace is required; the end of the input has
            // been reached without whitespace. This is an error.
            ERROR(parseError, errorCode, index);
            return;
        }

        // Input remains; process the next character if it's whitespace,
        // exit the loop otherwise
        if (isWhitespace(source[index])) {
            sawWhitespace = true;
            // Increment line number in parse error if we consume a newline
            if (source[index] == LF) {
                parseError.line++;
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
  No pre, no post, for the same reason as `parseWhitespaceMaybeRequired()`.
*/
static void parseRequiredWhitespace(const UnicodeString &source,
                                    uint32_t &index,
                                    UParseError &parseError,
                                    UErrorCode &errorCode) {
    parseWhitespaceMaybeRequired(true, source, index, parseError, errorCode);
}

/*
  No pre, no post, for the same reason as `parseWhitespaceMaybeRequired()`.
*/
static void parseOptionalWhitespace(const UnicodeString &source,
                                    uint32_t &index,
                                    UParseError &parseError,
                                    UErrorCode &errorCode) {
    parseWhitespaceMaybeRequired(false, source, index, parseError, errorCode);
}

// Consumes a single character, signaling an error if `source[index]` != `c`
static void parseToken(UChar c,
                       const UnicodeString &source,
                       uint32_t &index,
                       UParseError &parseError,
                       UErrorCode &errorCode) {
    CHECK_ERROR(errorCode);

    U_ASSERT(inBounds(source, index));
    if (source[index] == c) {
        index++;
        // Guarantee postcondition
        CHECK_BOUNDS(source, index, parseError, errorCode);
        return;
    }
    // Next character didn't match -- error out
    ERROR(parseError, errorCode, index);
}

/*
   Consumes a fixed-length token, signaling an error if the token isn't a prefix of
   the string beginning at `source[index]`
*/
template <size_t N>
static void parseToken(const UChar (&token)[N],
                       const UnicodeString &source,
                       uint32_t &index,
                       UParseError &parseError,
                       UErrorCode &errorCode) {
    CHECK_ERROR(errorCode);

    U_ASSERT(inBounds(source, index));

    size_t tokenPos = 0;
    while (tokenPos < N - 1) {
        if (source[index] != token[tokenPos]) {
            ERROR(parseError, errorCode, index);
            return;
        }
        index++;
        // Guarantee postcondition
        CHECK_BOUNDS(source, index, parseError, errorCode);

        tokenPos++;
    }
}

/*
   Consumes optional whitespace, possibly advancing `index` to `index'`,
   then consumes a fixed-length token (signaling an error if the token isn't a prefix of
   the string beginning at `source[index']`),
   then consumes optional whitespace again
*/
template <size_t N>
static void parseTokenWithWhitespace(const UChar (&token)[N],
                                     const UnicodeString &source,
                                     uint32_t &index,
                                     UParseError &parseError,
                                     UErrorCode &errorCode) {
    // No need for error check or bounds check before parseOptionalWhitespace
    parseOptionalWhitespace(source, index, parseError, errorCode);
    // Establish precondition
    CHECK_BOUNDS(source, index, parseError, errorCode);
    parseToken(token, source, index, parseError, errorCode);
    parseOptionalWhitespace(source, index, parseError, errorCode);
    // Guarantee postcondition
    CHECK_BOUNDS(source, index, parseError, errorCode);
}

/*
   Consumes optional whitespace, possibly advancing `index` to `index'`,
   then consumes a single character (signaling an error if it doesn't match
   `source[index']`),
   then consumes optional whitespace again
*/
static void parseTokenWithWhitespace(UChar c,
                                     const UnicodeString &source,
                                     uint32_t &index,
                                     UParseError &parseError,
                                     UErrorCode &errorCode) {
    // No need for error check or bounds check before parseOptionalWhitespace
    parseOptionalWhitespace(source, index, parseError, errorCode);
    // Establish precondition
    CHECK_BOUNDS(source, index, parseError, errorCode);
    parseToken(c, source, index, parseError, errorCode);
    parseOptionalWhitespace(source, index, parseError, errorCode);
    // Guarantee postcondition
    CHECK_BOUNDS(source, index, parseError, errorCode);
}

/*
  Consumes a non-empty sequence of `name-char`s.

  (Matches the `nmtoken` nonterminal in the grammar.)
*/
static void parseNmtoken(const UnicodeString &source,
                         uint32_t &index,
                         UParseError &parseError,
                         UErrorCode &errorCode) {
    CHECK_ERROR(errorCode);

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
  Consumes a non-empty sequence of `name-char`s, the first of which is
  also a `name-start`.
  that begins with a character `start` such that `isNameStart(start)`.

  (Matches the `name` nonterminal in the grammar.)
*/
static void parseName(const UnicodeString &source,
                      uint32_t &index,
                      UParseError &parseError,
                      UErrorCode &errorCode) {
    CHECK_ERROR(errorCode);

    U_ASSERT(inBounds(source, index));

    if (!isNameStart(source[index])) {
        ERROR(parseError, errorCode, index);
        return;
    }

    parseNmtoken(source, index, parseError, errorCode);
}

/*
  Consumes a '$' followed by a `name`.

  (Matches the `variable` nonterminal in the grammar.)
*/
static void parseVariableName(const UnicodeString &source,
                              uint32_t &index,
                              UParseError &parseError,
                              UErrorCode &errorCode) {
    CHECK_ERROR(errorCode);

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
  Consumes a reference to a function, matching the `function` nonterminal in
  the grammar.
*/
static void parseFunction(const UnicodeString &source,
                          uint32_t &index,
                          UParseError &parseError,
                          UErrorCode &errorCode) {
    CHECK_ERROR(errorCode);

    U_ASSERT(inBounds(source, index));
    if (!isFunctionStart(source[index])) {
        ERROR(parseError, errorCode, index);
        return;
    }

    index++; // Consume the function start token
    CHECK_BOUNDS(source, index, parseError, errorCode);
    parseName(source, index, parseError, errorCode);
}

/*
  Consumes a literal, matching the `literal` nonterminal in the grammar.
*/
static void parseLiteral(const UnicodeString &source,
                         uint32_t &index,
                         UParseError &parseError,
                         UErrorCode &errorCode) {
    CHECK_ERROR(errorCode);
    U_ASSERT(inBounds(source, index));

    // Parse the opening '|'
    parseToken(PIPE, source, index, parseError, errorCode);
    CHECK_BOUNDS(source, index, parseError, errorCode);

    // Parse the contents
    bool done = false;
    while (!done) {
        if (source[index] == BACKSLASH) {
          // '\' begins a literal-escape; the following character must be
          // either another '\' or a '|' as per the `literal-escape` nonterminal
          index++; // Consume the initial '\'
          CHECK_BOUNDS(source, index, parseError, errorCode);
          if (source[index] == BACKSLASH || source[index] == PIPE) {
            index++; // Valid escape character; consume iterate
          } else {
            ERROR(parseError, errorCode, index);
            return;
          }
        } else if (isLiteralChar(source[index])) {
            index++; // Consume this character
        } else {
          // Assume the sequence of literal characters ends here
          done = true;
        }
        CHECK_BOUNDS(source, index, parseError, errorCode);
    }

    // Parse the closing '|'
    parseToken(PIPE, source, index, parseError, errorCode);

    // Guarantee postcondition
    CHECK_BOUNDS(source, index, parseError, errorCode);
}

/*
  Consume a name-value pair, matching the `option` nonterminal in the grammar.
*/
static void parseOption(const UnicodeString &source,
                        uint32_t &index,
                        UParseError &parseError,
                        UErrorCode &errorCode) {
    CHECK_ERROR(errorCode);

    U_ASSERT(inBounds(source, index));

    // Parse LHS
    parseName(source, index, parseError, errorCode);

    // Parse '='
    parseTokenWithWhitespace(EQUALS, source, index, parseError, errorCode);

    // Parse RHS, which is either a literal, nmtoken, or variable
    switch (source[index]) {
    case PIPE: {
        parseLiteral(source, index, parseError, errorCode);
        break;
    }
    case DOLLAR: {
        parseVariableName(source, index, parseError, errorCode);
        break;
    }
    default: {
        // Not a literal or variable, so it must be an nmtoken
        parseNmtoken(source, index, parseError, errorCode);
        break;
    }
    }
}

/*
  Consume optional whitespace followed by a sequence of options
  (possibly empty), separated by whitespace
*/
static void parseOptions(const UnicodeString &source,
                         uint32_t &index,
                         UParseError &parseError,
                         UErrorCode &errorCode) {
    CHECK_ERROR(errorCode);

    U_ASSERT(inBounds(source, index));

    // Start with parsing optional whitespace, in case of an
    // empty options list that doesn't have trailing whitespace
    parseOptionalWhitespace(source, index, parseError, errorCode);

    while(true) {
      // Restore precondition
      CHECK_BOUNDS(source, index, parseError, errorCode);
      if (!isNameStart(source[index])) {
        // We've consumed all the options (and there may have been 0 options).
        // Done.
        break;
      }
      parseOption(source, index, parseError, errorCode);
      if (!isWhitespace(source[index])) {
        break; // Assume we've consumed all the options
      }
      parseOptionalWhitespace(source, index, parseError, errorCode);
    }
}

/*
  Consume an escaped curly brace or pipe, matching the `reserved-escape` nonterminal
  in the grammar
*/
static void parseReservedEscape(const UnicodeString &source,
                                uint32_t index,
                                UParseError &parseError,
                                UErrorCode &errorCode) {
    CHECK_ERROR(errorCode);

    U_ASSERT(inBounds(source, index));
    // Begins with two backslashes
    parseToken(BACKSLASH, source, index, parseError, errorCode);
    parseToken(BACKSLASH, source, index, parseError, errorCode);

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

/*
  Consume a `reserved-start` character followed by a possibly-empty sequence of reserved
  characters. Matches the `reserved` nonterminal in the grammar
*/
static void parseReserved(const UnicodeString &source,
                          uint32_t &index,
                          UParseError &parseError,
                          UErrorCode &errorCode) {
    CHECK_ERROR(errorCode);

    U_ASSERT(inBounds(source, index));

    // Require a `reservedStart` character
    if (!isReservedStart(source[index])) {
        ERROR(parseError, errorCode, index);
        return;
    }
    // Consume reservedStart
    index++;
    // Restore precondition
    CHECK_BOUNDS(source, index, parseError, errorCode);

    // Consume reserved characters / literals / reserved escapes
    // until a character that can't be in a `reserved-body` is seen
    while (true) {
        if (isReservedChar(source[index])) {
            // consume the char
            index++;
            // Guarantee postcondition
            CHECK_BOUNDS(source, index, parseError, errorCode);
        } else {
          if (source[index] == BACKSLASH) {
            parseReservedEscape(source, index, parseError, errorCode);
          } else if (source[index] == PIPE) {
            parseLiteral(source, index, parseError, errorCode);
          } else {
            // The reserved sequence ends here
            break; 
          }
          // Needed so we exit the loop immediately if parsing the escape
          // sequence or literal failed
          CHECK_ERROR(errorCode);
        }
    }
}


/*
  Consume a function call or reserved string, matching the `annotation`
  nonterminal in the grammar
*/
static void parseAnnotation(const UnicodeString &source,
                            uint32_t &index,
                            UParseError &parseError,
                            UErrorCode &errorCode) {
    CHECK_ERROR(errorCode);

    U_ASSERT(inBounds(source, index));
    if (isFunctionStart(source[index])) {
        // Consume the function name
        parseFunction(source, index, parseError, errorCode);

        // Consume the options (which may be empty)
        parseOptions(source, index, parseError, errorCode);
        return;
    }
    // Must be reserved
    parseReserved(source, index, parseError, errorCode);
}

/*
  Consume a literal followed by whitespace followed by an annotation
*/
static void parseLiteralWithAnnotation(const UnicodeString &source,
                                       uint32_t &index,
                                       UParseError &parseError,
                                       UErrorCode &errorCode) {
    CHECK_ERROR(errorCode);

    U_ASSERT(inBounds(source, index));
    parseLiteral(source, index, parseError, errorCode);

    parseOptionalWhitespace(source, index, parseError, errorCode);
    // Restore precondition
    CHECK_BOUNDS(source, index, parseError, errorCode);

    if (isAnnotationStart(source[index])) {
        parseAnnotation(source, index, parseError, errorCode);
    }
}

/*
  Consume a variable followed by whitespace followed by an annotation
*/
static void parseVariableWithAnnotation(const UnicodeString &source,
                                        uint32_t &index,
                                        UParseError &parseError,
                                        UErrorCode &errorCode) {
    CHECK_ERROR(errorCode);

    U_ASSERT(inBounds(source, index));
    parseVariableName(source, index, parseError, errorCode);

    parseOptionalWhitespace(source, index, parseError, errorCode);
    // Restore precondition
    CHECK_BOUNDS(source, index, parseError, errorCode);

    if (isAnnotationStart(source[index])) {
        parseAnnotation(source, index, parseError, errorCode);
    }
}

/*
  Consume an expression, matching the `expression` nonterminal in the grammar
*/
static void parseExpression(const UnicodeString &source,
                            uint32_t &index,
                            UParseError &parseError,
                            UErrorCode &errorCode) {
    CHECK_ERROR(errorCode);

    U_ASSERT(inBounds(source, index));
    // Parse opening brace
    parseToken(LEFT_CURLY_BRACE, source, index, parseError, errorCode);
    // Optional whitespace after opening brace
    parseOptionalWhitespace(source, index, parseError, errorCode);
    // Restore precondition
    CHECK_BOUNDS(source, index, parseError, errorCode);
    // literal '|', variable '$' or annotation
    switch (source[index]) {
    case PIPE: {
        // Literal
        parseLiteralWithAnnotation(source, index, parseError, errorCode);
        break;
    }
    case DOLLAR: {
        // Variable
        parseVariableWithAnnotation(source, index, parseError, errorCode);
        break;
    }
    default: {
        if (isAnnotationStart(source[index])) {
            parseAnnotation(source, index, parseError, errorCode);
            break;
        }
        // Not a literal, variable or annotation -- error out
        ERROR(parseError, errorCode, index);
        return;
    }
    }
    // Optional whitespace before closing brace
    parseOptionalWhitespace(source, index, parseError, errorCode);
    // Restore precondition
    CHECK_BOUNDS(source, index, parseError, errorCode);
    // Parse closing brace
    parseToken(RIGHT_CURLY_BRACE, source, index, parseError, errorCode);
}

/*
  Consume a possibly-empty sequence of declarations separated by whitespace;
  each declaration matches the `declaration` nonterminal in the grammar
*/
static void parseDeclarations(const UnicodeString &source,
                              uint32_t &index,
                              UParseError &parseError,
                              UErrorCode &errorCode) {
    CHECK_ERROR(errorCode);

    // End-of-input here would be an error; even empty
    // declarations must be followed by a body
    CHECK_BOUNDS(source, index, parseError, errorCode);

    while (source[index] == ID_LET[0]) {
        parseToken(ID_LET, source, index, parseError, errorCode);
        parseRequiredWhitespace(source, index, parseError, errorCode);
        // Restore precondition
        CHECK_BOUNDS(source, index, parseError, errorCode);
        parseVariableName(source, index, parseError, errorCode);
        parseTokenWithWhitespace(EQUALS, source, index, parseError, errorCode);
        parseExpression(source, index, parseError, errorCode);
        parseOptionalWhitespace(source, index, parseError, errorCode);
        // Restore precondition
        CHECK_BOUNDS(source, index, parseError, errorCode);
    }
}

/*
  Consume an escaped curly brace, or backslash, matching the `text-escape`
  nonterminal in the grammar
*/
static void parseTextEscape(const UnicodeString &source,
                            uint32_t &index,
                            UParseError &parseError,
                            UErrorCode &errorCode) {
    CHECK_ERROR(errorCode);
    U_ASSERT(inBounds(source, index));
    U_ASSERT(source[index] == BACKSLASH);
    index++; // Skip the initial backslash
    CHECK_BOUNDS(source, index, parseError, errorCode);
    switch (source[index]) {
    case BACKSLASH:
    case LEFT_CURLY_BRACE:
    case RIGHT_CURLY_BRACE: {
        index++; // Consume the character
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
  Consume a non-empty sequence of text characters and escaped text characters,
  matching the `text` nonterminal in the grammar
*/
static void parseText(const UnicodeString &source,
                      uint32_t &index,
                      UParseError &parseError,
                      UErrorCode &errorCode) {
    CHECK_ERROR(errorCode)
    U_ASSERT(inBounds(source, index));
    bool empty = true;

    while (true) {
        if (source[index] == BACKSLASH) {
            parseTextEscape(source, index, parseError, errorCode);
        } else if (isTextChar(source[index])) {
            index++;
            // Restore precondition
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
  Consume an `nmtoken`, `literal`, or the string "*", matching
  the `key` nonterminal in the grammar
*/
static void parseKey(const UnicodeString &source,
                     uint32_t &index,
                     UParseError &parseError,
                     UErrorCode &errorCode) {
    CHECK_ERROR(errorCode);
    U_ASSERT(inBounds(source, index));

    // Literal | nmtoken | '*'
    switch (source[index]) {
    case PIPE: {
        parseLiteral(source, index, parseError, errorCode);
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
        parseNmtoken(source, index, parseError, errorCode);
        break;
    }
    }
}

/*
  Consume a non-empty sequence of `key`s separated by whitespace
*/
static void parseNonEmptyKeys(const UnicodeString &source,
                              uint32_t &index,
                              UParseError &parseError,
                              UErrorCode &errorCode) {
    CHECK_ERROR(errorCode);
    U_ASSERT(inBounds(source, index));

    // The key sequence must begin with whitespace
    parseRequiredWhitespace(source, index, parseError, errorCode);
    // Restore precondition
    CHECK_BOUNDS(source, index, parseError, errorCode);
    parseKey(source, index, parseError, errorCode);

    while (isWhitespace(source[index])) {
        // Treat whitespace as optional here since the sequence doesn't
        // need to end with whitespace
        parseOptionalWhitespace(source, index, parseError, errorCode);
        // Restore precondition
        CHECK_BOUNDS(source, index, parseError, errorCode);

        if (source[index] == LEFT_CURLY_BRACE) {
            // then we're done parsing the keys and a pattern follows -- break
            return;
        }
        parseKey(source, index, parseError, errorCode);
    }
}

/*
  Consume a `pattern`, matching the nonterminal in the grammar
  No postcondition (on return, `index` might equal `source.length()` with U_SUCCESS(errorCode)),
  because a message can end with a pattern
*/
static void parsePattern(const UnicodeString &source,
                         uint32_t &index,
                         UParseError &parseError,
                         UErrorCode &errorCode) {
    CHECK_ERROR(errorCode);
    U_ASSERT(inBounds(source, index));

    parseToken(LEFT_CURLY_BRACE, source, index, parseError, errorCode);
    while (source[index] != RIGHT_CURLY_BRACE) {
        switch (source[index]) {
        case LEFT_CURLY_BRACE: {
            // Must be expression
            parseExpression(source, index, parseError, errorCode);
            break;
        }
        default: {
            // Must be text
            parseText(source, index, parseError, errorCode);
            break;
        }
        }
        // Need an explicit error check here so we don't loop infinitely
        CHECK_ERROR(errorCode);
    }
    // Consume the closing brace
    index++;
}

/*
  Consume a `selectors` (matching the nonterminal in the grammar),
  followed by a non-empty sequence of `variant`s (matching the nonterminal
  in the grammar) preceded by whitespace
  No postcondition (on return, `index` might equal `source.length()` with U_SUCCESS(errorCode)),
  because a message can end with a variant
*/
static void parseSelectors(const UnicodeString &source,
                           uint32_t &index,
                           UParseError &parseError,
                           UErrorCode &errorCode) {
    CHECK_ERROR(errorCode);
    U_ASSERT(inBounds(source, index));

    parseToken(ID_MATCH, source, index, parseError, errorCode);
    parseOptionalWhitespace(source, index, parseError, errorCode);
    // Restore precondition
    CHECK_BOUNDS(source, index, parseError, errorCode);

    bool selectorsEmpty = true;
    // Parse selectors
    while (source[index] == LEFT_CURLY_BRACE) {
        selectorsEmpty = false;
        parseOptionalWhitespace(source, index, parseError, errorCode);
        // Restore precondition
        CHECK_BOUNDS(source, index, parseError, errorCode);

        parseExpression(source, index, parseError, errorCode);
        parseOptionalWhitespace(source, index, parseError, errorCode);
        // Restore precondition
        CHECK_BOUNDS(source, index, parseError, errorCode);
    }

    // At least one selector is required
    if (selectorsEmpty) {
        ERROR(parseError, errorCode, index);
        return;
    }

    // Parse variants
    parseOptionalWhitespace(source, index, parseError, errorCode);
    // Restore precondition
    CHECK_BOUNDS(source, index, parseError, errorCode);

    bool empty = true;
    while (source[index] == ID_WHEN[0]) {
        empty = false;
        // Consume the "when"
        parseToken(ID_WHEN, source, index, parseError, errorCode);

        // At least one key is required
        parseNonEmptyKeys(source, index, parseError, errorCode);
        parseOptionalWhitespace(source, index, parseError, errorCode);
        // Restore precondition
        CHECK_BOUNDS(source, index, parseError, errorCode);

        parsePattern(source, index, parseError, errorCode);
        parseOptionalWhitespace(source, index, parseError, errorCode);

        // Special case: we've consumed all the input
        if (((int32_t)index) >= source.length()) {
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
  Consume a `body` (matching the nonterminal in the grammar),
  No postcondition (on return, `index` might equal `source.length()` with U_SUCCESS(errorCode)),
  because a message can end with a body (trailing whitespace is optional)
*/
static void parseBody(const UnicodeString &source, uint32_t &index, UParseError &parseError,
                      UErrorCode &errorCode) {
    CHECK_ERROR(errorCode);
    U_ASSERT(inBounds(source, index));

    // Body must be either a pattern or selectors
    switch (source[index]) {
    case LEFT_CURLY_BRACE: {
        // Pattern
        parsePattern(source, index, parseError, errorCode);
        break;
    }
    default: {
        // Selectors
        if (source[index] == ID_MATCH[0]) {
            parseSelectors(source, index, parseError, errorCode);
        } else {
            ERROR(parseError, errorCode, index);
        }
        break;
    }
    }
}

// -------------------------------------
// The copy constructor currently does nothing, since a `MessageFormat`
// object has no state.

MessageFormat2::MessageFormat2(const MessageFormat2 &) {}

// -------------------------------------
// Creates a copy of this MessageFormat2; the caller owns the copy.

MessageFormat2 *MessageFormat2::clone() const { return new MessageFormat2(*this); }

// Not yet implemented
bool MessageFormat2::operator==(const Format &other) const { return (this == &other); }
// Not yet implemented
bool MessageFormat2::operator!=(const Format &other) const { return (this != &other); }

// Not yet implemented
UnicodeString &MessageFormat2::format(const Formattable &, UnicodeString &appendTo, FieldPosition &,
                                      UErrorCode &status) const {
    status = U_UNSUPPORTED_ERROR;
    return appendTo;
}

// Not yet implemented
void MessageFormat2::parseObject(const UnicodeString &, Formattable &, ParsePosition &status) const {
    status = U_UNSUPPORTED_ERROR;
}

// -------------------------------------
// Parses (currently: validates) the source pattern.
// Building a data model is not yet implemented.
void MessageFormat2::parse(const UnicodeString &source,
                           UParseError &parseError,
                           UErrorCode &errorCode) const {
    // Return immediately in the case of a previous error
    CHECK_ERROR(errorCode);

    uint32_t index = 0;
    parseError.line = 0;
    // parseOptionalWhitespace() succeeds on an empty string, so don't check bounds yet
    parseOptionalWhitespace(source, index, parseError, errorCode);
    // parseDeclarations() requires there to be input left, so check to see if
    // parseOptionalWhitespace() consumed it all
    CHECK_BOUNDS(source, index, parseError, errorCode);
    parseDeclarations(source, index, parseError, errorCode);
    parseBody(source, index, parseError, errorCode);
    parseOptionalWhitespace(source, index, parseError, errorCode);

    // Check for errors, so as to avoid overwriting a previous error offset
    CHECK_ERROR(errorCode);

    // There are no errors; finally, check that the entire input was consumed
    if (((int32_t)index) != source.length()) {
        ERROR(parseError, errorCode, index);
    }
}

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */
