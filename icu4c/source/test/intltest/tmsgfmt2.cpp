// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html
/********************************************************************
 * COPYRIGHT: 
 * Copyright (c) 1997-2016, International Business Machines Corporation and
 * others. All Rights Reserved.
 ********************************************************************/

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/msgfmt2.h"
#include "tmsgfmt2.h"

/*
Tests reflect the syntax specified in

  https://github.com/unicode-org/message-format-wg/commits/main/spec/message.abnf

as of the following commit from 2023-05-09:
  https://github.com/unicode-org/message-format-wg/commit/194f6efcec5bf396df36a19bd6fa78d1fa2e0867
*/

/**
   TODO: For now, this just tests that valid messages are validated by the parser
   and invalid messages are rejected with an error reflecting the correct line/offset of
   the erroneous character
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
  "{Hello {$name :person formality=formal length=medium}}",
  0
};

/**
 * These tests are mostly from the test suite created for the JavaScript implementation of MessageFormat v2:
  <p>Original JSON file
  <a href="https://github.com/messageformat/messageformat/blob/master/packages/mf2-messageformat/src/__fixtures/test-messages.json">here</a>.</p>
  Some have been modified or added to reflect syntax changes that post-date this file.
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
                "{{-tag}}",
                // Modified next few patterns to reflect lack of special markup syntax
                "match {+foo} when * {foo}",
                "{{|content| +tag}}",
                "{{|content| -tag}}",
                "{{|content| +tag} {|content| -tag}}",
                "{content -tag}",
                "{{+tag foo=bar}}",
                "{{+tag foo=|foo| bar=$bar}}",
                "{{-tag foo=bar}}",
                "{content {|foo| +markup}}",
                "match {$foo} when * * {foo}", // Semantic error but syntactically correct
                "{}",
// tests for reserved syntax
                "{hello {|4.2| @number}}",
                "{hello {|4.2| @n|um|ber}}",
                "{hello {|4.2| &num|be|r}}",
                "{hello {|4.2| ?num|be||r|s}}",
                "{hello {|foo| !number}}",
                "{hello {|foo| *number}}",
                "{hello {#number}}",
                "match {$foo !select} when |1| {one} when * {other}",
                "match {$foo ^select} when |1| {one} when * {other}",
                "{{<tag}}",
                "let $bar = {$none ~plural} match {$foo} when * {{$bar}}",
// tests for ':' in nmtokens
                "match {$foo} when o:ne {one} when * {other}",
                "match {$foo} when one: {one} when * {other}",
                "let $foo = {$bar :fun option=a:b} {bar {$foo}}",
                "let $foo = {$bar :fun option=a:b:c} {bar {$foo}}",
                "let $foo = {$bar} match {$foo} when :one {one} when * {other}",
                "let $foo = {$bar :fun option=:a} {bar {$foo}}",
                0
};

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
    TESTCASE_AUTO(testInvalidPatterns);
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

template<size_t N>
void TestMessageFormat2::testPatterns(const UnicodeString (&patterns)[N], const char* testName) {
  for (uint32_t i = 0; i < N - 1; i++) {
    testPattern(patterns[i], i, testName);
  }
}

void TestMessageFormat2::testValidPatterns() {
  testPatterns(validTestCases, "testValidPatterns");
}

void TestMessageFormat2::testValidJsonPatterns() {
  testPatterns(jsonTestCasesValid, "testValidJsonPatterns");
}

void TestMessageFormat2::testInvalidPattern(uint32_t testNum, const UnicodeString& s) {
  // By default, the expected offset is the length of the message
  testInvalidPattern(testNum, s, s.length());
}

void TestMessageFormat2::testInvalidPattern(uint32_t testNum, const UnicodeString& s, uint32_t expectedErrorOffset) {
  UParseError parseError;
  IcuTestErrorCode errorCode(*this, "testInvalidPattern");

  MessageFormat2(s, parseError, errorCode);
  if (!U_FAILURE(errorCode)) {
    dataerrln("TestMessageFormat2::testInvalidPattern #%d - expected test to fail, but it passed", testNum);
    logln(UnicodeString("TestMessageFormat2::testInvalidPattern failed test ") + s + UnicodeString(" with error code ")+(int32_t)errorCode);
    return;
  } else {
    // Check the line and character numbers
    // For these test cases, the expected line number in the error is always 0.
    if (parseError.line != 0 || parseError.offset != ((int32_t) expectedErrorOffset)) {
      dataerrln("TestMessageFormat2::testInvalidPattern #%d - wrong line or character offset in parse error; expected (line %d, offset %d), got (line %d, offset %d)",
                testNum, 0, expectedErrorOffset, parseError.line, parseError.offset);
      logln(UnicodeString("TestMessageFormat2::testInvalidPattern failed #") + ((int32_t) testNum) + UnicodeString(" with error code ")+(int32_t)errorCode+" by returning the wrong line number or offset in the parse error");
    } else {
      errorCode.reset();
    }
  }
}

void TestMessageFormat2::testInvalidPatterns() {
  uint32_t i = 0;

  // Unexpected end of input
  testInvalidPattern(++i, "let    ");
  testInvalidPattern(++i, "let $foo");
  testInvalidPattern(++i, "let $foo =    ");
  testInvalidPattern(++i, "{{:fszzz");
  testInvalidPattern(++i, "match {$foo} when |xyz");
  testInvalidPattern(++i, "{{:f aaa");
  testInvalidPattern(++i, "{missing end brace");
  testInvalidPattern(++i, "{missing end {$brace");
  // @xyz is a valid annotation (`reserved`) so the error should be at the end of input
  testInvalidPattern(++i, "{{@xyz");

  // Missing '=' in `let` declaration
  testInvalidPattern(++i, "let $bar {|foo|} {{$bar}}", 9);

  // LHS of declaration doesn't start with a '$'
  testInvalidPattern(++i, "let bar = {|foo|} {{$bar}}", 4);

  // `let` RHS isn't an expression
  testInvalidPattern(++i, "let $bar = |foo| {{$bar}}", 11);

  // Non-expression
  testInvalidPattern(++i, "no braces", 0);
  testInvalidPattern(++i, "no braces {$foo}", 0);

  // Trailing characters that are not whitespace
  testInvalidPattern(++i, "{extra} content", 8);
  testInvalidPattern(++i, "match {|x|} when * {foo} extra", 25);

  // Empty expression
  testInvalidPattern(++i, "{empty { }}", 9);
  testInvalidPattern(++i, "match {} when * {foo}", 7);
  // ':' not preceding a function name
  testInvalidPattern(++i, "{bad {:}}", 7);
  // 'placeholder' is not a literal, variable or annotation
  testInvalidPattern(++i, "{bad {placeholder}}", 6);
  // Missing '=' after option name
  testInvalidPattern(++i, "{no-equal {|42| :number minimumFractionDigits 2}}", 46);
  testInvalidPattern(++i, "{bad {:placeholder option value}}", 26);
  // Missing RHS of option
  testInvalidPattern(++i, "{bad {:placeholder option=}}", 26);
  testInvalidPattern(++i, "{bad {:placeholder option}}", 25);
  // Annotation is not a function or reserved text
  testInvalidPattern(++i, "{bad {$placeholder option}}", 19);
  testInvalidPattern(++i, "{no {$placeholder end}", 18);

  // Missing whitespace before key in variant
  testInvalidPattern(++i, "match {|foo|} when*{foo}", 18);
  // Missing expression in selectors
  testInvalidPattern(++i, "match when * {foo}", 6);
  // Non-expression in selectors
  testInvalidPattern(++i, "match |x| when * {foo}", 6);

  // Missing RHS in variant
  testInvalidPattern(++i, "match {|x|} when * foo");
}

void TestMessageFormat2::testComplexMessage() {
  testPattern(complexMessage, 0, "testComplexMessage");
}

#endif /* #if !UCONFIG_NO_FORMATTING */
