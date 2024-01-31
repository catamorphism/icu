// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "messageformat2_errors.h"
#include "messageformat2_macros.h"
#include "messageformat2_parser.h"
#include "uvector.h" // U_ASSERT

U_NAMESPACE_BEGIN

namespace message2 {

using namespace pluralimpl;

using namespace data_model;

/*
    The `ERROR()` macro sets a syntax error in the context
    and sets the offset in `parseError` to `index`. It does not alter control flow.
*/
#define ERROR(parseError, errorCode, index)                                                             \
    if (!errors.hasSyntaxError()) {                                                                     \
        setParseError(parseError, index);                                                               \
        errors.addSyntaxError(errorCode);                                                               \
    }

// Returns true iff `index` is a valid index for the string `source`
static bool inBounds(const UnicodeString &source, uint32_t index) {
    return (((int32_t)index) < source.length());
}

// Increments the line number and updates the "characters seen before
// current line" count in `parseError`, iff `source[index]` is a newline
void Parser::maybeAdvanceLine() {
    if (source[index] == LF) {
        parseError.line++;
        // add 1 to index to get the number of characters seen so far
        // (including the newline)
        parseError.lengthBeforeCurrentLine = index + 1;
    }
}

/*
    Signals an error and returns either if `parseError` already denotes an
    error, or `index` is out of bounds for the string `source`
*/
#define CHECK_BOUNDS(source, index, parseError, errorCode)                                              \
    if (!inBounds(source, index)) {                                                                     \
        ERROR(parseError, errorCode, index);                                                            \
        return;                                                                                         \
    }

// -------------------------------------
// Helper functions

static void copyContext(const UChar in[U_PARSE_CONTEXT_LEN], UChar out[U_PARSE_CONTEXT_LEN]) {
    for (int32_t i = 0; i < U_PARSE_CONTEXT_LEN; i++) {
        out[i] = in[i];
        if (in[i] == '\0') {
            break;
        }
    }
}

/* static */ void Parser::translateParseError(const MessageParseError &messageParseError, UParseError &parseError) {
    parseError.line = messageParseError.line;
    parseError.offset = messageParseError.offset;
    copyContext(messageParseError.preContext, parseError.preContext);
    copyContext(messageParseError.postContext, parseError.postContext);
}

/* static */ void Parser::setParseError(MessageParseError &parseError, uint32_t index) {
    // Translate absolute to relative offset
    parseError.offset = index                               // Start with total number of characters seen
                      - parseError.lengthBeforeCurrentLine; // Subtract all characters before the current line
    // TODO: Fill this in with actual pre and post-context
    parseError.preContext[0] = 0;
    parseError.postContext[0] = 0;
}

// -------------------------------------
// Predicates

// Returns true if `c` is in the interval [`first`, `last`]
static bool inRange(UChar32 c, UChar32 first, UChar32 last) {
    U_ASSERT(first < last);
    return c >= first && c <= last;
}

/*
  The following helper predicates should exactly match nonterminals in the MessageFormat 2 grammar:

  `isTextChar()`      : `text-char`
  `isReservedStart()` : `reserved-start`
  `isReservedChar()`  : `reserved-char`
  `isAlpha()`         : `ALPHA`
  `isDigit()`         : `DIGIT`
  `isNameStart()`     : `name-start`
  `isNameChar()`      : `name-char`
  `isUnquotedStart()` : `unquoted-start`
  `isQuotedChar()`    : `quoted-char`
  `isWhitespace()`    : `s`
*/
static bool isTextChar(UChar32 c) {
    return inRange(c, 0x0000, 0x005B)    // Omit backslash
           || inRange(c, 0x005D, 0x007A) // Omit {
           || c == 0x007C                // }
           || inRange(c, 0x007E, 0xD7FF) // Omit surrogates
           || inRange(c, 0xE000, 0x10FFFF);
}

static bool isReservedStart(UChar32 c) {
    switch (c) {
    case BANG:
    case AT:
    case NUMBER_SIGN:
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

static bool isReservedChar(UChar32 c) {
    return inRange(c, 0x0000, 0x0008)    // Omit HTAB and LF
           || inRange(c, 0x000B, 0x000C) // Omit CR
           || inRange(c, 0x000E, 0x0019) // Omit SP
           || inRange(c, 0x0021, 0x005B) // Omit backslash
           || inRange(c, 0x005D, 0x007A) // Omit { | }
           || inRange(c, 0x007E, 0xD7FF) // Omit surrogates
           || inRange(c, 0xE000, 0x10FFFF);
}

static bool isAlpha(UChar32 c) { return inRange(c, 0x0041, 0x005A) || inRange(c, 0x0061, 0x007A); }

static bool isDigit(UChar32 c) { return inRange(c, 0x0030, 0x0039); }

static bool isNameStart(UChar32 c) {
    return isAlpha(c) || c == UNDERSCORE || inRange(c, 0x00C0, 0x00D6) || inRange(c, 0x00D8, 0x00F6) ||
           inRange(c, 0x00F8, 0x02FF) || inRange(c, 0x0370, 0x037D) || inRange(c, 0x037F, 0x1FFF) ||
           inRange(c, 0x200C, 0x200D) || inRange(c, 0x2070, 0x218F) || inRange(c, 0x2C00, 0x2FEF) ||
           inRange(c, 0x3001, 0xD7FF) || inRange(c, 0xF900, 0xFDCF) || inRange(c, 0xFDF0, 0xFFFD) ||
           inRange(c, 0x10000, 0xEFFFF);
}

static bool isNameChar(UChar32 c) {
    return isNameStart(c) || isDigit(c) || c == HYPHEN || c == PERIOD || c == COLON || c == 0x00B7 ||
           inRange(c, 0x0300, 0x036F) || inRange(c, 0x203F, 0x2040);
}

static bool isUnquotedStart(UChar32 c) {
    return isNameStart(c) || isDigit(c) || c == PERIOD || c == 0x00B7 ||
           inRange(c, 0x0300, 0x036F) || inRange(c, 0x203F, 0x2040);
}

static bool isQuotedChar(UChar32 c) {
    return inRange(c, 0x0000, 0x005B)    // Omit backslash
           || inRange(c, 0x005D, 0x007B) // Omit pipe
           || inRange(c, 0x007D, 0xD7FF) // Omit surrogates
           || inRange(c, 0xE000, 0x10FFFF);
}

// Returns true iff `c` can begin a `function` nonterminal
static bool isFunctionStart(UChar32 c) {
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
static bool isAnnotationStart(UChar32 c) {
    return isFunctionStart(c) || isReservedStart(c);
}

// Returns true iff `c` can begin either a `reserved-char` or `reserved-escape`
// literal
static bool reservedChunkFollows(UChar32 c) {
   switch(c) {
       // reserved-escape
       case BACKSLASH:
       // literal
       case PIPE: {
           return true;
       }
       default: {
           // reserved-char
           return (isReservedChar(c));
       }
    }
}

// See `s` in the MessageFormat 2 grammar
inline bool isWhitespace(UChar32 c) {
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

// -------------------------------------
// Parsing functions

/*
    This is a recursive-descent scannerless parser that,
    with a few exceptions, uses 1 character of lookahead.

All the exceptions involve ambiguities about the meaning of whitespace.

There are four ambiguities in the grammar that can't be resolved with finite
lookahead (since whitespace sequences can be arbitrarily long). They are resolved
with a form of backtracking (early exit). No state needs to be saved/restored
since whitespace doesn't affect the shape of the resulting parse tree, so it's
not true backtracking.

In addition, the grammar has been refactored
in a semantics-preserving way in some cases to make the code easier to structure.

First: variant = when 1*(s key) [s] pattern
   Example: when k     {a}
   When reading the first space after 'k', it's ambiguous whether it's the
   required space before another key, or the optional space before `pattern`.
 (See comments in parseNonEmptyKeys())

Second: expression = "{" [s] (((literal / variable) [s annotation]) / annotation) [s] "}"
        annotation = (function *(s option)) / reserved
   Example: {:f    }
   When reading the first space after 'f', it's ambiguous whether it's the
   required space before an option, or the optional trailing space after an options list
   (in this case, the options list is empty).
 (See comments in parseOptions() -- handling this case also meant it was easier to base
  the code on a slightly refactored grammar, which should be semantically equivalent.)

Third: expression = "{" [s] (((literal / variable) [s annotation]) / annotation) [s] "}"
        annotation = (function *(s option)) / reserved
   Example: {@a }
   Similar to the previous case; see comments in parseReserved()

Fourth: expression = "{" [s] (((literal / variable) [s annotation]) / annotation) [s] "}"
   Example: {|foo|   }
   When reading the first space after the '|', it's ambiguous whether it's the required
   space before an annotation, or the optional trailing space before the '}'.
  (See comments in parseLiteralOrVariableWithAnnotation(); handling this case relies on
  the same grammar refactoring as the second exception.)

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
void Parser::parseWhitespaceMaybeRequired(bool required, UErrorCode& errorCode) {
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
            maybeAdvanceLine();
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
void Parser::parseRequiredWhitespace(UErrorCode& errorCode) {
    parseWhitespaceMaybeRequired(true, errorCode);
    normalizedInput += SPACE;
}

/*
  No pre, no post, for the same reason as `parseWhitespaceMaybeRequired()`.
*/
void Parser::parseOptionalWhitespace(UErrorCode& errorCode) {
    parseWhitespaceMaybeRequired(false, errorCode);
}

// Consumes a single character, signaling an error if `source[index]` != `c`
void Parser::parseToken(UChar32 c, UErrorCode& errorCode) {
    CHECK_BOUNDS(source, index, parseError, errorCode);

    if (source[index] == c) {
        index++;
        // Guarantee postcondition
        CHECK_BOUNDS(source, index, parseError, errorCode);
        normalizedInput += c;
        return;
    }
    // Next character didn't match -- error out
    ERROR(parseError, errorCode, index);
}

/*
   Consumes a fixed-length token, signaling an error if the token isn't a prefix of
   the string beginning at `source[index]`
*/
template <int32_t N>
void Parser::parseToken(const UChar32 (&token)[N], UErrorCode& errorCode) {
    U_ASSERT(inBounds(source, index));

    int32_t tokenPos = 0;
    while (tokenPos < N - 1) {
        if (source[index] != token[tokenPos]) {
            ERROR(parseError, errorCode, index);
            return;
        }
        normalizedInput += token[tokenPos];
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
template <int32_t N>
void Parser::parseTokenWithWhitespace(const UChar32 (&token)[N], UErrorCode& errorCode) {
    // No need for error check or bounds check before parseOptionalWhitespace
    parseOptionalWhitespace(errorCode);
    // Establish precondition
    CHECK_BOUNDS(source, index, parseError, errorCode);
    parseToken(token);
    parseOptionalWhitespace(errorCode);
    // Guarantee postcondition
    CHECK_BOUNDS(source, index, parseError, errorCode);
}

/*
   Consumes optional whitespace, possibly advancing `index` to `index'`,
   then consumes a single character (signaling an error if it doesn't match
   `source[index']`),
   then consumes optional whitespace again
*/
void Parser::parseTokenWithWhitespace(UChar32 c, UErrorCode& errorCode) {
    // No need for error check or bounds check before parseOptionalWhitespace(errorCode)
    parseOptionalWhitespace(errorCode);
    // Establish precondition
    CHECK_BOUNDS(source, index, parseError, errorCode);
    parseToken(c, errorCode);
    parseOptionalWhitespace(errorCode);
    // Guarantee postcondition
    CHECK_BOUNDS(source, index, parseError, errorCode);
}

/*
  Consumes a non-empty sequence of `name-char`s, the first of which is
  also a `name-start`.
  that begins with a character `start` such that `isNameStart(start)`.

  Returns this sequence.

  (Matches the `name` nonterminal in the grammar.)
*/
UnicodeString Parser::parseName(UErrorCode& errorCode) {
    UnicodeString name;

    U_ASSERT(inBounds(source, index));

    if (!isNameStart(source[index])) {
        ERROR(parseError, errorCode, index);
        return name;
    }

    while (isNameChar(source[index])) {
        name += source[index];
        normalizedInput += source[index];
        index++;
        if (!inBounds(source, index)) {
            ERROR(parseError, errorCode, index);
            break;
        }
    }
    return name;
}

/*
  Consumes a '$' followed by a `name`, returning a VariableName
  with `name` as its name

  (Matches the `variable` nonterminal in the grammar.)
*/
VariableName Parser::parseVariableName(UErrorCode& errorCode) {
    VariableName result;

    U_ASSERT(inBounds(source, index));
    // If the '$' is missing, we don't want a binding
    // for this variable to be created.
    bool valid = source[index] == DOLLAR;
    parseToken(DOLLAR, errorCode);
    if (!inBounds(source, index)) {
        ERROR(parseError, errorCode, index);
        return result;
    }
    UnicodeString varName = parseName(errorCode);
    // Set the name to "" if the variable wasn't
    // declared correctly
    if (!valid) {
        varName.remove();
    }
    return VariableName(varName);
}

static FunctionName::Sigil functionSigil(UChar32 c) {
    switch (c) {
        case PLUS:   { return FunctionName::Sigil::OPEN; }
        case HYPHEN: { return FunctionName::Sigil::CLOSE; }
        default: {
            U_ASSERT(c == COLON);
            return FunctionName::Sigil::DEFAULT;
        }
    }
}
/*
  Consumes a reference to a function, matching the `function` nonterminal in
  the grammar.

  Initializes `func` to this name.
*/
FunctionName Parser::parseFunction(UErrorCode& errorCode) {
    U_ASSERT(inBounds(source, index));
    if (!isFunctionStart(source[index])) {
        ERROR(parseError, errorCode, index);
        return FunctionName();
    }

    FunctionName::Sigil sigil = functionSigil(source[index]);
    normalizedInput += source[index];
    index++; // Consume the function start character
    if (!inBounds(source, index)) {
        ERROR(parseError, errorCode, index);
        return FunctionName();
    }
    return FunctionName(parseName(errorCode), sigil);
}


/*
  Precondition: source[index] == BACKSLASH

  Consume an escaped character.

  Generalized to handle `reserved-escape`, `text-escape`,
  or `literal-escape`, depending on the `kind` argument.

  Appends result to `str`
*/
void Parser::parseEscapeSequence(EscapeKind kind,
                                 UnicodeString &str,
                                 UErrorCode& errorCode) {
    U_ASSERT(inBounds(source, index));
    U_ASSERT(source[index] == BACKSLASH);
    normalizedInput += BACKSLASH;
    index++; // Skip the initial backslash
    CHECK_BOUNDS(source, index, parseError, errorCode);

    #define SUCCEED \
       /* Append to the output string */                    \
       str += source[index];                                \
       /* Update normalizedInput */                         \
       normalizedInput += source[index];                    \
       /* Consume the character */                          \
       index++;                                             \
       /* Guarantee postcondition */                        \
       CHECK_BOUNDS(source, index, parseError, errorCode);  \
       return;

    // Expect a '{', '|' or '}'
    switch (source[index]) {
    case LEFT_CURLY_BRACE:
    case RIGHT_CURLY_BRACE: {
        // Allowed in a `text-escape` or `reserved-escape`
        switch (kind) {
        case TEXT:
        case RESERVED: {
            SUCCEED;
        }
        default: {
            break;
        }
        }
        break;
    }
    case PIPE: {
        // Allowed in a `literal-escape` or `reserved-escape`
        switch (kind) {
           case LITERAL:
           case RESERVED: {
               SUCCEED;
           }
           default: {
               break;
           }
        }
        break;
    }
   case BACKSLASH: {
       // Allowed in any escape sequence
       SUCCEED;
   }
   default: {
        // No other characters are allowed here
        break;
    }
   }
   // If control reaches here, there was an error
   ERROR(parseError, errorCode, index);
}

/*
  Consume an escaped pipe or backslash, matching the `literal-escape`
  nonterminal in the grammar
*/
void Parser::parseLiteralEscape(UnicodeString &str, UErrorCode& errorCode) {
    parseEscapeSequence(LITERAL, str, errorCode);
}

/*
  Consume and return a literal, matching the `literal` nonterminal in the grammar.
  May be quoted or unquoted -- returns true iff quoted
*/
Literal Parser::parseLiteral(UErrorCode& errorCode) {
    Literal result;
    if (!inBounds(source, index)) {
        ERROR(parseError, errorCode, index);
        return result;
    }

    bool quoted;
    // Parse the opening '|' if present
    if (source[index] == PIPE) {
        quoted = true;
        parseToken(PIPE, errorCode);
        if (!inBounds(source, index)) {
            ERROR(parseError, errorCode, index);
            return result;
        }
    } else {
        if (!isUnquotedStart(source[index])) {
            ERROR(parseError, errorCode, index);
            return result;
        }
        quoted = false;
    }

    // Parse the contents
    UnicodeString contents;
    bool done = false;
    while (!done) {
        if (quoted && source[index] == BACKSLASH) {
            parseLiteralEscape(contents, errorCode);
        } else if ((!quoted && isNameChar(source[index]))
                   || (quoted && isQuotedChar(source[index]))) {
            contents += source[index];
            normalizedInput += source[index];
            index++; // Consume this character
            maybeAdvanceLine();
        } else {
          // Assume the sequence of literal characters ends here
          done = true;
        }
        if (!inBounds(source, index)) {
            ERROR(parseError, errorCode, index);
            return result;
        }
    }

    // Parse the closing '|' if we saw an opening '|'
    if (quoted) {
        parseToken(PIPE, errorCode);
    }

    // Guarantee postcondition
    if (!inBounds(source, index)) {
        ERROR(parseError, errorCode, index);
        return result;
    }

    return Literal(quoted, contents);
}

/*
  Consume a name-value pair, matching the `option` nonterminal in the grammar.

  Adds the option to `optionList`
*/
void Parser::parseOption(Operator::Builder &builder, UErrorCode& errorCode) {
    U_ASSERT(inBounds(source, index));

    // Parse LHS
    UnicodeString lhs = parseName(errorCode);

    // Parse '='
    parseTokenWithWhitespace(EQUALS, errorCode);

    UnicodeString rhsStr;
    Operand rand;
    // Parse RHS, which is either a literal or variable
    switch (source[index]) {
    case DOLLAR: {
        rand = Operand(parseVariableName(errorCode));
        break;
    }
    default: {
        // Must be a literal
        rand = Operand(parseLiteral(errorCode));
        break;
    }
    }
    U_ASSERT(!rand.isNull());

    // Finally, add the key=value mapping
    // Use a local error code, check for duplicate option error and
    // record it as with other errors
    UErrorCode status = U_ZERO_ERROR;
    builder.addOption(lhs, std::move(rand), status);
    if (U_FAILURE(status)) {
      U_ASSERT(status == U_DUPLICATE_OPTION_NAME_ERROR);
      errors.setDuplicateOptionName(errorCode);
    }
}

/*
  Consume optional whitespace followed by a sequence of options
  (possibly empty), separated by whitespace
*/
void Parser::parseOptions(Operator::Builder& builder, UErrorCode& errorCode) {
    // Early exit if out of bounds -- no more work is possible
    CHECK_BOUNDS(source, index, parseError, errorCode);

/*
Arbitrary lookahead is required to parse option lists. To see why, consider
these rules from the grammar:

expression = "{" [s] (((literal / variable) [s annotation]) / annotation) [s] "}"
annotation = (function *(s option)) / reserved

And this example:
{:foo  }

Derivation:
expression -> "{" [s] (((literal / variable) [s annotation]) / annotation) [s] "}"
           -> "{" [s] annotation [s] "}"
           -> "{" [s] ((function *(s option)) / reserved) [s] "}"
           -> "{" [s] function *(s option) [s] "}"

In this example, knowing whether to expect a '}' or the start of another option
after the whitespace would require arbitrary lookahead -- in other words, which
rule should we apply?
    *(s option) -> s option *(s option)
  or
    *(s option) ->

The same would apply to the example {:foo k=v } (note the trailing space after "v").

This is addressed using a form of backtracking and (to make the backtracking easier
to apply) a slight refactoring to the grammar.

This code is written as if the grammar is:
  expression = "{" [s] (((literal / variable) ([s] / [s annotation])) / annotation) "}"
  annotation = (function *(s option) [s]) / (reserved [s])

Parsing the `*(s option) [s]` sequence can be done within `parseOptions()`, meaning
that `parseExpression()` can safely require a '}' after `parseOptions()` finishes.

Note that when "backtracking" really just means early exit, since only whitespace
is involved and there's no state to save.
*/

    while(true) {
        // If the next character is not whitespace, that means we've already
        // parsed the entire options list (which may have been empty) and there's
        // no trailing whitespace. In that case, exit.
        if (!isWhitespace(source[index])) {
            break;
        }

        // In any case other than an empty options list, there must be at least
        // one whitespace character.
        parseRequiredWhitespace(errorCode);
        // Restore precondition
        CHECK_BOUNDS(source, index, parseError, errorCode);

        // If a name character follows, then at least one more option remains
        // in the list.
        // Otherwise, we've consumed all the options and any trailing whitespace,
        // and can exit.
        // Note that exiting is sort of like backtracking: "(s option)" doesn't apply,
        // so we back out to [s].
        if (!isNameStart(source[index])) {
            // We've consumed all the options (meaning that either we consumed non-empty
            // whitespace, or consumed at least one option.)
            // Done.
            // Remove the whitespace from normalizedInput
            U_ASSERT(normalizedInput.truncate(normalizedInput.length() - 1));
            break;
        }
        parseOption(builder, errorCode);
    }
}

void Parser::parseReservedEscape(UnicodeString &str, UErrorCode& errorCode) {
    parseEscapeSequence(RESERVED, str, errorCode);
}

/*
  Consumes a non-empty sequence of reserved-chars, reserved-escapes, and
  literals (as in 1*(reserved-char / reserved-escape / literal) in the `reserved-body` rule)

  Appends it to `str`
*/
void Parser::parseReservedChunk(Reserved::Builder& result, UErrorCode& status) {
    CHECK_ERROR(status);

    bool empty = true;
    UnicodeString chunk;
    while(reservedChunkFollows(source[index])) {
        empty = false;
        // reserved-char
        if (isReservedChar(source[index])) {
            chunk += source[index];
            normalizedInput += source[index];
            // consume the char
            index++;
            // Restore precondition
            CHECK_BOUNDS(source, index, parseError, status);
            continue;
        }

        if (chunk.length() > 0) {
          result.add(Literal(false, chunk), status);
          chunk.setTo(u"", 0);
        }

        if (source[index] == BACKSLASH) {
            // reserved-escape
            parseReservedEscape(chunk, status);
            result.add(Literal(false, chunk), status);
            chunk.setTo(u"", 0);
        } else if (source[index] == PIPE || isUnquotedStart(source[index])) {
            result.add(parseLiteral(status), status);
        } else {
            // The reserved chunk ends here
            break;
        }

        CHECK_ERROR(status); // Avoid looping infinitely
    }

    // Add the last chunk if necessary
    if (chunk.length() > 0) {
        result.add(Literal(false, chunk), status);
    }

    if (empty) {
        ERROR(parseError, status, index);
    }
}

/*
  Consume a `reserved-start` character followed by a possibly-empty sequence
  of non-empty sequences of reserved characters, separated by whitespace.
  Matches the `reserved` nonterminal in the grammar

*/
Reserved Parser::parseReserved(UErrorCode& status) {
    Reserved::Builder builder(status);

    if (U_FAILURE(status)) {
        return {};
    }

    U_ASSERT(inBounds(source, index));

    // Require a `reservedStart` character
    if (!isReservedStart(source[index])) {
        ERROR(parseError, status, index);
        return Reserved();
    }

    // Add the start char as a separate text chunk
    UnicodeString firstCharString(source[index]);
    builder.add(Literal(false, firstCharString), status);
    if (U_FAILURE(status)) {
        return {};
    }
    // Consume reservedStart
    normalizedInput += source[index];
    index++;

/*
  Arbitrary lookahead is required to parse a `reserved`, for similar reasons
  to why it's required for parsing function annotations.

  In the grammar:

  annotation = (function *(s option)) / reserved
  expression = "{" [s] (((literal / variable) [s annotation]) / annotation) [s] "}"
  reserved       = reserved-start reserved-body
  reserved-body  = *( [s] 1*(reserved-char / reserved-escape / literal))

  When reading a whitespace character, it's ambiguous whether it's the optional
  whitespace in this rule, or the optional whitespace that precedes a '}' in an
  expression.

  The ambiguity is resolved using the same grammar refactoring as shown in
  the comment in `parseOptions()`.
*/
    // Consume reserved characters / literals / reserved escapes
    // until a character that can't be in a `reserved-body` is seen
    while (true) {
        /*
          First, if there is whitespace, it means either a chunk follows it,
          or this is the trailing whitespace before the '}' that terminates an
          expression.

          Next, if the next character can start a reserved-char, reserved-escape,
          or literal, then parse a "chunk" of reserved things.
          In any other case, we exit successfully, since per the refactored
          grammar rule:
               annotation = (function *(s option) [s]) / (reserved [s])
          it's valid to consume whitespace after a `reserved`.
          (`parseExpression()` is responsible for checking that the next
          character is in fact a '}'.)
         */
        if (!inBounds(source, index)) {
            break;
        }
        bool sawWhitespace = false;
        if (isWhitespace(source[index])) {
            sawWhitespace = true;
            parseOptionalWhitespace(status);
            // Restore precondition
            if (!inBounds(source, index)) {
                break;
            }
        }

        if (reservedChunkFollows(source[index])) {
            parseReservedChunk(builder, status);

            // Avoid looping infinitely
            if (U_FAILURE(status) || !inBounds(source, index)) {
                break;
            }
        } else {
            if (sawWhitespace) {
                if (source[index] == RIGHT_CURLY_BRACE) {
                    // Not an error: just means there's no trailing whitespace
                    // after this `reserved`
                    break;
                }
                // Error: if there's whitespace, it must either be followed
                // by a non-empty sequence or by '}'
                ERROR(parseError, status, index);
                break;
            }
            // If there was no whitespace, it's not an error,
            // just the end of the reserved string
            break;
        }
    }

    return builder.build(status);
}


/*
  Consume a function call or reserved string, matching the `annotation`
  nonterminal in the grammar

  Returns an `Operator` representing this (a reserved is a parse error)
*/
Operator Parser::parseAnnotation(UErrorCode& status) {
    U_ASSERT(inBounds(source, index));
    Operator::Builder ratorBuilder(status);
    if (U_FAILURE(status)) {
        return {};
    }
    if (isFunctionStart(source[index])) {
        // Consume the function name
        FunctionName func = parseFunction(status);
        ratorBuilder.setFunctionName(std::move(func));
        // Consume the options (which may be empty)
        parseOptions(ratorBuilder, status);
    } else {
      // Must be reserved
      // A reserved sequence is not a parse error, but might be a formatting error
      Reserved rator = parseReserved(status);
      ratorBuilder.setReserved(std::move(rator));
    }
    UErrorCode localStatus = U_ZERO_ERROR;
    Operator result = ratorBuilder.build(localStatus);
    // Either `setReserved` or `setFunctionName` was called,
    // so there shouldn't be an error.
    U_ASSERT(U_SUCCESS(localStatus));
    return result;
}

/*
  Consume a literal or variable (depending on `isVariable`),
  followed by either required whitespace followed by an annotation,
  or optional whitespace.
*/
void Parser::parseLiteralOrVariableWithAnnotation(bool isVariable,
                                                  Expression::Builder& builder,
                                                  UErrorCode& status) {
    CHECK_ERROR(status);

    U_ASSERT(inBounds(source, index));

    Operand rand;
    if (isVariable) {
        rand = Operand(parseVariableName(status));
    } else {
        rand = Operand(parseLiteral(status));
    }

    builder.setOperand(std::move(rand));

/*
Parsing a literal or variable with an optional annotation requires arbitrary lookahead.
To see why, consider this rule from the grammar:

expression = "{" [s] (((literal / variable) [s annotation]) / annotation) [s] "}"

And this example:

{|foo|   }

Derivation:
expression -> "{" [s] (((literal / variable) [s annotation]) / annotation) [s] "}"
           -> "{" [s] ((literal / variable) [s annotation]) [s] "}"
           -> "{" [s] (literal [s annotation]) [s] "}"

When reading the ' ' after the second '|', it's ambiguous whether that's the required
space before an annotation, or the optional space before the '}'.

To make this ambiguity easier to handle, this code is based on the same grammar
refactoring for the `expression` nonterminal that `parseOptions()` relies on. See
the comment in `parseOptions()` for details.
*/

    if (isWhitespace(source[index])) {
      // If the next character is whitespace, either [s annotation] or [s] applies
      // (the character is either the required space before an annotation, or optional
      // trailing space after the literal or variable). It's still ambiguous which
      // one does apply.
      parseOptionalWhitespace(status);
      // Restore precondition
      CHECK_BOUNDS(source, index, parseError, status);

      // This next check resolves the ambiguity between [s annotation] and [s]
      if (isAnnotationStart(source[index])) {
        normalizedInput += SPACE;
        // The previously consumed whitespace precedes an annotation
        builder.setOperator(parseAnnotation(status));
      }
    } else {
      // Either there was never whitespace, or
      // the previously consumed whitespace is the optional trailing whitespace;
      // either the next character is '}' or the error will be handled by parseExpression.
      // Do nothing, since the operand was already set
    }
}

/*
  Consume an expression, matching the `expression` nonterminal in the grammar
*/

static void exprFallback(Expression::Builder& exprBuilder) {
    // Construct a literal consisting just of  The U+FFFD REPLACEMENT CHARACTER
    // per https://github.com/unicode-org/message-format-wg/blob/main/spec/formatting.md#fallback-resolution
    exprBuilder.setOperand(Operand(Literal(false, UnicodeString(REPLACEMENT))));
}

static Expression exprFallback() {
    Expression::Builder exprBuilder;

    // Construct a literal consisting just of  The U+FFFD REPLACEMENT CHARACTER
    // per https://github.com/unicode-org/message-format-wg/blob/main/spec/formatting.md#fallback-resolution
    exprBuilder.setOperand(Operand(Literal(false, UnicodeString(REPLACEMENT))));
    UErrorCode status = U_ZERO_ERROR;
    Expression result = exprBuilder.build(status);
    // An operand was set, so there can't be an error
    U_ASSERT(U_SUCCESS(status));
    return result;
}

// Sets `parseError` to true if there was an error parsing this expression
// Uses a flag rather than just returning a fallback expression because which
// fallback to use depends on context
Expression Parser::parseExpression(bool& err, UErrorCode& status) {
    if (U_FAILURE(status)) {
        return {};
    }

    err = false;

    // Early return if out of input -- no more work is possible
    U_ASSERT(inBounds(source, index));

    // Parse opening brace
    parseToken(LEFT_CURLY_BRACE, status);
    // Optional whitespace after opening brace
    parseOptionalWhitespace(status);

    Expression::Builder exprBuilder;
    // Restore precondition
    if (!inBounds(source, index)) {
        exprFallback(exprBuilder);
    } else {
        // literal '|', variable '$' or annotation
        switch (source[index]) {
        case PIPE: {
            // Quoted literal
            parseLiteralOrVariableWithAnnotation(false, exprBuilder, status);
            break;
        }
        case DOLLAR: {
            // Variable
            parseLiteralOrVariableWithAnnotation(true, exprBuilder, status);
            break;
        }
        default: {
            if (isAnnotationStart(source[index])) {
                Operator rator = parseAnnotation(status);
                exprBuilder.setOperator(std::move(rator));
            } else if (isUnquotedStart(source[index])) {
                // Unquoted literal
                parseLiteralOrVariableWithAnnotation(false, exprBuilder, status);
            } else {
                // Not a literal, variable or annotation -- error out
                ERROR(parseError, status, index);
                // Set the operand in order to avoid an invalid state error --
                // however, the caller will ignore the result
                exprFallback(exprBuilder);
                err = true;
                break;
            }
            break;
        }
        }
    }
    // For why we don't parse optional whitespace here, even though the grammar
    // allows it, see comments in parseLiteralWithAnnotation() and parseOptions()

    // Parse closing brace
    parseToken(RIGHT_CURLY_BRACE, status);

    // Either an operand or operator (or both) must have been set already,
    // so there can't be an error
    UErrorCode localStatus = U_ZERO_ERROR;
    Expression result = exprBuilder.build(localStatus);
    U_ASSERT(U_SUCCESS(localStatus));
    return result;
}

/*
  Consume a possibly-empty sequence of declarations separated by whitespace;
  each declaration matches the `declaration` nonterminal in the grammar

  Builds up an environment representing those declarations
*/
void Parser::parseDeclarations(UErrorCode& status) {
    // End-of-input here would be an error; even empty
    // declarations must be followed by a body
    CHECK_BOUNDS(source, index, parseError, status);

    while (source[index] == ID_LET[0]) {
        parseToken(ID_LET, status);
        parseRequiredWhitespace(status);
        // Restore precondition
        CHECK_BOUNDS(source, index, parseError, status);
        VariableName lhs = parseVariableName(status);
        parseTokenWithWhitespace(EQUALS, status);

        // Restore precondition before calling parseExpression()
        // (which must return a non-null value)
        CHECK_BOUNDS(source, index, parseError, status);

        bool rhsError = false;
        Expression rhs = parseExpression(rhsError, status);
        // Avoid looping infinitely
        CHECK_ERROR(status);

        if (rhsError) {
            rhs = exprFallback();
        }
        parseOptionalWhitespace(status);
        // Restore precondition
        CHECK_BOUNDS(source, index, parseError, status);

        // Add binding from lhs to rhs, unless there was an error
        if (lhs.identifier().length() > 0) {
            dataModel.addLocalVariable(std::move(lhs), std::move(rhs), status);
        }
    }
}

/*
  Consume an escaped curly brace, or backslash, matching the `text-escape`
  nonterminal in the grammar
*/
void Parser::parseTextEscape(UnicodeString &str, UErrorCode& status) {
    parseEscapeSequence(TEXT, str, status);
}

/*
  Consume a non-empty sequence of text characters and escaped text characters,
  matching the `text` nonterminal in the grammar
*/
UnicodeString Parser::parseText(UErrorCode& status) {
    UnicodeString str;
    if (!inBounds(source, index)) {
        ERROR(parseError, status, index);
        return str;
    }

    bool empty = true;

    while (true) {
        if (source[index] == BACKSLASH) {
            parseTextEscape(str, status);
        } else if (isTextChar(source[index])) {
            normalizedInput += source[index];
            str += source[index];
            index++;
            maybeAdvanceLine();
        } else {
            break;
        }
        // Restore precondition
        if (!inBounds(source, index)) {
            ERROR(parseError, status, index);
            return str;
        }
        empty = false;
    }
    if (empty) {
        // text must be non-empty
        ERROR(parseError, status, index);
    }

    return str;
}

/*
  Consume an `nmtoken`, `literal`, or the string "*", matching
  the `key` nonterminal in the grammar
*/
Key Parser::parseKey(UErrorCode& status) {
    U_ASSERT(inBounds(source, index));

    Key k; // wildcard by default
    // Literal | '*'
    switch (source[index]) {
    case ASTERISK: {
        index++;
        // Guarantee postcondition
        if (!inBounds(source, index)) {
            ERROR(parseError, status, index);
            return k;
        }
        normalizedInput += ASTERISK;
        break;
    }
    default: {
        // Literal
        k = Key(parseLiteral(status));
        break;
    }
    }
    return k;
}

/*
  Consume a non-empty sequence of `key`s separated by whitespace

  Takes ownership of `keys`
*/
SelectorKeys Parser::parseNonEmptyKeys(UErrorCode& status) {
    SelectorKeys result;

    if (U_FAILURE(status)) {
        return result;
    }

    U_ASSERT(inBounds(source, index));

/*
Arbitrary lookahead is required to parse key lists. To see why, consider
this rule from the grammar:

variant = when 1*(s key) [s] pattern

And this example:
when k1    {a}

Derivation:
   variant -> when 1*(s key) [s] pattern
           -> when s key *(s key) [s] pattern

After matching ' ' to `s` and 'k1' to `key`, it would require arbitrary lookahead
to know whether to expect the start of a pattern or the start of another key.
In other words: is the second whitespace sequence the required space in 1*(s key),
or the optional space in [s] pattern?

This is addressed using "backtracking" (similarly to `parseOptions()`).
*/

    SelectorKeys::Builder keysBuilder(status);
    if (U_FAILURE(status)) {
        return result;
    }

    // Since the first key is required, it's simplest to parse the required
    // whitespace and then the first key separately.
    parseRequiredWhitespace(status);
    // Restore precondition
    if (!inBounds(source, index)) {
        ERROR(parseError, status, index);
        return result;
    }
    keysBuilder.add(parseKey(status), status);


    // We've seen at least one whitespace-key pair, so now we can parse
    // *(s key) [s]
    while (source[index] != LEFT_CURLY_BRACE) { // Try to recover from errors
        bool wasWhitespace = isWhitespace(source[index]);
        parseRequiredWhitespace(status);
        if (!wasWhitespace) {
            // Avoid infinite loop when parsing something like:
            // when * @{!...
            index++;
        }

        // Restore precondition
        if (!inBounds(source, index)) {
            ERROR(parseError, status, index);
            return result;
        }

        // At this point, it's ambiguous whether we are inside (s key) or [s].
        // This check resolves that ambiguity.
        if (source[index] == LEFT_CURLY_BRACE) {
            // A pattern follows, so what we just parsed was the optional
            // trailing whitespace. All the keys have been parsed.

            // Unpush the whitespace from `normalizedInput`
            U_ASSERT(normalizedInput.truncate(normalizedInput.length() - 1));
            break;
        }
        keysBuilder.add(parseKey(status), status);
    }

    return keysBuilder.build(status);
}

/*
  Consume a `pattern`, matching the nonterminal in the grammar
  No postcondition (on return, `index` might equal `source.length()` with U_SUCCESS(status)),
  because a message can end with a pattern
*/
Pattern Parser::parsePattern(UErrorCode& status) {
    U_ASSERT(inBounds(source, index));

    Pattern::Builder result(status);

    if (U_SUCCESS(status)) {
        parseToken(LEFT_CURLY_BRACE, status);

        Expression expression;
        while (source[index] != RIGHT_CURLY_BRACE) {
            switch (source[index]) {
            case LEFT_CURLY_BRACE: {
                // Must be expression
                bool rhsError = false;
                result.add(PatternPart(parseExpression(rhsError, status)), status);
                break;
            }
            default: {
                // Must be text
                result.add(PatternPart(parseText(status)), status);
                break;
            }
            }
            // Need an explicit error check here so we don't loop infinitely
            if (!inBounds(source, index)) {
                return result.build(status);
            }
        }
        // Consume the closing brace
        index++;
        normalizedInput += RIGHT_CURLY_BRACE;
    }
    return result.build(status);
}


/*
  Consume a `selectors` (matching the nonterminal in the grammar),
  followed by a non-empty sequence of `variant`s (matching the nonterminal
  in the grammar) preceded by whitespace
  No postcondition (on return, `index` might equal `source.length()` with no syntax error
  because a message can end with a variant)
*/
void Parser::parseSelectors(UErrorCode& status) {
    CHECK_ERROR(status);

    U_ASSERT(inBounds(source, index));

    parseToken(ID_MATCH, status);

    bool empty = true;
    // Parse selectors
    while (isWhitespace(source[index]) || source[index] == LEFT_CURLY_BRACE) {
        parseOptionalWhitespace(status);
        // Restore precondition
        CHECK_BOUNDS(source, index, parseError, status);
        if (source[index] != LEFT_CURLY_BRACE) {
            // This is not necessarily an error, but rather,
            // means the whitespace we parsed was the optional
            // whitespace preceding the first variant, not the
            // optional whitespace preceding a subsequent expression.
            break;
        }

        bool selectorError = false;
        Expression expression;
        expression = parseExpression(selectorError, status);
        if (selectorError) {
            // What happens if one of the variant keys is the
            // fallback string? this should be a `nomatch` according
            // to the spec, but there's no way to pass that through
            expression = exprFallback();
        }
        empty = false;

        dataModel.addSelector(std::move(expression), status);
        CHECK_ERROR(status);
    }

    // At least one selector is required
    if (empty) {
        ERROR(parseError, status, index);
        return;
    }

    #define CHECK_END_OF_INPUT                     \
        if (((int32_t)index) >= source.length()) { \
            break;                                 \
        }                                          \

    // Parse variants
    while (isWhitespace(source[index]) || source[index] == ID_WHEN[0]) {
        parseOptionalWhitespace(status);
        // Restore the precondition, *without* erroring out if we've
        // reached the end of input. That's because it's valid for the
        // message to end with trailing whitespace that follows a variant.
        CHECK_END_OF_INPUT

        // Consume the "when"
            parseToken(ID_WHEN, status);

        // At least one key is required
        SelectorKeys keyList(parseNonEmptyKeys(status));

        CHECK_ERROR(status);

        // parseNonEmptyKeys() consumes any trailing whitespace,
        // so the pattern can be consumed next.

        // Restore precondition before calling parsePattern()
        // (which must return a non-null value)
        CHECK_BOUNDS(source, index, parseError, status);
        Pattern rhs = parsePattern(status);

        dataModel.addVariant(std::move(keyList), std::move(rhs), status);

        // Restore the precondition, *without* erroring out if we've
        // reached the end of input. That's because it's valid for the
        // message to end with a variant that has no trailing whitespace.
        // Why do we need to check this condition twice inside the loop?
        // Because if we don't check it here, the `isWhitespace()` call in
        // the loop head will read off the end of the input string.
        CHECK_END_OF_INPUT
    }
}

/*
  Consume a `body` (matching the nonterminal in the grammar),
  No postcondition (on return, `index` might equal `source.length()` with no syntax error,
  because a message can end with a body (trailing whitespace is optional)
*/

void Parser::errorPattern(UErrorCode& status) {
    errors.addSyntaxError(status);
    // Set to empty pattern
    Pattern::Builder result = Pattern::Builder(status);
    CHECK_ERROR(status);

    // If still in bounds, then add the remaining input as a single text part
    // to the pattern
    /*
      TODO: this behavior isn't documented in the spec, but it comes from
      https://github.com/messageformat/messageformat/blob/e0087bff312d759b67a9129eac135d318a1f0ce7/packages/mf2-messageformat/src/__fixtures/test-messages.json#L236
      and a pending pull request https://github.com/unicode-org/message-format-wg/pull/462 will clarify
      whether this is the intent behind the spec
     */
    UnicodeString partStr(LEFT_CURLY_BRACE);
    while (inBounds(source, index)) {
        partStr += source[index++];
    }
    // Add curly braces around the entire output (same comment as above)
    partStr += RIGHT_CURLY_BRACE;
    result.add(PatternPart(partStr), status);
    dataModel.setPattern(result.build(status));
}

void Parser::parseBody(UErrorCode& status) {
    CHECK_ERROR(status);

    // Out-of-input is a syntax warning
    if (!inBounds(source, index)) {
        errorPattern(status);
        return;
    }

    // Body must be either a pattern or selectors
    switch (source[index]) {
    case LEFT_CURLY_BRACE: {
        // Pattern
        dataModel.setPattern(parsePattern(status));
        break;
    }
    case ID_MATCH[0]: {
        // Selectors
        parseSelectors(status);
        return;
    }
    default: {
        ERROR(parseError, status, index);
        errorPattern(status);
        return;
    }
    }
}

// -------------------------------------
// Parses the source pattern.

void Parser::parse(UParseError &parseErrorResult, UErrorCode& status) {
    CHECK_ERROR(status);

    // parseOptionalWhitespace(status) succeeds on an empty string, so don't check bounds yet
    parseOptionalWhitespace(status);
    // parseDeclarations() requires there to be input left, so check to see if
    // parseOptionalWhitespace(status) consumed it all

    // Skip the check if status is already set, so as to avoid overwriting a
    // previous error offset
    if (!inBounds(source, index)) {
        ERROR(parseError, status, index);
    }

    parseDeclarations(status);
    parseBody(status);
    CHECK_ERROR(status);

    parseOptionalWhitespace(status);

    // There are no errors; finally, check that the entire input was consumed
    if (((int32_t)index) != source.length()) {
      ERROR(parseError, status, index);
    }

    // Finally, copy the relevant fields of the internal `MessageParseError`
    // into the `UParseError` argument
    translateParseError(parseError, parseErrorResult);
}

Parser::~Parser() {}

} // namespace message2
U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */

