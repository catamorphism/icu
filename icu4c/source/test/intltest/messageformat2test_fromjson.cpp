// © 2016 and later: Unicode, Inc. and others.

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "messageformat2test.h"

using namespace icu::message2;

/*
  TODO: Tests need to be unified in a single format that
  both ICU4C and ICU4J can use, rather than being embedded in code.

  Tests are included in their current state to give a sense of
  how much test coverage has been achieved. Most of the testing is
  of the parser/serializer; the formatter needs to be tested more
  thoroughly.
*/

/*
Tests reflect the syntax specified in

  https://github.com/unicode-org/message-format-wg/commits/main/spec/message.abnf

as of the following commit from 2023-05-09:
  https://github.com/unicode-org/message-format-wg/commit/194f6efcec5bf396df36a19bd6fa78d1fa2e0867

*/

/*
  Transcribed from https://github.com/messageformat/messageformat/blob/main/packages/mf2-messageformat/src/__fixtures/test-messages.json
https://github.com/messageformat/messageformat/commit/6656c95d66414da29a332a6f5bbb225371f2b9a3

*/
void TestMessageFormat2::jsonTests(IcuTestErrorCode& errorCode) {
    TestCase::Builder testBuilder;
    testBuilder.setName("jsonTests");

    TestCase test = testBuilder.setPattern("hello")
        .setExpected("hello")
        .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern("hello {|world|}")
                                .setExpected("hello world")
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern("hello {||}")
                                .setExpected("hello ")
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern("hello {$place}")
                                .setExpected("hello world")
                                .setArgument("place", "world")
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern("hello {$place:-.}")
                                .setExpected("hello world")
                                .setArgument("place:-.", "world")
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern("hello {$place}")
                                .setExpected("hello {$place}")
                                .clearArguments()
                                .setExpectedError(U_UNRESOLVED_VARIABLE_ERROR)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern("{$one} and {$two}")
                                .setExpected("1.3 and 4.2")
                                .setExpectSuccess()
                                .setArgument("one", 1.3)
                                .setArgument("two", 4.2)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);
    testBuilder.setArgument("one", "1.3").setArgument("two", "4.2");
    test = testBuilder.build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern("{$one} et {$two}")
                                .setExpected("1,3 et 4,2")
                                .setLocale(Locale("fr"))
                                .setArgument("one", 1.3)
                                .setArgument("two", 4.2)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern("hello {|4.2| :number}")
                                .setExpected("hello 4.2")
                                .setLocale(Locale("en"))
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern("hello {|foo| :number}")
                                .setExpected("hello NaN")
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern("hello {:number}")
                                .setExpected("hello NaN")
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);


    test = testBuilder.setPattern("hello {|4.2| :number minimumFractionDigits=2}")
                                .setExpected("hello 4.20")
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern("hello {|4.2| :number minimumFractionDigits=|2|}")
                                .setExpected("hello 4.20")
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern("hello {|4.2| :number minimumFractionDigits=$foo}")
                                .setExpected("hello 4.20")
                                .setArgument("foo", (int64_t) 2)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern(".local $foo = {bar} {{bar {$foo}}}")
                                .setExpected("bar bar")
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern(".local $foo = {|bar|} {{bar {$foo}}}")
                                .setExpected("bar bar")
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern(".local $foo = {|bar|} {{bar {$foo}}}")
                                .setExpected("bar bar")
                                .setArgument("foo", "foo")
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern(".local $foo = {$bar} {{bar {$foo}}}")
                                .setExpected("bar foo")
                                .setArgument("bar", "foo")
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern(".local $foo = {$bar :number} {{bar {$foo}}}")
                                .setExpected("bar 4.2")
                                .setArgument("bar", 4.2)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern(".local $foo = {$bar :number minimumFractionDigits=2} {{bar {$foo}}}")
                                .setExpected("bar 4.20")
                                .setArgument("bar", 4.2)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern(".local $foo = {$bar :number minimumFractionDigits=foo} {{bar {$foo}}}")
                                .setExpected("bar 4.2")
                                .setArgument("bar", 4.2)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern(".local $foo = {$bar :number} {{bar {$foo}}}")
                                .setExpected("bar NaN")
                                .setArgument("bar", "foo")
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern(".local $foo = {$baz} .local $bar = {$foo} {{bar {$bar}}}")
                                .setExpected("bar foo")
                                .setArgument("baz", "foo")
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern(".local $foo = {$foo} {{bar {$foo}}}")
                                .setExpected("bar foo")
                                .setArgument("foo", "foo")
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern(".local $foo = {$foo} .local $foo = {42} {{bar {$foo}}}")
                                .setExpected("bar 42")
                                .setArgument("foo", "foo")
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern(".local $foo = {42} .local $foo = {$foo} {{bar {$foo}}}")
                                .setExpected("bar 42")
                                .setArgument("foo", "foo")
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern(".local $foo = {$foo} .local $foo = {42} {{bar {$foo}}}")
                                .setExpected("bar 42")
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern(".local $foo = {:unknown} .local $foo = {42} {{bar {$foo}}}")
                                .setExpected("bar 42")
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern(".local $x = {42} .local $y = {$x} .local $x = {13} {{{$x} {$y}}}")
                                .setExpected("13 42")
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

/*
  Shouldn't this be "bar {$bar}"?

    test = testBuilder.setPattern(".local $foo = {$bar} .local $bar = {$baz} {{bar {$foo}}}")
                                .setExpected("bar foo")
                                .setArgument("baz", "foo", errorCode)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);
*/

    test = testBuilder.setPattern(".match {$foo :select} .when |1| {{one}} .when * {{other}}")
                                .setExpected("one")
                                .setArgument("foo", (int64_t) 1)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern(".match {$foo :plural} .when 1 {{one}} .when * {{other}}")
                                .setExpected("one")
                                .setArgument("foo", (int64_t) 1)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

/*
  This case can't be tested without a way to set the "foo" argument to null

    test = testBuilder.setPattern(".match {$foo :plural} .when 1 {{one}} .when * {{other}}")
                                .setExpected("other")
                                .setArgument("foo", "", errorCode)
                                .setExpectedError(U_UNRESOLVED_VARIABLE_ERROR)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);
*/

    test = testBuilder.setPattern(".match {$foo :plural} .when one {{one}} .when * {{other}}")
                                .setExpected("one")
                                .setArgument("foo", (int64_t) 1)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern(".match {$foo :plural} .when 1 {{=1}} .when one {{one}} .when * {{other}}")
                                .setExpected("=1")
                                .setArgument("foo", "1")
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern(".match {$foo :plural} .when 1 {{=1}} .when one {{one}} .when * {{other}}")
                                .setExpected("=1")
                                .setArgument("foo", (int64_t) 1)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern(".match {$foo :plural} .when one {{one}} .when 1 {{=1}} .when * {{other}}")
                                .setExpected("=1")
                                .setArgument("foo", (int64_t) 1)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern(".match {$foo :plural} {$bar :plural} .when one one {{one one}} .when one * {{one other}} .when * * {{other}}")
                                .setExpected("one one")
                                .setArgument("foo", (int64_t) 1)
                                .setArgument("bar", (int64_t) 1)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern(".match {$foo :plural} {$bar :plural} .when one one {{one one}} .when one * {{one other}} .when * * {{other}}")
                                .setExpected("one other")
                                .setArgument("foo", (int64_t) 1)
                                .setArgument("bar", (int64_t) 2)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern(".match {$foo :plural} {$bar :plural} .when one one {{one one}} .when one * {{one other}} .when * * {{other}}")
                                .setExpected("other")
                                .setArgument("foo", (int64_t) 2)
                                .setArgument("bar", (int64_t) 2)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern(".local $foo = {$bar :plural} .match {$foo} .when one {{one}} .when * {{other}}")
                                .setExpected("one")
                                .setArgument("bar", (int64_t) 1)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern(".local $foo = {$bar :plural} .match {$foo} .when one {{one}} .when * {{other}}")
                                .setExpected("other")
                                .setArgument("bar", (int64_t) 2)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern(".local $bar = {$none} .match {$foo :plural} .when one {{one}} .when * {{{$bar}}}")
                                .setExpected("one")
                                .setArgument("foo", (int64_t) 1)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

/*
  Note: this differs from https://github.com/messageformat/messageformat/blob/e0087bff312d759b67a9129eac135d318a1f0ce7/packages/mf2-messageformat/src/__fixtures/test-messages.json#L197

  The expected value in the test as defined there is "{$bar}".
  The value should be "{$none}" per 
https://github.com/unicode-org/message-format-wg/blob/main/spec/formatting.md#fallback-resolution -
".When an error occurs in an expression with a variable operand and the variable refers to a local declaration, the fallback value is formatted based on the expression on the right-hand side of the declaration, rather than the expression in the selector or pattern."
*/
    test = testBuilder.setPattern(".local $bar = {$none} .match {$foo :plural} .when one {{one}} .when * {{{$bar}}}")
                                .setExpected("{$none}")
                                .setArgument("foo", (int64_t) 2)
                                .setExpectedError(U_UNRESOLVED_VARIABLE_ERROR)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    // Missing '$' before `bar`
    test = testBuilder.setPattern(".local bar = {|foo|} {{{$bar}}}")
                                .setExpected("{$bar}")
                                .clearArguments()
                                .setExpectedError(U_SYNTAX_ERROR)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    // Missing '=' after `bar`
    /*
      Spec is ambiguous -- see https://github.com/unicode-org/message-format-wg/issues/703 --
      but we choose the '{$bar}' interpretation for the partial result
     */
    test = testBuilder.setPattern(".local $bar {|foo|} {{{$bar}}}")
                                .setExpected("{$bar}")
                                .setExpectedError(U_SYNTAX_ERROR)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    // Missing '{'/'}' around `foo`
    test = testBuilder.setPattern(".local $bar = |foo| {{{$bar}}}")
                                .setExpected("{$bar}")
                                .setExpectedError(U_SYNTAX_ERROR)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    // TODO: make sure there are round-trip tests for these
    // (add to runTestCase()?)

    // Markup is ignored when formatting to string
    test = testBuilder.setPattern("{#tag}")
                                .setExpectSuccess()
                                .setExpected("")
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern("{#tag/}")
                                .setExpected("")
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern("{/tag}")
                                .setExpected("")
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern("{#tag}content")
                      .setExpected("content")
                      .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern("{#tag}content{/tag}")
                      .setExpected("content")
                      .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern("{/tag}content")
                      .setExpected("content")
                      .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern("{#tag foo=bar}")
                      .setExpected("")
                      .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern("{#tag foo=|foo| bar=$bar}")
                      .setArgument("bar", "b a r")
                      .setExpected("")
                      .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern("{/tag foo=bar}")
                      .setExpected("")
                      .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern("no braces")
                      .setExpected("no braces")
                      .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern("no braces {$foo}")
                      .setExpected("no braces 2")
                      .setArgument("foo", (int64_t) 2)
                      .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern("{{missing end brace")
                      .setExpected("missing end brace")
                      .setExpectedError(U_SYNTAX_ERROR)
                      .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern("{{missing end {$brace")
                      .setExpected("missing end {$brace}")
                      .setExpectedError(U_SYNTAX_ERROR)
                      .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern("{extra} content")
                      .setExpected("extra content")
                      .setExpectSuccess()
                      .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern("{{extra}} content")
                      .setExpected("extra") // Everything after the closing '{{' should be ignored
                                            // per the `complex-body- production in the grammar
                      .setExpectedError(U_SYNTAX_ERROR)
                      .build();
    TestUtils::runTestCase(*this, test, errorCode);

    // "empty \0xfffd"
    static constexpr UChar emptyWithReplacement[] = {
        0x65, 0x6D, 0x70, 0x74, 0x79, 0x20, REPLACEMENT, 0
    };

    test = testBuilder.setPattern("empty { }")
                      .setExpectedError(U_SYNTAX_ERROR)
                      .setExpected(UnicodeString(emptyWithReplacement))
                      .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern("{{bad {:}}")
                      .setExpected("bad {:}")
                      .setExpectedError(U_SYNTAX_ERROR)
                      .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern("unquoted {literal}")
                      .setExpected("unquoted literal")
                      .setExpectSuccess()
                      .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern(CharsToUnicodeString("bad {\\u0000placeholder}"))
                      .clearExpected()
                      .setExpectedError(U_SYNTAX_ERROR)
                      .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern("no-equal {|42| :number minimumFractionDigits 2}")
                      .setExpected("no-equal 42.00")
                      .setExpectedError(U_SYNTAX_ERROR)
                      .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern("bad {:placeholder option=}")
                      .setExpected("bad {:placeholder}")
                      .setExpectedError(U_SYNTAX_ERROR)
                      .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern("bad {:placeholder option value}")
                      .setExpected("bad {:placeholder}")
                      .setExpectedError(U_SYNTAX_ERROR)
                      .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern("bad {:placeholder option}")
                      .setExpected("bad {:placeholder}")
                      .setExpectedError(U_SYNTAX_ERROR)
                      .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern("bad {$placeholder option}")
                      .clearExpected()
                      .setExpectedError(U_SYNTAX_ERROR)
                      .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern("no {$placeholder end")
                      .clearExpected()
                      .setExpectedError(U_SYNTAX_ERROR)
                      .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern(".match {} .when * {{foo}}")
                      .clearExpected()
                      .setExpectedError(U_SYNTAX_ERROR)
                      .build();
    TestUtils::runTestCase(*this, test, errorCode);

    // "empty \0xfffd"
    static constexpr UChar replacement[] = {
        REPLACEMENT, 0
    };

    test = testBuilder.setPattern(".match {#foo} .when * {{foo}}")
                      .setExpected(UnicodeString(replacement))
                      .setExpectedError(U_SYNTAX_ERROR)
                      .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern(".match {|foo|} .when*{{foo}}")
                      .clearExpected()
                      .setExpectedError(U_SYNTAX_ERROR)
                      .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern(".match .when * {{foo}}")
                      .clearExpected()
                      .setExpectedError(U_SYNTAX_ERROR)
                      .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern(".match {|x|} .when * foo")
                      .clearExpected()
                      .setExpectedError(U_SYNTAX_ERROR)
                      .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern(".match {|x|} .when * {{foo}} extra")
                      .clearExpected()
                      .setExpectedError(U_SYNTAX_ERROR)
                      .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern(".match |x| .when * {{foo}}")
                      .clearExpected()
                      .setExpectedError(U_SYNTAX_ERROR)
                      .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern(".match {$foo :plural} .when * * {{foo}}")
                      .clearExpected()
                      .setExpectedError(U_VARIANT_KEY_MISMATCH_ERROR)
                      .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern(".match {$foo :plural} {$bar :plural} .when * {{foo}}")
                      .clearExpected()
                      .setExpectedError(U_VARIANT_KEY_MISMATCH_ERROR)
                      .build();
    TestUtils::runTestCase(*this, test, errorCode);
}

#endif /* #if !UCONFIG_NO_FORMATTING */
