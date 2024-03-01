// © 2016 and later: Unicode, Inc. and others.

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/gregocal.h"
#include "messageformat2test.h"

using namespace icu::message2;
using namespace data_model;

/*
  Tests based on ICU4J's MessageFormat2Test.java
and Mf2FeaturesTest.java
*/

/*
  TODO: Tests need to be unified in a single format that
  both ICU4C and ICU4J can use, rather than being embedded in code.
*/

/*
Tests reflect the syntax specified in

  https://github.com/unicode-org/message-format-wg/commits/main/spec/message.abnf

as of the following commit from 2023-05-09:
  https://github.com/unicode-org/message-format-wg/commit/194f6efcec5bf396df36a19bd6fa78d1fa2e0867

*/

void TestMessageFormat2::testEmptyMessage(TestCase::Builder& testBuilder, IcuTestErrorCode& errorCode) {
    TestUtils::runTestCase(*this, testBuilder.setPattern("")
                           .setExpected("")
                           .build(), errorCode);
}

void TestMessageFormat2::testPlainText(TestCase::Builder& testBuilder, IcuTestErrorCode& errorCode) {
    TestUtils::runTestCase(*this, testBuilder.setPattern("Hello World!")
                           .setExpected("Hello World!")
                           .build(), errorCode);
}

void TestMessageFormat2::testPlaceholders(TestCase::Builder& testBuilder, IcuTestErrorCode& errorCode) {
    TestUtils::runTestCase(*this, testBuilder.setPattern("Hello, {$userName}!")
                                .setExpected("Hello, John!")
                                .setArgument("userName", "John")
                                .build(), errorCode);
}

void TestMessageFormat2::testArgumentMissing(TestCase::Builder& testBuilder, IcuTestErrorCode& errorCode) {
    CHECK_ERROR(errorCode);

    UnicodeString message = "Hello {$name}, today is {$today :datetime skeleton=yMMMMdEEEE}.";
    LocalPointer<Calendar> cal(Calendar::createInstance(errorCode));
    CHECK_ERROR(errorCode);

    // November 23, 2022 at 7:42:37.123 PM
    cal->set(2022, Calendar::NOVEMBER, 23, 19, 42, 37);
    UDate TEST_DATE = cal->getTime(errorCode);
    CHECK_ERROR(errorCode);

    TestCase test = testBuilder.setPattern(message)
        .clearArguments()
        .setArgument("name", "John")
        .setDateArgument("today", TEST_DATE)
        .setExpected("Hello John, today is Wednesday, November 23, 2022.")
        .build();
    TestUtils::runTestCase(*this, test, errorCode);

    // Missing date argument
    test = testBuilder.setPattern(message)
                                .clearArguments()
                                .setArgument("name", "John")
                                .setExpected("Hello John, today is {$today}.")
                                .setExpectedError(U_UNRESOLVED_VARIABLE_ERROR)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);
    test = testBuilder.setPattern(message)
                                .clearArguments()
                                .setDateArgument("today", TEST_DATE)
                                .setExpectedError(U_UNRESOLVED_VARIABLE_ERROR)
                                .setExpected("Hello {$name}, today is Wednesday, November 23, 2022.")
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    // Both arguments missing
    test = testBuilder.setPattern(message)
                                .clearArguments()
                                .setExpectedError(U_UNRESOLVED_VARIABLE_ERROR)
                                .setExpected("Hello {$name}, today is {$today}.")
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);
}

void TestMessageFormat2::testDefaultLocale(TestCase::Builder& testBuilder, IcuTestErrorCode& errorCode) {
    CHECK_ERROR(errorCode);

    LocalPointer<Calendar> cal(Calendar::createInstance(errorCode));
    CHECK_ERROR(errorCode);
    // November 23, 2022 at 7:42:37.123 PM
    cal->set(2022, Calendar::NOVEMBER, 23, 19, 42, 37);
    UDate TEST_DATE = cal->getTime(errorCode);
    CHECK_ERROR(errorCode);

    UnicodeString message = "Date: {$date :datetime skeleton=yMMMMdEEEE}.";
    UnicodeString expectedEn = "Date: Wednesday, November 23, 2022.";
    UnicodeString expectedRo = "Date: miercuri, 23 noiembrie 2022.";

    testBuilder.setPattern(message);

    TestCase test = testBuilder.clearArguments()
        .setDateArgument("date", TEST_DATE)
        .setExpected(expectedEn)
        .setExpectSuccess()
        .build();
    TestUtils::runTestCase(*this, test, errorCode);
    test = testBuilder.setExpected(expectedRo)
                                .setLocale(Locale("ro"))
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    Locale originalLocale = Locale::getDefault();
    Locale::setDefault(Locale::forLanguageTag("ro", errorCode), errorCode);
    CHECK_ERROR(errorCode);

    test = testBuilder.setExpected(expectedEn)
                                .setLocale(Locale("en", "US"))
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);
    test = testBuilder.setExpected(expectedRo)
                                .setLocale(Locale::forLanguageTag("ro", errorCode))
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    Locale::setDefault(originalLocale, errorCode);
    CHECK_ERROR(errorCode);
}

void TestMessageFormat2::testSpecialPluralWithDecimals(TestCase::Builder& testBuilder, IcuTestErrorCode& errorCode) {
    CHECK_ERROR(errorCode);

    UnicodeString message;

    message = ".local $amount = {$count :number}\n\
                .match {$amount :plural}\n\
                  .when 1 {{I have {$amount} dollar.}}\n\
                  .when * {{I have {$amount} dollars.}}\n";

    TestCase test = testBuilder.setPattern(message)
        .clearArguments()
        .setArgument("count", (int64_t) 1)
        .setExpected("I have 1 dollar.")
        .setLocale(Locale("en", "US"))
        .build();
    TestUtils::runTestCase(*this, test, errorCode);

    message = ".local $amount = {$count :number skeleton=|.00*|}\n\
                .match {$amount :plural skeleton=|.00*|}\n\
                  .when 1 {{I have {$amount} dollar.}}\n\
                  .when * {{I have {$amount} dollars.}}\n";

    test = testBuilder.setPattern(message)
                                .setExpected("I have 1.00 dollar.")
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);
}

void TestMessageFormat2::testDefaultFunctionAndOptions(TestCase::Builder& testBuilder, IcuTestErrorCode& errorCode) {
    CHECK_ERROR(errorCode);

    LocalPointer<Calendar> cal(Calendar::createInstance(errorCode));
    CHECK_ERROR(errorCode);
    // November 23, 2022 at 7:42:37.123 PM
    cal->set(2022, Calendar::NOVEMBER, 23, 19, 42, 37);
    UDate TEST_DATE = cal->getTime(errorCode);
    CHECK_ERROR(errorCode);

    TestCase test = testBuilder.setPattern("Testing date formatting: {$date}.")
        .clearArguments()
        .setDateArgument("date", TEST_DATE)
        .setExpected("Testing date formatting: 23.11.2022, 19:42.")
        .setLocale(Locale("ro"))
        .build();
    TestUtils::runTestCase(*this, test, errorCode);
    test = testBuilder.setPattern("Testing date formatting: {$date :datetime}.")
                                .setExpected("Testing date formatting: 23.11.2022, 19:42.")
                                .setLocale(Locale("ro"))
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);
}

void TestMessageFormat2::testSimpleSelection(TestCase::Builder& testBuilder, IcuTestErrorCode& errorCode) {
    (void) testBuilder;
    (void) errorCode;

    /* Covered by testPlural */
}

void TestMessageFormat2::testComplexSelection(TestCase::Builder& testBuilder, IcuTestErrorCode& errorCode) {
    CHECK_ERROR(errorCode);

    UnicodeString message = ".match {$photoCount :plural} {$userGender :select}\n\
                 .when 1 masculine {{{$userName} added a new photo to his album.}}\n\
                 .when 1 feminine {{{$userName} added a new photo to her album.}}\n\
                 .when 1 * {{{$userName} added a new photo to their album.}}\n\
                 .when * masculine {{{$userName} added {$photoCount} photos to his album.}}\n\
                 .when * feminine {{{$userName} added {$photoCount} photos to her album.}}\n\
                 .when * * {{{$userName} added {$photoCount} photos to their album.}}";
    testBuilder.setPattern(message);

    int64_t count = 1;
    TestCase test = testBuilder.clearArguments().setArgument("photoCount", count)
        .setArgument("userGender", "masculine")
        .setArgument("userName", "John")
        .setExpected("John added a new photo to his album.")
        .build();
    TestUtils::runTestCase(*this, test, errorCode);
    test = testBuilder.setArgument("userGender", "feminine")
                      .setArgument("userName", "Anna")
                      .setExpected("Anna added a new photo to her album.")
                      .build();
    TestUtils::runTestCase(*this, test, errorCode);
    test = testBuilder.setArgument("userGender", "unknown")
                      .setArgument("userName", "Anonymous")
                      .setExpected("Anonymous added a new photo to their album.")
                      .build();
    TestUtils::runTestCase(*this, test, errorCode);

    count = 13;
    test = testBuilder.clearArguments().setArgument("photoCount", count)
                                .setArgument("userGender", "masculine")
                                .setArgument("userName", "John")
                                .setExpected("John added 13 photos to his album.")
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);
    test = testBuilder.setArgument("userGender", "feminine")
                      .setArgument("userName", "Anna")
                      .setExpected("Anna added 13 photos to her album.")
                      .build();
    TestUtils::runTestCase(*this, test, errorCode);
    test = testBuilder.setArgument("userGender", "unknown")
                      .setArgument("userName", "Anonymous")
                      .setExpected("Anonymous added 13 photos to their album.")
                      .build();
    TestUtils::runTestCase(*this, test, errorCode);
}

void TestMessageFormat2::testSimpleLocalVariable(TestCase::Builder& testBuilder, IcuTestErrorCode& errorCode) {
    CHECK_ERROR(errorCode);

    LocalPointer<Calendar> cal(Calendar::createInstance(errorCode));
    CHECK_ERROR(errorCode);
    // November 23, 2022 at 7:42:37.123 PM
    cal->set(2022, Calendar::NOVEMBER, 23, 19, 42, 37);
    UDate TEST_DATE = cal->getTime(errorCode);
    CHECK_ERROR(errorCode);

    testBuilder.setPattern(".local $expDate = {$expDate :datetime skeleton=yMMMdE}\n\
                            {{Your tickets expire on {$expDate}.}}");

    int64_t count = 1;
    TestUtils::runTestCase(*this, testBuilder.clearArguments().setArgument("count", count)
                      .setLocale(Locale("en"))
                      .setDateArgument("expDate", TEST_DATE)
                      .setExpected("Your tickets expire on Wed, Nov 23, 2022.")
                      .build(), errorCode);
}

void TestMessageFormat2::testLocalVariableWithSelect(TestCase::Builder& testBuilder, IcuTestErrorCode& errorCode) {
    CHECK_ERROR(errorCode);

    LocalPointer<Calendar> cal(Calendar::createInstance(errorCode));
    CHECK_ERROR(errorCode);
    // November 23, 2022 at 7:42:37.123 PM
    cal->set(2022, Calendar::NOVEMBER, 23, 19, 42, 37);
    UDate TEST_DATE = cal->getTime(errorCode);
    CHECK_ERROR(errorCode);

    testBuilder.setPattern(".local $expDate = {$expDate :datetime skeleton=yMMMdE}\n\
                .match {$count :plural}\n\
                .when 1 {{Your ticket expires on {$expDate}.}}\n\
                .when * {{Your {$count} tickets expire on {$expDate}.}}\n");

    int64_t count = 1;
    TestCase test = testBuilder.clearArguments().setArgument("count", count)
                      .setLocale(Locale("en"))
                      .setDateArgument("expDate", TEST_DATE)
                      .setExpected("Your ticket expires on Wed, Nov 23, 2022.")
                      .build();
    TestUtils::runTestCase(*this, test, errorCode);
    count = 3;
    test = testBuilder.setArgument("count", count)
                      .setExpected("Your 3 tickets expire on Wed, Nov 23, 2022.")
                      .build();
    TestUtils::runTestCase(*this, test, errorCode);
}

void TestMessageFormat2::testDateFormat(TestCase::Builder& testBuilder, IcuTestErrorCode& errorCode) {
    LocalPointer<Calendar> cal(Calendar::createInstance(errorCode));
    CHECK_ERROR(errorCode);

    cal->set(2022, Calendar::OCTOBER, 27, 0, 0, 0);
    UDate expiration = cal->getTime(errorCode);
    CHECK_ERROR(errorCode);

    TestCase test = testBuilder.clearArguments().setPattern("Your card expires on {$exp :datetime skeleton=yMMMdE}!")
                                .setLocale(Locale("en"))
                                .setExpected("Your card expires on Thu, Oct 27, 2022!")
                                .setDateArgument("exp", expiration)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.clearArguments().setPattern("Your card expires on {$exp :datetime datestyle=full}!")
                      .setExpected("Your card expires on Thursday, October 27, 2022!")
                      .setDateArgument("exp", expiration)
                      .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.clearArguments().setPattern("Your card expires on {$exp :datetime datestyle=long}!")
                      .setExpected("Your card expires on October 27, 2022!")
                      .setDateArgument("exp", expiration)
                      .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.clearArguments().setPattern("Your card expires on {$exp :datetime datestyle=medium}!")
                      .setExpected("Your card expires on Oct 27, 2022!")
                      .setDateArgument("exp", expiration)
                      .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.clearArguments().setPattern("Your card expires on {$exp :datetime datestyle=short}!")
                      .setExpected("Your card expires on 10/27/22!")
                      .setDateArgument("exp", expiration)
                      .build();
    TestUtils::runTestCase(*this, test, errorCode);

/*
  This test would require the calendar to be passed as a UObject* with the datetime formatter
  doing an RTTI check -- however, that would be awkward, since it would have to check the tag for each
  possible subclass of `Calendar`. datetime currently has no support for formatting any object argument

    cal.adoptInstead(new GregorianCalendar(2022, Calendar::OCTOBER, 27, errorCode));
    if (cal.isValid()) {
        test = testBuilder.setPattern("Your card expires on {$exp :datetime skeleton=yMMMdE}!")
                          .setExpected("Your card expires on Thu, Oct 27, 2022!")
                          .setArgument("exp", cal.orphan(), errorCode)
                          .build();
        TestUtils::runTestCase(*this, test, errorCode);
    }
*/

    // Implied function based on type of the object to format
    test = testBuilder.clearArguments().setPattern("Your card expires on {$exp}!")
                      .setExpected(CharsToUnicodeString("Your card expires on 10/27/22, 12:00\\u202FAM!"))
                      .setDateArgument("exp", expiration)
                      .build();
    TestUtils::runTestCase(*this, test, errorCode);
}

void TestMessageFormat2::testPlural(TestCase::Builder& testBuilder, IcuTestErrorCode& errorCode) {
    UnicodeString message = ".match {$count :plural}\n\
                .when 1 {{You have one notification.}}\n           \
                .when * {{You have {$count} notifications.}}\n";

    int64_t count = 1;
    TestCase test = testBuilder.clearArguments().setPattern(message)
                                .setExpected("You have one notification.")
                                .setArgument("count", count)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    count = 42;
    test = testBuilder.clearArguments().setExpected("You have 42 notifications.")
                      .setArgument("count", count)
                      .build();
    TestUtils::runTestCase(*this, test, errorCode);

    count = 1;
    test = testBuilder.clearArguments().setPattern(message)
                      .setExpected("You have one notification.")
                      .setArgument("count", "1")
                      .build();
    TestUtils::runTestCase(*this, test, errorCode);

    count = 42;
    test = testBuilder.clearArguments().setExpected("You have 42 notifications.")
                      .setArgument("count", "42")
                      .build();
    TestUtils::runTestCase(*this, test, errorCode);
}

void TestMessageFormat2::testPluralOrdinal(TestCase::Builder& testBuilder, IcuTestErrorCode& errorCode) {
    UnicodeString message =  ".match {$place :selectordinal}\n\
                .when 1 {{You got the gold medal}}\n            \
                .when 2 {{You got the silver medal}}\n          \
                .when 3 {{You got the bronze medal}}\n\
                .when one {{You got in the {$place}st place}}\n\
                .when two {{You got in the {$place}nd place}}\n \
                .when few {{You got in the {$place}rd place}}\n \
                .when * {{You got in the {$place}th place}}\n";

    TestCase test = testBuilder.clearArguments().setPattern(message)
                                .setExpected("You got the gold medal")
                                .setArgument("place", "1")
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.clearArguments().setExpected("You got the silver medal")
                          .setArgument("place", "2")
                          .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.clearArguments().setExpected("You got the bronze medal")
                          .setArgument("place", "3")
                          .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.clearArguments().setExpected("You got in the 21st place")
                          .setArgument("place", "21")
                          .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.clearArguments().setExpected("You got in the 32nd place")
                          .setArgument("place", "32")
                          .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.clearArguments().setExpected("You got in the 23rd place")
                          .setArgument("place", "23")
                          .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.clearArguments().setExpected("You got in the 15th place")
                          .setArgument("place", "15")
                          .build();
    TestUtils::runTestCase(*this, test, errorCode);
}

TemperatureFormatter::TemperatureFormatter(const Locale& l, TemperatureFormatterFactory& c, UErrorCode& errorCode) : locale(l), counter(c) {
    CHECK_ERROR(errorCode);

    cachedFormatters = new Hashtable(uhash_compareUnicodeString, nullptr, errorCode);
    if (cachedFormatters == nullptr) {
        errorCode = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
    cachedFormatters->setValueDeleter(uprv_free);
    counter.constructCount++;
}

Formatter* TemperatureFormatterFactory::createFormatter(const Locale& locale, UErrorCode& errorCode) {
    NULL_ON_ERROR(errorCode);

    LocalPointer<Formatter> result(new TemperatureFormatter(locale, *this, errorCode));
    NULL_ON_ERROR(errorCode)
    return result.orphan();
}

message2::FormattedPlaceholder TemperatureFormatter::format(FormattedPlaceholder&& arg, FunctionOptions&& options, UErrorCode& errorCode) const {
    if (U_FAILURE(errorCode)) {
        return {};
    }

    message2::FormattedPlaceholder errorVal("temp");

    // Argument must be present
    if (!arg.canFormat()) {
        errorCode = U_FORMATTING_ERROR;
        return errorVal;
    }
    // Assume arg is not-yet-formatted
    const Formattable& toFormat = arg.asFormattable();

    counter.formatCount++;

    FunctionOptionsMap opt = options.getOptions();
    bool unitExists = opt.count("unit") > 0 && opt["unit"].getType() == UFMT_STRING;
    if (!unitExists) {
        errorCode = U_FORMATTING_ERROR;
        return errorVal;
    }
    UnicodeString unit = opt["unit"].getString(errorCode);
    U_ASSERT(U_SUCCESS(errorCode));
    bool skeletonExists = opt.count("skeleton") > 0 && opt["skeleton"].getType() == UFMT_STRING;

    number::LocalizedNumberFormatter* realNfCached = (number::LocalizedNumberFormatter*) cachedFormatters->get(unit);
    number::LocalizedNumberFormatter realNf;
    if (realNfCached == nullptr) {
        number::LocalizedNumberFormatter nf;
        if (skeletonExists) {
            const UnicodeString& s = opt["skeleton"].getString(errorCode);
            U_ASSERT(U_SUCCESS(errorCode));
            nf = number::NumberFormatter::forSkeleton(s, errorCode).locale(locale);
        } else {
            nf = number::NumberFormatter::withLocale(locale);
        }
        if (unit == "C") {
            counter.cFormatterCount++;
            realNf = nf.unit(MeasureUnit::getCelsius());
        } else if (unit == "F") {
            counter.fFormatterCount++;
            realNf = nf.unit(MeasureUnit::getFahrenheit());
        } else {
            realNf = nf;
        }
        realNfCached = new number::LocalizedNumberFormatter(realNf);
        if (realNfCached == nullptr) {
            errorCode = U_MEMORY_ALLOCATION_ERROR;
            return errorVal;
        }
        cachedFormatters->put(unit, realNfCached, errorCode);
    } else {
        realNf = *realNfCached;
    }

    number::FormattedNumber result;
    switch (toFormat.getType()) {
        case UFMT_DOUBLE: {
            double d = toFormat.getDouble(errorCode);
            U_ASSERT(U_SUCCESS(errorCode));
            result = realNf.formatDouble(d, errorCode);
            break;
        }
        case UFMT_LONG: {
            long l = toFormat.getLong(errorCode);
            U_ASSERT(U_SUCCESS(errorCode));
            result = realNf.formatInt(l, errorCode);
            break;
        }
        case UFMT_INT64: {
            int64_t i = toFormat.getInt64(errorCode);
            result = realNf.formatInt(i, errorCode);
            break;
        }
        default: {
            return message2::FormattedPlaceholder(arg, FormattedValue(UnicodeString()));
        }
    }
    return message2::FormattedPlaceholder(arg, FormattedValue(std::move(result)));
}

TemperatureFormatter::~TemperatureFormatter() { delete cachedFormatters; }
TemperatureFormatterFactory::~TemperatureFormatterFactory() {}

void TestMessageFormat2::testFormatterIsCreatedOnce(IcuTestErrorCode& errorCode) {
    using Formattable = message2::Formattable;

    FunctionRegistry::Builder frBuilder(errorCode);
    CHECK_ERROR(errorCode);

    LocalPointer<TemperatureFormatterFactory> counter(new TemperatureFormatterFactory());
    if (!counter.isValid()) {
        ((UErrorCode&) errorCode) = U_MEMORY_ALLOCATION_ERROR;
        return;
    }

    UnicodeString message = "Testing {$count :temp unit=$unit skeleton=|.00/w|}.";

    MessageFormatter::Builder mfBuilder(errorCode);
    FunctionRegistry reg = frBuilder.setFormatter(FunctionName("temp"), counter.getAlias(), errorCode)
        .build();
    CHECK_ERROR(errorCode);
    UParseError parseError;
    mfBuilder.setPattern(message, parseError, errorCode).setFunctionRegistry(reg);
    MessageFormatter mf = mfBuilder.build(errorCode);
    UnicodeString result;
    UnicodeString countKey("count");
    UnicodeString unitKey("unit");

    const int64_t maxCount = 10;
    char expected[20];

    std::map<UnicodeString, Formattable> argumentsBuilder;

    for (int64_t count = 0; count < maxCount; count++) {

        snprintf(expected, sizeof(expected), "Testing %d\\u00B0C.", (int32_t) count);

        argumentsBuilder[countKey] = Formattable(count);
        argumentsBuilder[unitKey] = Formattable("C");

        result = mf.formatToString(MessageArguments(std::move(argumentsBuilder), errorCode), errorCode);
        assertEquals("temperature formatter", CharsToUnicodeString(expected), result);

        snprintf(expected, sizeof(expected), "Testing %d\\u00B0F.", (int32_t) count);

        argumentsBuilder[countKey] = Formattable(count);
        argumentsBuilder[unitKey] = Formattable("F");

        result = mf.formatToString(MessageArguments(std::move(argumentsBuilder), errorCode), errorCode);
        assertEquals("temperature formatter", CharsToUnicodeString(expected), result);
    }

    assertEquals("cached formatter", 1, counter->constructCount);
    assertEquals("cached formatter", (int64_t) maxCount * 2, (int64_t) counter->formatCount);
    assertEquals("cached formatter", 1, counter->fFormatterCount);
    assertEquals("cached formatter", 1, counter->cFormatterCount);

    argumentsBuilder[countKey] = Formattable(12.0);
    argumentsBuilder[unitKey] = Formattable("C");
    result = mf.formatToString(MessageArguments(std::move(argumentsBuilder), errorCode), errorCode);

    assertEquals("cached formatter", CharsToUnicodeString("Testing 12\\u00B0C."), result);

    argumentsBuilder[countKey] = Formattable((double) 12.5);
    argumentsBuilder[unitKey] = Formattable("F");
    result = mf.formatToString(MessageArguments(std::move(argumentsBuilder), errorCode), errorCode);
    assertEquals("cached formatter", CharsToUnicodeString("Testing 12.50\\u00B0F."), result);

    argumentsBuilder[countKey] = Formattable((double) 12.54);
    argumentsBuilder[unitKey] = Formattable("C");
    result = mf.formatToString(MessageArguments(std::move(argumentsBuilder), errorCode), errorCode);
    assertEquals("cached formatter", CharsToUnicodeString("Testing 12.54\\u00B0C."), result);

    argumentsBuilder[countKey] = Formattable((double) 12.54321);
    argumentsBuilder[unitKey] = Formattable("F");
    result = mf.formatToString(MessageArguments(std::move(argumentsBuilder), errorCode), errorCode);
    assertEquals("cached formatter", CharsToUnicodeString("Testing 12.54\\u00B0F."), result);

    // Check skeleton
    message = "Testing {$count :temp unit=$unit skeleton=|.0/w|}.";
    mfBuilder.setPattern(message, parseError, errorCode);
    mf = mfBuilder.build(errorCode);

    argumentsBuilder[countKey] = Formattable(12.0);
    argumentsBuilder[unitKey] = Formattable("C");
    result = mf.formatToString(MessageArguments(std::move(argumentsBuilder), errorCode), errorCode);
    assertEquals("cached formatter", CharsToUnicodeString("Testing 12\\u00B0C."), result);

    argumentsBuilder[countKey] = Formattable((double) 12.5);
    argumentsBuilder[unitKey] = Formattable("F");
    result = mf.formatToString(MessageArguments(std::move(argumentsBuilder), errorCode), errorCode);
    assertEquals("cached formatter", CharsToUnicodeString("Testing 12.5\\u00B0F."), result);

    argumentsBuilder[countKey] = Formattable((double) 12.54);
    argumentsBuilder[unitKey] = Formattable("C");
    result = mf.formatToString(MessageArguments(std::move(argumentsBuilder), errorCode), errorCode);
    assertEquals("cached formatter", CharsToUnicodeString("Testing 12.5\\u00B0C."), result);

    argumentsBuilder[countKey] = Formattable((double) 12.54321);
    argumentsBuilder[unitKey] = Formattable("F");
    result = mf.formatToString(MessageArguments(std::move(argumentsBuilder), errorCode), errorCode);
    assertEquals("cached formatter", CharsToUnicodeString("Testing 12.5\\u00B0F."), result);
}

void TestMessageFormat2::testPluralWithOffset(TestCase::Builder& testBuilder, IcuTestErrorCode& errorCode) {
    UnicodeString message = ".match {$count :plural offset=2}\n\
                  .when 1 {{Anna}}\n\
                  .when 2 {{Anna and Bob}}\n\
                  .when one {{Anna, Bob, and {$count :number offset=2} other guest}}\n\
                  .when * {{Anna, Bob, and {$count :number offset=2} other guests}}\n";

    testBuilder.setPattern(message);
    testBuilder.setName("plural with offset");

    TestCase test = testBuilder.setExpected("Anna")
                                .setArgument("count", (int64_t) 1)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.clearArguments().setExpected("Anna and Bob")
                          .setArgument("count", (int64_t) 2)
                          .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.clearArguments().setExpected("Anna, Bob, and 1 other guest")
                          .setArgument("count", (int64_t) 3)
                          .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.clearArguments().setExpected("Anna, Bob, and 2 other guests")
                          .setArgument("count", (int64_t) 4)
                          .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.clearArguments().setExpected("Anna, Bob, and 10 other guests")
                          .setArgument("count", (int64_t) 12)
                          .build();
    TestUtils::runTestCase(*this, test, errorCode);
}

void TestMessageFormat2::testPluralWithOffsetAndLocalVar(TestCase::Builder& testBuilder, IcuTestErrorCode& errorCode) {

    // $foo should "inherit" the offset
    UnicodeString message = ".local $foo = {$count :number offset=2}\
                .match {$foo :plural}\n                                 \
                .when 1 {{Anna}}\n                                        \
                .when 2 {{Anna and Bob}}\n                                \
                .when one {{Anna, Bob, and {$foo} other guest}}\n         \
                .when * {{Anna, Bob, and {$foo} other guests}}\n";

    testBuilder.clearArguments().setPattern(message);
    testBuilder.setName("plural with offset and local var");

    TestCase test = testBuilder.setExpected("Anna")
                                .setArgument("count", (int64_t) 1)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setExpected("Anna and Bob")
                          .setArgument("count", (int64_t) 2)
                          .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setExpected("Anna, Bob, and 1 other guest")
                          .setArgument("count", (int64_t) 3)
                          .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setExpected("Anna, Bob, and 2 other guests")
                          .setArgument("count", (int64_t) 4)
                          .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setExpected("Anna, Bob, and 10 other guests")
                          .setArgument("count", (int64_t) 12)
                          .build();
    TestUtils::runTestCase(*this, test, errorCode);

    message = ".local $foo = {$amount :number skeleton=|.00/w|}\n\
                .match {$foo :plural}\n\
                .when 1 {{Last dollar}}\n\
                .when one {{{$foo} dollar}}\n\
                .when * {{{$foo} dollars}}\n";
    testBuilder.setPattern(message);
    test = testBuilder.setExpected("Last dollar")
                          .setArgument("amount", (int64_t) 1)
                          .build();
    TestUtils::runTestCase(*this, test, errorCode);
    test = testBuilder.setExpected("2 dollars")
                          .setArgument("amount", (int64_t) 2)
                          .build();
    TestUtils::runTestCase(*this, test, errorCode);
    test = testBuilder.setExpected("3 dollars")
                          .setArgument("amount", (int64_t) 3)
                          .build();
    TestUtils::runTestCase(*this, test, errorCode);
}

void TestMessageFormat2::testDeclareBeforeUse(TestCase::Builder& testBuilder, IcuTestErrorCode& errorCode) {

    UnicodeString message = ".local $foo = {$baz :number}\n\
                 .local $bar = {$foo}\n                    \
                 .local $baz = {$bar}\n                    \
                 {{The message uses {$baz} and works}}";
    testBuilder.setPattern(message);
    testBuilder.setName("declare-before-use");

    TestCase test = testBuilder.clearArguments().setExpected("The message uses {$baz} and works")
                                .setExpectedError(U_UNRESOLVED_VARIABLE_ERROR)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);
}

void TestMessageFormat2::testVariableOptionsInSelector(TestCase::Builder& testBuilder, IcuTestErrorCode& errorCode) {
    UnicodeString message = ".match {$count :plural offset=$delta}\n\
                .when 1 {{A}}\n\
                .when 2 {{A and B}}\n\
                .when one {{A, B, and {$count :number offset=$delta} more character}}\n\
                .when * {{A, B, and {$count :number offset=$delta} more characters}}\n";

    testBuilder.setPattern(message);
    testBuilder.setName("variable options in selector");
    testBuilder.setExpectSuccess();

    TestCase test = testBuilder.clearArguments().setExpected("A")
                                .setArgument("count", (int64_t) 1)
                                .setArgument("delta", (int64_t) 2)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);
    test = testBuilder.clearArguments().setExpected("A and B")
                                .setArgument("count", (int64_t) 2)
                                .setArgument("delta", (int64_t) 2)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);
    test = testBuilder.clearArguments().setExpected("A, B, and 1 more character")
                                .setArgument("count", (int64_t) 3)
                                .setArgument("delta", (int64_t) 2)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);
    test = testBuilder.clearArguments().setExpected("A, B, and 5 more characters")
                                .setArgument("count", (int64_t) 7)
                                .setArgument("delta", (int64_t) 2)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    message = ".match {$count :plural offset=$delta}\n\
                  .when 1 {{Exactly 1}}\n\
                  .when 2 {{Exactly 2}}\n\
                  .when * {{Count = {$count :number offset=$delta} and delta={$delta}.}}\n";
    testBuilder.setPattern(message);

    test = testBuilder.clearArguments().setExpected("Exactly 1")
                                .setArgument("count", (int64_t) 1)
                                .setArgument("delta", (int64_t) 0)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);
    test = testBuilder.clearArguments().setExpected("Exactly 1")
                                .setArgument("count", (int64_t) 1)
                                .setArgument("delta", (int64_t) 1)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);
    test = testBuilder.clearArguments().setExpected("Exactly 1")
                                .setArgument("count", (int64_t) 1)
                                .setArgument("delta", (int64_t) 2)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);
    test = testBuilder.clearArguments().setExpected("Exactly 2")
                                .setArgument("count", (int64_t) 2)
                                .setArgument("delta", (int64_t) 0)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);
    test = testBuilder.clearArguments().setExpected("Exactly 2")
                                .setArgument("count", (int64_t) 2)
                                .setArgument("delta", (int64_t) 1)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);
    test = testBuilder.clearArguments().setExpected("Exactly 2")
                                .setArgument("count", (int64_t) 2)
                                .setArgument("delta", (int64_t) 2)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);
    test = testBuilder.clearArguments().setExpected("Count = 3 and delta=0.")
                                .setArgument("count", (int64_t) 3)
                                .setArgument("delta", (int64_t) 0)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);
    test = testBuilder.clearArguments().setExpected("Count = 2 and delta=1.")
                                .setArgument("count", (int64_t) 3)
                                .setArgument("delta", (int64_t) 1)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);
    test = testBuilder.clearArguments().setExpected("Count = 1 and delta=2.")
                                .setArgument("count", (int64_t) 3)
                                .setArgument("delta", (int64_t) 2)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);
    test = testBuilder.clearArguments().setExpected("Count = 23 and delta=0.")
                                .setArgument("count", (int64_t) 23)
                                .setArgument("delta", (int64_t) 0)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);
    test = testBuilder.clearArguments().setExpected("Count = 22 and delta=1.")
                                .setArgument("count", (int64_t) 23)
                                .setArgument("delta", (int64_t) 1)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);
    test = testBuilder.clearArguments().setExpected("Count = 21 and delta=2.")
                                .setArgument("count", (int64_t) 23)
                                .setArgument("delta", (int64_t) 2)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);
}

void TestMessageFormat2::testVariableOptionsInSelectorWithLocalVar(TestCase::Builder& testBuilder, IcuTestErrorCode& errorCode) {
    CHECK_ERROR(errorCode);

    UnicodeString messageFix = ".local $offCount = {$count :number offset=2}\n\
                .match {$offCount :plural}\n\
                .when 1 {{A}}\n\
                .when 2 {{A and B}}\n\
                .when one {{A, B, and {$offCount} more character}}\n\
                .when * {{A, B, and {$offCount} more characters}}\n";

    testBuilder.setPattern(messageFix);
    testBuilder.setName("variable options in selector with local var");
    testBuilder.setExpectSuccess();

    TestCase test = testBuilder.clearArguments().setExpected("A")
                                .setArgument("count", (int64_t) 1)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);
    test = testBuilder.clearArguments().setExpected("A and B")
                                .setArgument("count", (int64_t) 2)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);
    test = testBuilder.clearArguments().setExpected("A, B, and 1 more character")
                                .setArgument("count", (int64_t) 3)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);
    test = testBuilder.clearArguments().setExpected("A, B, and 5 more characters")
                                .setArgument("count", (int64_t) 7)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    UnicodeString messageVar = ".local $offCount = {$count :number offset=$delta}\n\
                .match {$offCount :plural}\n\
                .when 1 {{A}}\n\
                .when 2 {{A and B}}\n\
                .when one {{A, B, and {$offCount} more character}}\n\
                .when * {{A, B, and {$offCount} more characters}}\n";
    testBuilder.setPattern(messageVar);

    test = testBuilder.clearArguments().setExpected("A")
                                .setArgument("count", (int64_t) 1)
                                .setArgument("delta", (int64_t) 2)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);
    test = testBuilder.clearArguments().setExpected("A and B")
                                .setArgument("count", (int64_t) 2)
                                .setArgument("delta", (int64_t) 2)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);
    test = testBuilder.clearArguments().setExpected("A, B, and 1 more character")
                                .setArgument("count", (int64_t) 3)
                                .setArgument("delta", (int64_t) 2)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);
    test = testBuilder.clearArguments().setExpected("A, B, and 5 more characters")
                                .setArgument("count", (int64_t) 7)
                                .setArgument("delta", (int64_t) 2)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    UnicodeString messageVar2 = ".local $offCount = {$count :number offset=$delta}\n\
                .match {$offCount :plural}\n\
                .when 1 {{Exactly 1}}\n\
                .when 2 {{Exactly 2}}\n\
                .when * {{Count = {$count}, OffCount = {$offCount}, and delta={$delta}.}}\n";
    testBuilder.setPattern(messageVar2);
    test = testBuilder.clearArguments().setExpected("Exactly 1")
                                .setArgument("count", (int64_t) 1)
                                .setArgument("delta", (int64_t) 0)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);
    test = testBuilder.clearArguments().setExpected("Exactly 1")
                                .setArgument("count", (int64_t) 1)
                                .setArgument("delta", (int64_t) 1)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);
    test = testBuilder.clearArguments().setExpected("Exactly 1")
                                .setArgument("count", (int64_t) 1)
                                .setArgument("delta", (int64_t) 2)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.clearArguments().setExpected("Exactly 2")
                                .setArgument("count", (int64_t) 2)
                                .setArgument("delta", (int64_t) 0)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);
    test = testBuilder.clearArguments().setExpected("Exactly 2")
                                .setArgument("count", (int64_t) 2)
                                .setArgument("delta", (int64_t) 1)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);
    test = testBuilder.clearArguments().setExpected("Exactly 2")
                                .setArgument("count", (int64_t) 2)
                                .setArgument("delta", (int64_t) 2)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);
    test = testBuilder.clearArguments().setExpected("Count = 3, OffCount = 3, and delta=0.")
                                .setArgument("count", (int64_t) 3)
                                .setArgument("delta", (int64_t) 0)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);
    test = testBuilder.clearArguments().setExpected("Count = 3, OffCount = 2, and delta=1.")
                                .setArgument("count", (int64_t) 3)
                                .setArgument("delta", (int64_t) 1)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);
    test = testBuilder.clearArguments().setExpected("Count = 3, OffCount = 1, and delta=2.")
                                .setArgument("count", (int64_t) 3)
                                .setArgument("delta", (int64_t) 2)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.clearArguments().setExpected("Count = 23, OffCount = 23, and delta=0.")
                                .setArgument("count", (int64_t) 23)
                                .setArgument("delta", (int64_t) 0)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);
    test = testBuilder.clearArguments().setExpected("Count = 23, OffCount = 22, and delta=1.")
                                .setArgument("count", (int64_t) 23)
                                .setArgument("delta", (int64_t) 1)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);
    test = testBuilder.clearArguments().setExpected("Count = 23, OffCount = 21, and delta=2.")
                                .setArgument("count", (int64_t) 23)
                                .setArgument("delta", (int64_t) 2)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);
}


void TestMessageFormat2::featureTests() {
    IcuTestErrorCode errorCode(*this, "featureTests");

    TestCase::Builder testBuilder;
    testBuilder.setName("featureTests");

    testEmptyMessage(testBuilder, errorCode);
    testPlainText(testBuilder, errorCode);
    testPlaceholders(testBuilder, errorCode);
    testArgumentMissing(testBuilder, errorCode);
    testDefaultLocale(testBuilder, errorCode);
    testSpecialPluralWithDecimals(testBuilder, errorCode);
    testDefaultFunctionAndOptions(testBuilder, errorCode);
    testSimpleSelection(testBuilder, errorCode);
    testComplexSelection(testBuilder, errorCode);
    testSimpleLocalVariable(testBuilder, errorCode);
    testLocalVariableWithSelect(testBuilder, errorCode);

    testDateFormat(testBuilder, errorCode);
    testPlural(testBuilder, errorCode);
    testPluralOrdinal(testBuilder, errorCode);
    testFormatterIsCreatedOnce(errorCode);
    testPluralWithOffset(testBuilder, errorCode);
    testPluralWithOffsetAndLocalVar(testBuilder, errorCode);
    testDeclareBeforeUse(testBuilder, errorCode);
    testVariableOptionsInSelector(testBuilder, errorCode);
    testVariableOptionsInSelectorWithLocalVar(testBuilder, errorCode);
}

TestCase::~TestCase() {}
TestCase::Builder::~Builder() {}

#endif /* #if !UCONFIG_NO_FORMATTING */
