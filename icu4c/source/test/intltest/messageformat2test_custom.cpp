// © 2016 and later: Unicode, Inc. and others.

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "plurrule_impl.h"

#include "unicode/listformatter.h"
#include "messageformat2test.h"
#include "hash.h"
#include "intltest.h"


using namespace message2;
using namespace pluralimpl;

/*
Tests reflect the syntax specified in

  https://github.com/unicode-org/message-format-wg/commits/main/spec/message.abnf

as of the following commit from 2023-05-09:
  https://github.com/unicode-org/message-format-wg/commit/194f6efcec5bf396df36a19bd6fa78d1fa2e0867
*/

using namespace data_model;

static FunctionRegistry personFunctionRegistry(UErrorCode& status) {
    return FunctionRegistry::Builder(status)
        .setFormatter(FunctionName("person"), new PersonNameFormatterFactory(), status)
        .build();
}

void TestMessageFormat2::testPersonFormatter(IcuTestErrorCode& errorCode) {
    CHECK_ERROR(errorCode);

    FunctionRegistry customRegistry(personFunctionRegistry(errorCode));
    UnicodeString name = "name";
    LocalPointer<Person> person(new Person(UnicodeString("Mr."), UnicodeString("John"), UnicodeString("Doe")));
    TestCase::Builder testBuilder;
    testBuilder.setName("testPersonFormatter");
    testBuilder.setLocale(Locale("en"));

    TestCase test = testBuilder.setPattern("{Hello {$name :person formality=formal}}")
        .setArgument(name, person.getAlias())
        .setExpected("Hello {$name}")
        .setExpectedError(U_UNKNOWN_FUNCTION_ERROR)
        .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern("{Hello {$name :person formality=informal}}")
                                .setArgument(name, person.getAlias())
                                .setExpected("Hello {$name}")
                                .setExpectedError(U_UNKNOWN_FUNCTION_ERROR)
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    testBuilder.setFunctionRegistry(std::make_shared<FunctionRegistry>(std::move(customRegistry)));

    test = testBuilder.setPattern("{Hello {$name :person formality=formal}}")
                                .setArgument(name, person.getAlias())
                                .setExpected("Hello Mr. Doe")
                                .setExpectSuccess()
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern("{Hello {$name :person formality=informal}}")
                                .setArgument(name, person.getAlias())
                                .setExpected("Hello John")
                                .setExpectSuccess()
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern("{Hello {$name :person formality=formal length=long}}")
                                .setArgument(name, person.getAlias())
                                .setExpected("Hello Mr. John Doe")
                                .setExpectSuccess()
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern("{Hello {$name :person formality=formal length=medium}}")
                                .setArgument(name, person.getAlias())
                                .setExpected("Hello John Doe")
                                .setExpectSuccess()
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setPattern("{Hello {$name :person formality=formal length=short}}")
                                .setArgument(name, person.getAlias())
                                .setExpected("Hello Mr. Doe")
                                .setExpectSuccess()
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);
}

void TestMessageFormat2::testCustomFunctionsComplexMessage(IcuTestErrorCode& errorCode) {
    CHECK_ERROR(errorCode);

    FunctionRegistry customRegistry = personFunctionRegistry(errorCode);
    UnicodeString host = "host";
    UnicodeString hostGender = "hostGender";
    UnicodeString guest = "guest";
    UnicodeString guestCount = "guestCount";

    LocalPointer<Person> jane(new Person(UnicodeString("Ms."), UnicodeString("Jane"), UnicodeString("Doe")));
    LocalPointer<Person> john(new Person(UnicodeString("Mr."), UnicodeString("John"), UnicodeString("Doe")));
    LocalPointer<Person> anonymous(new Person(UnicodeString("Mx."), UnicodeString("Anonymous"), UnicodeString("Doe")));

    if (!jane.isValid() || !john.isValid() || !anonymous.isValid()) {
       ((UErrorCode&) errorCode) = U_MEMORY_ALLOCATION_ERROR;
       return;
   }

    UnicodeString message = "let $hostName = {$host :person length=long}\n\
                let $guestName = {$guest :person length=long}\n\
                let $guestsOther = {$guestCount :number offset=1}\n\
                match {$hostGender :gender} {$guestCount :plural}\n\
                when female 0 {{$hostName} does not give a party.}\n\
                when female 1 {{$hostName} invites {$guestName} to her party.}\n\
                when female 2 {{$hostName} invites {$guestName} and one other person to her party.}\n\
                when female * {{$hostName} invites {$guestName} and {$guestsOther} other people to her party.}\n\
                when male 0 {{$hostName} does not give a party.}\n\
                when male 1 {{$hostName} invites {$guestName} to his party.}\n\
                when male 2 {{$hostName} invites {$guestName} and one other person to his party.}\n\
                when male * {{$hostName} invites {$guestName} and {$guestsOther} other people to his party.}\n\
                when * 0 {{$hostName} does not give a party.}\n\
                when * 1 {{$hostName} invites {$guestName} to their party.}\n\
                when * 2 {{$hostName} invites {$guestName} and one other person to their party.}\n\
                when * * {{$hostName} invites {$guestName} and {$guestsOther} other people to their party.}\n";


    TestCase::Builder testBuilder;
    testBuilder.setName("testCustomFunctionsComplexMessage");
    testBuilder.setLocale(Locale("en"));
    testBuilder.setPattern(message);
    testBuilder.setFunctionRegistry(std::make_shared<FunctionRegistry>(std::move(customRegistry)));

    TestCase test = testBuilder.setArgument(host, jane.getAlias())
        .setArgument(hostGender, "female")
        .setArgument(guest, john.getAlias())
        .setArgument(guestCount, (int64_t) 3)
        .setExpected("Ms. Jane Doe invites Mr. John Doe and 2 other people to her party.")
        .setExpectSuccess()
        .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setArgument(host, jane.getAlias())
                                .setArgument(hostGender, "female")
                                .setArgument(guest, john.getAlias())
                                .setArgument(guestCount, (int64_t) 2)
                                .setExpected("Ms. Jane Doe invites Mr. John Doe and one other person to her party.")
                                .setExpectSuccess()
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setArgument(host, jane.getAlias())
                                .setArgument(hostGender, "female")
                                .setArgument(guest, john.getAlias())
                                .setArgument(guestCount, (int64_t) 1)
                                .setExpected("Ms. Jane Doe invites Mr. John Doe to her party.")
                                .setExpectSuccess()
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setArgument(host, john.getAlias())
                                .setArgument(hostGender, "male")
                                .setArgument(guest, jane.getAlias())
                                .setArgument(guestCount, (int64_t) 3)
                                .setExpected("Mr. John Doe invites Ms. Jane Doe and 2 other people to his party.")
                                .setExpectSuccess()
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setArgument(host, anonymous.getAlias())
                                .setArgument(hostGender, "unknown")
                                .setArgument(guest, jane.getAlias())
                                .setArgument(guestCount, (int64_t) 2)
                                .setExpected("Mx. Anonymous Doe invites Ms. Jane Doe and one other person to their party.")
                                .setExpectSuccess()
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);
}

void TestMessageFormat2::testCustomFunctions() {
  IcuTestErrorCode errorCode(*this, "testCustomFunctions");

  testPersonFormatter(errorCode);
  testCustomFunctionsComplexMessage(errorCode);
  testGrammarCasesFormatter(errorCode);
  testListFormatter(errorCode);
  testMessageRefFormatter(errorCode);
}


// -------------- Custom function implementations

Formatter* PersonNameFormatterFactory::createFormatter(const Locale& locale, UErrorCode& errorCode) {
    if (U_FAILURE(errorCode)) {
        return nullptr;
    }

    // Locale not used
    (void) locale;

    Formatter* result = new PersonNameFormatter();
    if (result == nullptr) {
        errorCode = U_MEMORY_ALLOCATION_ERROR;
    }
    return result;
}

message2::FormattedPlaceholder PersonNameFormatter::format(FormattedPlaceholder&& arg, FunctionOptions&& options, UErrorCode& errorCode) const {
    if (U_FAILURE(errorCode)) {
        return {};
    }

    message2::FormattedPlaceholder errorVal = message2::FormattedPlaceholder("not a person");

    if (!arg.canFormat() || arg.asFormattable().getType() != Formattable::Type::kObject) {
        return errorVal;
    }
    const Formattable& toFormat = arg.asFormattable();

    FunctionOptionsMap opt = options.getOptions();
    bool hasFormality = opt.count("formality") > 0 && opt["formality"].getType() == Formattable::Type::kString;
    bool hasLength = opt.count("length") > 0 && opt["length"].getType() == Formattable::Type::kString;

    bool useFormal = hasFormality && opt["formality"].getString() == "formal";
    UnicodeString length = hasLength ? opt["length"].getString() : "short";

    const FormattableObject* fp = toFormat.getObject();
    if (fp == nullptr || fp->tag() != u"person") {
        return errorVal;
    }
    const Person* p = static_cast<const Person*>(fp);

    UnicodeString title = p->title;
    UnicodeString firstName = p->firstName;
    UnicodeString lastName = p->lastName;

    UnicodeString result;
    if (length == "long") {
        result += title;
        result += " ";
        result += firstName;
        result += " ";
        result += lastName;
    } else if (length == "medium") {
        if (useFormal) {
            result += firstName;
            result += " ";
            result += lastName;
        } else {
            result += title;
            result += " ";
            result += firstName;
        }
    } else if (useFormal) {
        // Default to "short" length
        result += title;
        result += " ";
        result += lastName;
    } else {
        result += firstName;
    }

    return FormattedPlaceholder(arg, FormattedValue(std::move(result)));
}

FormattableProperties::~FormattableProperties() {}
Person::~Person() {}

/*
  See ICU4J: CustomFormatterGrammarCaseTest.java
*/
Formatter* GrammarCasesFormatterFactory::createFormatter(const Locale& locale, UErrorCode& errorCode) {
    if (U_FAILURE(errorCode)) {
        return nullptr;
    }

    // Locale not used
    (void) locale;

    Formatter* result = new GrammarCasesFormatter();
    if (result == nullptr) {
        errorCode = U_MEMORY_ALLOCATION_ERROR;
    }
    return result;
}


/* static */ void GrammarCasesFormatter::getDativeAndGenitive(const UnicodeString& value, UnicodeString& result) const {
    UnicodeString postfix;
    if (value.endsWith("ana")) {
        value.extract(0,  value.length() - 3, postfix);
        postfix += "nei";
    }
    else if (value.endsWith("ca")) {
        value.extract(0, value.length() - 2, postfix);
        postfix += "căi";
    }
    else if (value.endsWith("ga")) {
        value.extract(0, value.length() - 2, postfix);
        postfix += "găi";
    }
    else if (value.endsWith("a")) {
        value.extract(0, value.length() - 1, postfix);
        postfix += "ei";
    }
    else {
        postfix = "lui " + value;
    }
    result += postfix;
}

message2::FormattedPlaceholder GrammarCasesFormatter::format(FormattedPlaceholder&& arg, FunctionOptions&& options, UErrorCode& errorCode) const {
    if (U_FAILURE(errorCode)) {
        return {};
    }

    // Argument must be present
    if (!arg.canFormat()) {
        errorCode = U_FORMATTING_ERROR;
        return message2::FormattedPlaceholder("grammarBB");
    }

    // Assumes the argument is not-yet-formatted
    const Formattable& toFormat = arg.asFormattable();
    UnicodeString result;

    FunctionOptionsMap opt = options.getOptions();
    switch (toFormat.getType()) {
        case Formattable::Type::kString: {
            const UnicodeString& in = toFormat.getString();
            bool hasCase = opt.count("case") > 0;
            bool caseIsString = opt["case"].getType() == Formattable::Type::kString;
            if (hasCase && caseIsString && (opt["case"].getString() == "dative" || opt["case"].getString() == "genitive")) {
                getDativeAndGenitive(in, result);
            } else {
                result += in;
            }
            break;
        }
        default: {
            result += toFormat.getString();
            break;
        }
    }

    return message2::FormattedPlaceholder(arg, FormattedValue(std::move(result)));
}

/* static */ FunctionRegistry GrammarCasesFormatter::customRegistry(UErrorCode& errorCode) {
    return FunctionRegistry::Builder(errorCode)
      .setFormatter(FunctionName("grammarBB"), new GrammarCasesFormatterFactory(), errorCode)
      .build();
}

void TestMessageFormat2::testGrammarCasesFormatter(IcuTestErrorCode& errorCode) {
    CHECK_ERROR(errorCode);

    FunctionRegistry customRegistry = GrammarCasesFormatter::customRegistry(errorCode);
    TestCase::Builder testBuilder;
    testBuilder.setName("testGrammarCasesFormatter - genitive");
    testBuilder.setFunctionRegistry(std::make_shared<FunctionRegistry>(std::move(customRegistry)));
    testBuilder.setLocale(Locale("ro"));
    testBuilder.setPattern("{Cartea {$owner :grammarBB case=genitive}}");
    TestCase test = testBuilder.setArgument("owner", "Maria")
                                .setExpected("Cartea Mariei")
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setArgument("owner", "Rodica")
                                .setExpected("Cartea Rodicăi")
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setArgument("owner", "Ileana")
                                .setExpected("Cartea Ilenei")
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setArgument("owner", "Petre")
                                .setExpected("Cartea lui Petre")
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    testBuilder.setName("testGrammarCasesFormatter - nominative");
    testBuilder.setPattern("{M-a sunat {$owner :grammarBB case=nominative}}");

    test = testBuilder.setArgument("owner", "Maria")
                                .setExpected("M-a sunat Maria")
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setArgument("owner", "Rodica")
                                .setExpected("M-a sunat Rodica")
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setArgument("owner", "Ileana")
                                .setExpected("M-a sunat Ileana")
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setArgument("owner", "Petre")
                                .setExpected("M-a sunat Petre")
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);
}

/* static */ FunctionRegistry message2::ListFormatter::customRegistry(UErrorCode& errorCode) {
    return FunctionRegistry::Builder(errorCode)
      .setFormatter(FunctionName("listformat"), new ListFormatterFactory(), errorCode)
      .build();
}

/*
  See ICU4J: CustomFormatterListTest.java
*/
Formatter* ListFormatterFactory::createFormatter(const Locale& locale, UErrorCode& errorCode) {
    if (U_FAILURE(errorCode)) {
        return nullptr;
    }

    Formatter* result = new ListFormatter(locale);
    if (result == nullptr) {
        errorCode = U_MEMORY_ALLOCATION_ERROR;
    }
    return result;
}

message2::FormattedPlaceholder message2::ListFormatter::format(FormattedPlaceholder&& arg, FunctionOptions&& options, UErrorCode& errorCode) const {
    if (U_FAILURE(errorCode)) {
        return {};
    }

    message2::FormattedPlaceholder errorVal = FormattedPlaceholder("listformat");

    // Argument must be present
    if (!arg.canFormat()) {
        errorCode = U_FORMATTING_ERROR;
        return errorVal;
    }
    // Assumes arg is not-yet-formatted
    const Formattable& toFormat = arg.asFormattable();

    FunctionOptionsMap opt = options.getOptions();
    bool hasType = opt.count("type") > 0 && opt["type"].getType() == Formattable::Type::kString;
    UListFormatterType type = UListFormatterType::ULISTFMT_TYPE_AND;
    if (hasType) {
        if (opt["type"].getString() == "OR") {
            type = UListFormatterType::ULISTFMT_TYPE_OR;
        } else if (opt["type"].getString() == "UNITS") {
            type = UListFormatterType::ULISTFMT_TYPE_UNITS;
        }
    }
    bool hasWidth = opt.count("width") > 0 && opt["width"].getType() == Formattable::Type::kString;
    UListFormatterWidth width = UListFormatterWidth::ULISTFMT_WIDTH_WIDE;
    if (hasWidth) {
        if (opt["width"].getString() == "SHORT") {
            width = UListFormatterWidth::ULISTFMT_WIDTH_SHORT;
        } else if (opt["width"].getString() == "NARROW") {
            width = UListFormatterWidth::ULISTFMT_WIDTH_NARROW;
        }
    }
    LocalPointer<icu::ListFormatter> lf(icu::ListFormatter::createInstance(locale, type, width, errorCode));
    if (U_FAILURE(errorCode)) {
        return {};
    }

    UnicodeString result;

    switch (toFormat.getType()) {
        case Formattable::Type::kArray: {
            int32_t n_items;
            const Formattable* objs = toFormat.getArray(n_items);
            if (objs == nullptr) {
                errorCode = U_FORMATTING_ERROR;
                return errorVal;
            }
            LocalArray<UnicodeString> parts(new UnicodeString[n_items]);
            if (!parts.isValid()) {
                errorCode = U_MEMORY_ALLOCATION_ERROR;
                return {};
            }
            for (int32_t i = 0; i < n_items; i++) {
                parts[i] = objs[i].getString();
            }
            lf->format(parts.getAlias(), n_items, result, errorCode);
            break;
        }
        default: {
            result += toFormat.getString();
            break;
        }
    }

    return FormattedPlaceholder(arg, FormattedValue(std::move(result)));
}

void TestMessageFormat2::testListFormatter(IcuTestErrorCode& errorCode) {
    if (U_FAILURE(errorCode)) {
        return;
    }
    const message2::Formattable progLanguages[3] = {
        message2::Formattable("C/C++"),
        message2::Formattable("Java"),
        message2::Formattable("Python")
    };

    TestCase::Builder testBuilder;
    testBuilder.setFunctionRegistry(std::make_shared<FunctionRegistry>(message2::ListFormatter::customRegistry(errorCode)));
    testBuilder.setArgument("languages", progLanguages, 3);

    TestCase test = testBuilder.setName("testListFormatter")
        .setPattern("{I know {$languages :listformat type=AND}!}")
        .setExpected("I know C/C++, Java, and Python!")
        .build();
    TestUtils::runTestCase(*this, test, errorCode);

    test = testBuilder.setName("testListFormatter")
                      .setPattern("{You are allowed to use {$languages :listformat type=OR}!}")
                      .setExpected("You are allowed to use C/C++, Java, or Python!")
                      .build();
    TestUtils::runTestCase(*this, test, errorCode);
}

/*
  See ICU4J: CustomFormatterMessageRefTest.java
*/

/* static */ FunctionRegistry message2::ResourceManager::customRegistry(UErrorCode& errorCode) {
    return FunctionRegistry::Builder(errorCode)
        .setFormatter(FunctionName("msgRef"), new ResourceManagerFactory(), errorCode)
        .build();
}

/* static */ Hashtable* message2::ResourceManager::properties(UErrorCode& errorCode) {
    NULL_ON_ERROR(errorCode);

    LocalPointer<UnicodeString> firefox(new UnicodeString("match {$gcase :select} when genitive {Firefoxin} when * {Firefox}"));
    if (!firefox.isValid()) {
        errorCode = U_MEMORY_ALLOCATION_ERROR;
        return nullptr;
    }
    LocalPointer<UnicodeString> chrome(new UnicodeString("match {$gcase :select} when genitive {Chromen} when * {Chrome}"));
    if (!chrome.isValid()) {
        errorCode = U_MEMORY_ALLOCATION_ERROR;
        return nullptr;
    }
    LocalPointer<UnicodeString> safari(new UnicodeString("match {$gcase :select} when genitive {Safarin} when * {Safari}"));
    if (!safari.isValid()) {
        errorCode = U_MEMORY_ALLOCATION_ERROR;
        return nullptr;
    }

    Hashtable* result = new Hashtable(uhash_compareUnicodeString, nullptr, errorCode);
    if (result == nullptr) {
        return nullptr;
    }
    result->setValueDeleter(uprv_deleteUObject);
    result->put("safari", safari.orphan(), errorCode);
    result->put("firefox", firefox.orphan(), errorCode);
    result->put("chrome", chrome.orphan(), errorCode);
    return result;
}

Formatter* ResourceManagerFactory::createFormatter(const Locale& locale, UErrorCode& errorCode) {
    if (U_FAILURE(errorCode)) {
        return nullptr;
    }

    Formatter* result = new ResourceManager(locale);
    if (result == nullptr) {
        errorCode = U_MEMORY_ALLOCATION_ERROR;
    }
    return result;
}

using Arguments = MessageArguments;

static Arguments localToGlobal(const FunctionOptionsMap& opts, UErrorCode& status) {
    if (U_FAILURE(status)) {
        return {};
    }
    return MessageArguments(opts, status);
}

message2::FormattedPlaceholder ResourceManager::format(FormattedPlaceholder&& arg, FunctionOptions&& options, UErrorCode& errorCode) const {
    if (U_FAILURE(errorCode)) {
        return {};
    }

    message2::FormattedPlaceholder errorVal = message2::FormattedPlaceholder("msgref");

    // Argument must be present
    if (!arg.canFormat()) {
        errorCode = U_FORMATTING_ERROR;
        return errorVal;
    }

    // Assumes arg is not-yet-formatted
    const Formattable& toFormat = arg.asFormattable();
    UnicodeString in;
    switch (toFormat.getType()) {
        case Formattable::Type::kString: {
            in = toFormat.getString();
            break;
        }
        default: {
            // Ignore non-strings
            return errorVal;
        }
    }

    FunctionOptionsMap opt = options.getOptions();
    bool hasProperties = opt.count("resbundle") > 0 && opt["resbundle"].getType() == Formattable::Type::kObject && opt["resbundle"].getObject()->tag() == u"properties";
    // If properties were provided, look up the given string in the properties,
    // yielding a message
    if (hasProperties) {
        const FormattableProperties* properties = reinterpret_cast<const FormattableProperties*>(opt["resbundle"].getObject());
        UnicodeString* msg = static_cast<UnicodeString*>(properties->properties->get(in));
        if (msg == nullptr) {
            // No message given for this key -- error out
            errorCode = U_FORMATTING_ERROR;
            return errorVal;
        }
	MessageFormatter::Builder mfBuilder;
        UParseError parseErr;
        // Any parse/data model errors will be propagated
	MessageFormatter mf = mfBuilder.setPattern(*msg).build(parseErr, errorCode);
        Arguments arguments = localToGlobal(opt, errorCode);
        if (U_FAILURE(errorCode)) {
            return errorVal;
        }

        UErrorCode savedStatus = errorCode;
        UnicodeString result = mf.formatToString(arguments, errorCode);
        // Here, we want to ignore errors (this matches the behavior in the ICU4J test).
        // For example: we want $gcase to default to "$gcase" if the gcase option was
        // omitted.
        if (U_FAILURE(errorCode)) {
            errorCode = savedStatus;
        }
        return FormattedPlaceholder(arg, FormattedValue(std::move(result)));
    } else {
        // Properties must be provided
        errorCode = U_FORMATTING_ERROR;
    }
    return errorVal;
}


void TestMessageFormat2::testMessageRefFormatter(IcuTestErrorCode& errorCode) {
    CHECK_ERROR(errorCode);

    Hashtable* properties = ResourceManager::properties(errorCode);
    CHECK_ERROR(errorCode);
    LocalPointer<FormattableProperties> fProperties(new FormattableProperties(properties));
    if (!fProperties.isValid()) {
        ((UErrorCode&) errorCode) = U_MEMORY_ALLOCATION_ERROR;
        return;
    }

    TestCase::Builder testBuilder;
    testBuilder.setLocale(Locale("ro"));
    testBuilder.setFunctionRegistry(std::make_shared<FunctionRegistry>(ResourceManager::customRegistry(errorCode)));
    testBuilder.setPattern(*((UnicodeString*) properties->get("firefox")));
    testBuilder.setName("message-ref");

    TestCase test = testBuilder.setArgument("gcase", "whatever")
                                .setExpected("Firefox")
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);
    test = testBuilder.setArgument("gcase", "genitive")
                                .setExpected("Firefoxin")
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    testBuilder.setPattern(*((UnicodeString*) properties->get("chrome")));

    test = testBuilder.setArgument("gcase", "whatever")
                                .setExpected("Chrome")
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);
    test = testBuilder.setArgument("gcase", "genitive")
                                .setExpected("Chromen")
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    testBuilder.setArgument("res", fProperties.getAlias());

    testBuilder.setPattern("{Please start {$browser :msgRef gcase=genitive resbundle=$res}}");
    test = testBuilder.setArgument("browser", "firefox")
                                .setExpected("Please start Firefoxin")
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);
    test = testBuilder.setArgument("browser", "chrome")
                                .setExpected("Please start Chromen")
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);
    test = testBuilder.setArgument("browser", "safari")
                                .setExpected("Please start Safarin")
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);

    testBuilder.setPattern("{Please start {$browser :msgRef resbundle=$res}}");
    test = testBuilder.setArgument("browser", "firefox")
                                .setExpected("Please start Firefox")
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);
    test = testBuilder.setArgument("browser", "chrome")
                                .setExpected("Please start Chrome")
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);
    test = testBuilder.setArgument("browser", "safari")
                                .setExpected("Please start Safari")
                                .build();
    TestUtils::runTestCase(*this, test, errorCode);
}

#endif /* #if !UCONFIG_NO_FORMATTING */
