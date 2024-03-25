// © 2016 and later: Unicode, Inc. and others.

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/calendar.h"
#include "messageformat2test.h"

using namespace icu::message2;

/*
Tests reflect the syntax specified in

  https://github.com/unicode-org/message-format-wg/commits/main/spec/message.abnf

as of the following commit from 2023-05-09:
  https://github.com/unicode-org/message-format-wg/commit/194f6efcec5bf396df36a19bd6fa78d1fa2e0867
*/

void TestMessageFormat2::testDateTime(IcuTestErrorCode& errorCode) {
    CHECK_ERROR(errorCode);

    LocalPointer<Calendar> cal(Calendar::createInstance(errorCode));
    TestCase::Builder testBuilder;

    testBuilder.setName("testDateTime");
    // November 23, 2022 at 7:42:37.123 PM
    cal->set(2022, Calendar::NOVEMBER, 23, 19, 42, 37);
    UDate TEST_DATE = cal->getTime(errorCode);
    UnicodeString date = "date";
    testBuilder.setLocale(Locale("ro"));

    TestCase test = testBuilder.setPattern("{Testing date formatting: {$date :datetime}.}")
                                .setExpected("Testing date formatting: 23.11.2022, 19:42.")
                                .setDateArgument(date, TEST_DATE)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    testBuilder.setLocale(Locale("ro", "RO"));

    // Skeleton
    test = testBuilder.setPattern("{Testing date formatting: {$date :datetime skeleton=yMMMMd}.}")
                                .setExpected("Testing date formatting: 23 noiembrie 2022.")
                                .setDateArgument(date, TEST_DATE)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern("{Testing date formatting: {$date :datetime skeleton=jm}.}")
                                .setExpected("Testing date formatting: 19:42.")
                                .setDateArgument(date, TEST_DATE)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    testBuilder.setLocale(Locale("en"));

    test = testBuilder.setPattern("{Testing date formatting: {$date :datetime skeleton=yMMMd}.}")
                                .setExpected("Testing date formatting: Nov 23, 2022.")
                                .setDateArgument(date, TEST_DATE)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern("{Testing date formatting: {$date :datetime skeleton=yMMMdjms}.}")
                                .setExpected(CharsToUnicodeString("Testing date formatting: Nov 23, 2022, 7:42:37\\u202FPM."))
                                .setDateArgument(date, TEST_DATE)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern("{Testing date formatting: {$date :datetime skeleton=jm}.}")
                                .setExpected(CharsToUnicodeString("Testing date formatting: 7:42\\u202FPM."))
                                .setDateArgument(date, TEST_DATE)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

   // Style

    test = testBuilder.setPattern("{Testing date formatting: {$date :datetime datestyle=long}.}")
                                .setExpected("Testing date formatting: November 23, 2022.")
                                .setDateArgument(date, TEST_DATE)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern("{Testing date formatting: {$date :datetime datestyle=medium}.}")
                                .setExpected("Testing date formatting: Nov 23, 2022.")
                                .setDateArgument(date, TEST_DATE)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern("{Testing date formatting: {$date :datetime datestyle=short}.}")
                                .setExpected("Testing date formatting: 11/23/22.")
                                .setDateArgument(date, TEST_DATE)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern("{Testing date formatting: {$date :datetime timestyle=long}.}")
                                .setExpected(CharsToUnicodeString("Testing date formatting: 7:42:37\\u202FPM PST."))
                                .setDateArgument(date, TEST_DATE)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern("{Testing date formatting: {$date :datetime timestyle=medium}.}")
                                .setExpected(CharsToUnicodeString("Testing date formatting: 7:42:37\\u202FPM."))
                                .setDateArgument(date, TEST_DATE)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern("{Testing date formatting: {$date :datetime timestyle=short}.}")
                                .setExpected(CharsToUnicodeString("Testing date formatting: 7:42\\u202FPM."))
                                .setDateArgument(date, TEST_DATE)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    // Pattern
    test = testBuilder.setPattern("{Testing date formatting: {$date :datetime pattern=|d 'of' MMMM, y 'at' HH:mm|}.}")
                                .setExpected("Testing date formatting: 23 of November, 2022 at 19:42.")
                                .setDateArgument(date, TEST_DATE)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    // Error cases
    // Number as argument
    test = testBuilder.setPattern("let $num = {|42| :number}\n\
                                              {Testing date formatting: {$num :datetime}}")
                                .clearArguments()
                                .setExpected("Testing date formatting: {|42|}")
                                .setExpectedError(U_FORMATTING_ERROR)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);
    // Literal string as argument
    test = testBuilder.setPattern("{Testing date formatting: {|horse| :datetime}}")
                                .setExpected("Testing date formatting: |horse|")
                                .setExpectedError(U_FORMATTING_ERROR)
                                .build();
    // Formatted string as argument
    test = testBuilder.setPattern("let $dateStr = {$date :datetime}\n\
                                               {Testing date formatting: {$dateStr :datetime}}")
                                .setExpected("Testing date formatting: {$date}")
                                .setExpectedError(U_FORMATTING_ERROR)
                                .setDateArgument(date, TEST_DATE)
                                .build();

    TestUtils::runTestCase(*this, test, errorCode);

}

void TestMessageFormat2::testNumbers(IcuTestErrorCode& errorCode) {
    CHECK_ERROR(errorCode);

    double value = 1234567890.97531;
    UnicodeString val = "val";

    TestCase::Builder testBuilder;
    testBuilder.setName("testNumbers");

    // Literals
    TestCase test = testBuilder.setPattern("{From literal: {123456789 :number}!}")
        .setArgument(val, value)
        .setExpected("From literal: 123.456.789!")
        .setLocale(Locale("ro"))
        .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern("{From literal: {|123456789,531| :number}!}")
                                .setArgument(val, value)
                                .setExpected("From literal: 123.456.789,531!")
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern(CharsToUnicodeString("{From literal: {|\\u1041\\u1042\\u1043,\\u1044\\u1045\\u1046,\\u1047\\u1048\\u1049.\\u1045\\u1043\\u1041| :number}!}"))
                                .setArgument(val, value)
                                .setExpected(CharsToUnicodeString("From literal: \\u1041\\u1042\\u1043,\\u1044\\u1045\\u1046,\\u1047\\u1048\\u1049.\\u1045\\u1043\\u1041!"))
                                .setLocale(Locale("my"))
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);


    // Testing that the detection works for various types (without specifying :number)
    test = testBuilder.setPattern("{Default double: {$val}!}")
                                .setLocale(Locale("en", "IN"))
                                .setArgument(val, value)
                                .setExpected("Default double: 1,23,45,67,890.97531!")
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);
    test = testBuilder.setPattern("{Default double: {$val}!}")
                                .setLocale(Locale("ro"))
                                .setArgument(val, value)
                                .setExpected("Default double: 1.234.567.890,97531!")
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);
    test = testBuilder.setPattern("{Default float: {$val}!}")
                                .setLocale(Locale("ro"))
                                .setArgument(val, 3.1415926535)
                                .setExpected("Default float: 3,141593!")
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);
    test = testBuilder.setPattern("{Default int64: {$val}!}")
                                .setLocale(Locale("ro"))
                                .setArgument(val, (int64_t) 1234567890123456789)
                                .setExpected("Default int64: 1.234.567.890.123.456.789!")
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);
    test = testBuilder.setPattern("{Default number: {$val}!}")
                                .setLocale(Locale("ro"))
                                .setDecimalArgument(val, StringPiece("1234567890123456789.987654321"), errorCode)
                                .setExpected("Default number: 1.234.567.890.123.456.789,987654!")
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    // Omitted CurrencyAmount test from ICU4J since it's not supported by Formattable

   // Skeletons
    value = 1234567890.97531;
    testBuilder.setLocale(Locale("ro"));
    test = testBuilder.setPattern("{Skeletons, minFraction: {$val :number skeleton=|.00000000*|}!}")
                                .setArgument(val, value)
                                .setExpected("Skeletons, minFraction: 1.234.567.890,97531000!")
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern("{Skeletons, maxFraction: {$val :number skeleton=|.###|}!}")
                                .setArgument(val, value)
                                .setExpected("Skeletons, maxFraction: 1.234.567.890,975!")
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    // Currency
    test = testBuilder.setPattern("{Skeletons, currency: {$val :number skeleton=|currency/EUR|}!}")
                                .setArgument(val, value)
                                .setExpected(CharsToUnicodeString("Skeletons, currency: 1.234.567.890,98\\u00A0\\u20AC!"))
                                .setLocale(Locale("de"))
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    // Currency as a parameter
    test = testBuilder.setPattern("{Skeletons, currency: {$val :number skeleton=$skel}!}")
                                .setArgument(val, value)
                                .setArgument("skel", "currency/EUR")
                                .setExpected(CharsToUnicodeString("Skeletons, currency: 1.234.567.890,98\\u00A0\\u20AC!"))
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern("{Skeletons, currency: {$val :number skeleton=$skel}!}")
                                .setArgument(val, value)
                                .setArgument("skel", "currency/JPY")
                                .setExpected(CharsToUnicodeString("Skeletons, currency: 1.234.567.891\\u00A0\\u00A5!"))
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    // Various measures
    test = testBuilder.setPattern("let $intl = {$valC :number skeleton=|unit/celsius|}\n\
                                 let $us = {$valF :number skeleton=|unit/fahrenheit|}\n\
                                 {Temperature: {$intl} ({$us})}")
                                .setArgument("valC", 27.0)
                                .setArgument("valF", 80.6)
                                .setExpected(CharsToUnicodeString("Temperature: 27 \\u00B0C (80,6 \\u00B0F)"))
                                .setLocale(Locale("ro"))
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern("{Height: {$len :number skeleton=|unit/meter|}}")
                                .setArgument("len", 1.75)
                                .setExpected("Height: 1,75 m")
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);
}

void TestMessageFormat2::testBuiltInFunctions() {
  IcuTestErrorCode errorCode(*this, "testBuiltInFunctions");

  testDateTime(errorCode);
  testNumbers(errorCode);
}

#endif /* #if !UCONFIG_NO_FORMATTING */
