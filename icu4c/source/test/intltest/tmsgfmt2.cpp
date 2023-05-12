// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2016, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

// TODO remove unused includes

#include "tmsgfmt2.h"
#include "cmemory.h"
#include "loctest.h"

#include "unicode/errorcode.h"
#include "unicode/normalizer2.h"
#include "unicode/sortkey.h"
#include "unicode/std_string.h"
#include "unicode/stringpiece.h"
#include "unicode/tblcoll.h"
#include "unicode/uiter.h"
#include "unicode/uniset.h"
#include "unicode/unistr.h"
#include "unicode/usetiter.h"
#include "unicode/ustring.h"

#include "unicode/format.h"
#include "unicode/decimfmt.h"
#include "unicode/localpointer.h"
#include "unicode/locid.h"
#include "unicode/msgfmt.h"
#include "unicode/numfmt.h"
#include "unicode/choicfmt.h"
#include "unicode/messagepattern.h"
#include "unicode/selfmt.h"
#include "unicode/gregocal.h"
#include "unicode/strenum.h"
#include <stdio.h>
#include "uhash.h"

/*
  TODO: clean up, check against coding conventions

  TODO: Add tests for syntax changes from 2023-04-10 onward, here:
  https://github.com/unicode-org/message-format-wg/commits/main/spec/message.abnf
*/

/**
   TODO: For now, this just tests that valid messages are validated by the parser
   and that a certain set is not validated

   TODO: tests should check that the error diagnostics (both the message and line and character
   numbers) are correct. (Also, the parser should give good diagnostics, which it currently does
   not.)
**/
UnicodeString validTestCases[] = {
  /* From Mf2IcuTest.java */
  "{There are {$count} files on {$where}}",
  "{At {$when :datetime timestyle=default} on {$when :datetime datestyle=default}, \
      there was {$what} on planet {$planet :number kind=integer}.}",
  "{The disk \"{$diskName}\" contains {$fileCount} file(s).}",
  "match {$userGender :select}\n\
     when female {{$userName} est all\u00E9e \u00E0 Paris.} \
     when  *     {{$userName} est all\u00E9 \u00E0 Paris.}",
  "{{$when :datetime skeleton=MMMMd}}",
// Edited this from testMessageFormatDateTimeSkeleton() -- nmtokens can't contain spaces
  "{{$when :datetime skeleton=|(   yMMMMd   )|}}",
  "{Expiration: {$when :datetime skeleton=yMMM}!}",
  "{Hello {$user}, today is {$today :datetime datestyle=long}.}",
// Edited this from testMessageFormatDateTimeSkeleton() -- nmtokens can't contain parentheses or single quotation marks
  "{{$when :datetime pattern=|('::'yMMMMd)|}}",
  /* From CustomFormatterMessageRefTest.java */
  "match {$gcase :select} when genitive {Firefoxin} when * {Firefox}",
  /* From CustomFormatterPersonTest.java */
  "{Hello {$name :person formality=formal length=medium}}"
};

#define NUM_VALID_TEST_CASES 11

/**
 * These tests come from the test suite created for the JavaScript implementation of MessageFormat v2.
 *
 */

UnicodeString jsonTestCasesValid[] = {
                "{hello}",
                "{hello {|world|}}",
                "{hello {||}}",
                "{hello {$place}}",
                "{{$one} and {$two}}",
                "{{$one} et {$two}}",
                "{hello {|4.2| :number}}",
                "{hello {|4.2| :number}}",
                "{hello {|foo| :number}}",
                "{hello {:number}}",
                "{hello {|4.2| :number minimumFractionDigits=2}}",
                "{hello {|4.2| :number minimumFractionDigits=|2|}}",
                "{hello {|4.2| :number minimumFractionDigits=$foo}}",
                "let $foo = {|bar|} {bar {$foo}}",
                "let $foo = {$bar} {bar {$foo}}",
                "let $foo = {$bar :number} {bar {$foo}}",
                "let $foo = {$bar :number minimumFractionDigits=2} {bar {$foo}}",
                "let $foo = {$bar :number minimumFractionDigits=foo} {bar {$foo}}",
                "let $foo = {$bar :number} {bar {$foo}}",
                "let $foo = {$bar} let $bar = {$baz} {bar {$foo}}",
                "match {$foo} when |1| {one} when * {other}",
                "match {$foo :select} when |1| {one} when * {other}",
                "match {$foo :plural} when 1 {one} when * {other}",
                "match {$foo} when 1 {one} when * {other}",
                "match {$foo :plural} when 1 {one} when * {other}",
                "match {$foo} when one {one} when * {other}",
                "match {$foo :plural} when one {one} when * {other}",
                "match {$foo} when 1 {=1} when one {one} when * {other}",
                "match {$foo :plural} when 1 {=1} when one {one} when * {other}",
                "match {$foo} when one {one} when 1 {=1} when * {other}",
                "match {$foo :plural} when one {one} when 1 {=1} when * {other}",
                "match {$foo} {$bar} when one one {one one} when one * {one other} when * * {other}",
                "match {$foo :plural} {$bar :plural} when one one {one one} when one * {one other} when * * {other}",
                "let $foo = {$bar} match {$foo} when one {one} when * {other}",
                "let $foo = {$bar} match {$foo :plural} when one {one} when * {other}",
                "let $foo = {$bar} match {$foo} when one {one} when * {other}",
                "let $foo = {$bar} match {$foo :plural} when one {one} when * {other}",
                "let $bar = {$none} match {$foo} when one {one} when * {{$bar}}",
                "let $bar = {$none} match {$foo :plural} when one {one} when * {{$bar}}",
                "let $bar = {$none} match {$foo} when one {one} when * {{$bar}}",
                "let $bar = {$none :plural} match {$foo} when one {one} when * {{$bar}}",
                "{{+tag}}", // Modified next few patterns to reflect lack of special markup syntax
                "match {+foo} when * {foo}",
                "{{|content| +tag}}",
                "{{|content| -tag}}",
                "{{|content| +tag} {|content| -tag}}",
                "{content -tag}",
                "{{+tag foo=bar}}",
                "{{+tag foo=|foo| bar=$bar}}",
                "{{-tag foo=bar}}",
                "{content {|foo| +markup}}",
                "match {$foo} when * * {foo}" // Semantic error but syntactically correct 
};

#define NUM_VALID_JSON_TEST_CASES 50

// TODO use null-terminated array and removed the #defined lengths

// TODO: rename this and add comment saying "these came from [whatever .java file],
// to avoid confusion
UnicodeString jsonTestCasesInvalid[] = {
                "let    ",
                "let $foo",
                "let $foo =    ",
                "{{:fszzz",
                "match {$foo} when |xyz",
                "{{:f aaa",
                "{{@xyz",
                "let $bar {|foo|} {{$bar}}",
                "let bar = {|foo|} {{$bar}}",
                "let $bar = |foo| {{$bar}}",
                "no braces",
                "no braces {$foo}",
                "{missing end brace",
                "{missing end {$brace",
                "{extra} content",
                "{empty { }}",
                "{bad {:}}",
                "{bad {placeholder}}",
                "{no-equal {|42| :number minimumFractionDigits 2}}",
                "{bad {:placeholder option=}}",
                "{bad {:placeholder option value}}",
                "{bad {:placeholder option}}",
                "{bad {$placeholder option}}",
                "{no {$placeholder end}",
                "match {} when * {foo}",
                "match {|foo|} when*{foo}",
                "match when * {foo}",
                "match {|x|} when * foo",
                "match {|x|} when * {foo} extra",
                "match |x| when * {foo}",
                "{}"
};

// This has to be kept in sync! Yuck!
// TODO
int32_t errorOffsets[] = {
  -1, // this means "use the length of the string"
  -1,
  -1,
  -1,
  -1,
  -1,
  -1, // @xyz is a valid annotation (`reserved`) so the error should be at the end of input
  9, // missing = in `let` decl
  4, // `let` lhs doesn't start with a '$'
  11, // start of `let` rhs is not an expression
  0, // not an expression
  0, // not an expression,
  -1,
  -1,
  8, // trailing non-whitespace
  9, // empty expression
  7, // ':' that isn't a prefix
  6, // expected a literal or variable here
  46, // missing '=' after option name
  26, // missing rhs of option
  26, // missing '=' after option name
  25, // missing rhs of option
  19, // context requires an annotation, this isn't one
  18, // context requires an annotation, "end" isn't one
  7, // empty expression
  18, // put a whitespace before a key
  6, // need at least one expression to match on
  -1, // case arm has no rhs
  25, // context requires either a "when" keyword or whitespace
  6, // need an expression to match on (`|x|` isn't an expression)
  1, // parsed as pattern -> text; text is empty
};

#define NUM_INVALID_JSON_TEST_CASES 30

UnicodeString complexMessage = "\
                let $hostName = {$host :person length=long}\n\
                let $guestName = {$guest :person length=long}\n\
                let $guestsOther = {$guestCount :number offset=1}\n\
                \n\
                match {$hostGender :gender} {$guestCount :plural}\n\
                when female 0 {{$hostName} does not give a party.}\n\
                when female 1 {{$hostName} invites {$guestName} to her party.}\n\
                when female 2 {{$hostName} invites {$guestName} and one other person to her party.}\n\
                when female * {{$hostName} invites {$guestName} and {$guestsOther} other people to her party.}\n\
                \n\
                when male 0 {{$hostName} does not give a party.}\n\
                when male 1 {{$hostName} invites {$guestName} to his party.}\n\
                when male 2 {{$hostName} invites {$guestName} and one other person to his party.}\n\
                when male * {{$hostName} invites {$guestName} and {$guestsOther} other people to his party.}\n\
                \n\
                when * 0 {{$hostName} does not give a party.}\n\
                when * 1 {{$hostName} invites {$guestName} to their party.}\n\
                when * 2 {{$hostName} invites {$guestName} and one other person to their party.}\n\
                when * * {{$hostName} invites {$guestName} and {$guestsOther} other people to their party.}\n";

void
TestMessageFormat2::runIndexedTest(int32_t index, UBool exec,
                                  const char* &name, char* /*par*/) {
    TESTCASE_AUTO_BEGIN;
    TESTCASE_AUTO(testInvalidJsonPatterns);
    TESTCASE_AUTO(testValidJsonPatterns);
    TESTCASE_AUTO(testValidPatterns);
    TESTCASE_AUTO(testComplexMessage);
    TESTCASE_AUTO_END;
}

void
TestMessageFormat2::testPattern(const UnicodeString& s, uint32_t i, const char* testName) {
  UParseError parseError;
  IcuTestErrorCode errorCode(*this, testName);

  MessageFormat2(s, parseError, errorCode);

  if (U_FAILURE(errorCode)) {
    dataerrln(s);
    dataerrln("TestMessageFormat2::%s #%d - %s", testName, i, u_errorName(errorCode));
    dataerrln("TestMessageFormat2::%s #%d - %d %d", testName, i, parseError.line, parseError.offset);
    logln(UnicodeString("TestMessageFormat2::" + UnicodeString(testName) + " failed test #") + ((int32_t) i) + UnicodeString(" with error code ")+(int32_t)errorCode);
  }
}

void TestMessageFormat2::testPatterns(UnicodeString* patterns, uint32_t numPatterns, const char* testName) {
  for (uint32_t i = 0; i < numPatterns; i++) {
    testPattern(patterns[i], i, testName);
  }
}

void TestMessageFormat2::testValidPatterns() {
  testPatterns(validTestCases, NUM_VALID_TEST_CASES, "testValidPatterns");
}

void TestMessageFormat2::testValidJsonPatterns() {
  testPatterns(jsonTestCasesValid, NUM_VALID_JSON_TEST_CASES, "testValidJsonPatterns");
}

void TestMessageFormat2::testInvalidJsonPatterns() {
  UParseError parseError;
  IcuTestErrorCode errorCode(*this, "testInvalidJsonPatterns");

  for (uint32_t i = 0; i < NUM_INVALID_JSON_TEST_CASES; i++) {
    // For these test cases, the expected line number in the error is always 0.
    // Find the expected character number
    uint32_t expectedErrorOffset = (errorOffsets[i] == -1 ? jsonTestCasesInvalid[i].length() : errorOffsets[i]);

    MessageFormat2(jsonTestCasesInvalid[i], parseError, errorCode);
    if (!U_FAILURE(errorCode)) {
        dataerrln("TestMessageFormat2::testInvalidJsonPatterns #%d - expected test to fail, but it passed", i);
        logln(UnicodeString("TestMessageFormat2::testInvalidJsonPatterns failed test #") + ((int32_t) i) + UnicodeString(" with error code ")+(int32_t)errorCode);
        return;
    } else {
      // Check the line and character numbers
      if (parseError.line != 0 || parseError.offset != ((int32_t) expectedErrorOffset)) {
        dataerrln("TestMessageFormat2::testInvalidJsonPatterns #%d - wrong line or character offset in parse error; expected (line %d, offset %d), got (line %d, offset %d)",
                  i, 0, expectedErrorOffset, parseError.line, parseError.offset);
        logln(UnicodeString("TestMessageFormat2::testInvalidJsonPatterns failed test #") + ((int32_t) i) + UnicodeString(" with error code ")+(int32_t)errorCode+" by returning the wrong line number or offset in the parse error");        
      } else {
        errorCode.reset();
      }
    }
  }
}


void TestMessageFormat2::testComplexMessage() {
  testPattern(complexMessage, 0, "testComplexMessage");
}

#endif /* #if !UCONFIG_NO_FORMATTING */
