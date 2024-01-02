// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/messageformat2_formatting_context.h"
#include "unicode/messageformat2_function_registry.h"
#include "unicode/messageformat2.h"
#include "messageformat2_context.h"
#include "messageformat2_expression_context.h"
#include "messageformat2_macros.h"
#include "hash.h"
#include "uvector.h" // U_ASSERT

#if U_PF_WINDOWS <= U_PLATFORM && U_PLATFORM <= U_PF_CYGWIN && defined(_MSC_VER)
// Ignore warning 4661 as LocalPointerBase does not use operator== or operator!=
#pragma warning(disable: 4661)
#endif

U_NAMESPACE_BEGIN

namespace message2 {

using namespace data_model;

// Context that's specific to formatting a single expression

// Constructors
// ------------

ExpressionContext::ExpressionContext(MessageContext& c, UErrorCode& status) : context(c), inState(FALLBACK), outState(NONE) {
    functionObjectOptions.adoptInstead(new Hashtable(status));
}

ExpressionContext ExpressionContext::create(UErrorCode& status) const {
    return ExpressionContext(context, status);
}

ExpressionContext::ExpressionContext(ExpressionContext&& other) : context(other.context), inState(other.inState), outState(other.outState) {
    hasPendingFunctionName = other.hasPendingFunctionName;
    if (hasPendingFunctionName) {
	pendingFunctionName = std::move(other.pendingFunctionName);
    }
    fallback = std::move(other.fallback);
    input = std::move(other.input);
    objectInput = other.objectInput;
    stringOutput = std::move(other.stringOutput);
    numberOutput = std::move(other.numberOutput);
    functionOptions = std::move(other.functionOptions);
    functionOptionsLen = other.functionOptionsLen;
    functionObjectOptions = std::move(other.functionObjectOptions);
}

// State
// ---------

void ExpressionContext::enterState(InputState s) {
    // If we're entering an error state, clear the output
    if (s == InputState::FALLBACK) {
        enterState(OutputState::NONE);
    }
    inState = s;
}

void ExpressionContext::enterState(OutputState s) {
    // Input must exist if output exists
    if (s > OutputState::NONE) {
        U_ASSERT(hasInput());
    }
    outState = s;
}

bool ExpressionContext::isFallback() const {
    return (inState == InputState::FALLBACK);
}

void ExpressionContext::setFallback() {
    enterState(FALLBACK);
}

// Fallback values are enclosed in curly braces;
// see https://github.com/unicode-org/message-format-wg/blob/main/spec/formatting.md#formatting-fallback-values
static void fallbackToString(const UnicodeString& s, UnicodeString& result) {
    result += LEFT_CURLY_BRACE;
    result += s;
    result += RIGHT_CURLY_BRACE;
}

void ExpressionContext::setFallbackTo(const FunctionName& f) {
    fallback.remove();
    fallbackToString(f.toString(), fallback);
}

void ExpressionContext::setFallbackTo(const VariableName& v) {
    fallback.remove();
    fallbackToString(v.declaration(), fallback);
}

void ExpressionContext::setFallbackTo(const Literal& l) {
    fallback.remove();
    fallbackToString(l.quoted(), fallback);
}

// Add the fallback string as the input string, and
// unset this as a fallback
void ExpressionContext::promoteFallbackToInput() {
    U_ASSERT(isFallback());
    return setInput(fallback);
}

// Add the fallback string as the output string
void ExpressionContext::promoteFallbackToOutput() {
    U_ASSERT(isFallback());
    return setOutput(fallback);
}

// Used when handling function calls with no argument
void ExpressionContext::setNoOperand() {
    U_ASSERT(isFallback());
    enterState(NO_OPERAND);
}

void ExpressionContext::setInput(const UnicodeString& s) {
    U_ASSERT(inState <= NO_OPERAND);
    enterState(FORMATTABLE_INPUT);
    input = Formattable(s);
}

void ExpressionContext::setInput(const Formattable& s) {
    U_ASSERT(isFallback());
    enterState(FORMATTABLE_INPUT);
    U_ASSERT(s.getType() != Formattable::Type::kObject);
    input = s;
}

UBool ExpressionContext::hasFormattableInput() const {
    return (inState == InputState::FORMATTABLE_INPUT);
}

UBool ExpressionContext::hasObjectInput() const {
    return (inState == InputState::OBJECT_INPUT);
}

const UObject& ExpressionContext::getObjectInput() const {
    U_ASSERT(hasObjectInput());
    return *objectInput;
}

const UObject* ExpressionContext::getObjectInputPointer() const {
    U_ASSERT(hasObjectInput());
    return objectInput;
}

const Formattable& ExpressionContext::getFormattableInput() const {
    U_ASSERT(hasFormattableInput());
    return input;
}

const number::FormattedNumber& ExpressionContext::getNumberOutput() const {
    U_ASSERT(hasNumberOutput());
    return numberOutput;
}

UBool ExpressionContext::hasStringOutput() const {
    return (inState > FALLBACK && outState == OutputState::STRING);
}

UBool ExpressionContext::hasNumberOutput() const {
    return (inState > FALLBACK && outState == OutputState::NUMBER);
}

const UnicodeString& ExpressionContext::getStringOutput() const {
    U_ASSERT(hasStringOutput());
    return stringOutput;
}

void ExpressionContext::setInput(const UObject* obj) {
    U_ASSERT(isFallback());
    U_ASSERT(obj != nullptr);
    enterState(OBJECT_INPUT);
    objectInput = obj;
}

void ExpressionContext::setOutput(const UnicodeString& s) {
    if (inState == InputState::NO_OPERAND) {
        // If setOutput() is called while the
        // operand is null, set the input to the
        // output string
        setInput(s);
    }
    U_ASSERT(hasInput());
    enterState(OutputState::STRING);
    stringOutput = s;
}

void ExpressionContext::setOutput(number::FormattedNumber&& num) {
    U_ASSERT(hasInput());
    enterState(OutputState::NUMBER);
    numberOutput = std::move(num);
}

void ExpressionContext::clearOutput() {
    stringOutput.remove();
    enterState(OutputState::NONE);
}

// Called when output is required and no output is present;
// formats the input to a string with defaults, for inputs that can be
// formatted with a default formatter
void ExpressionContext::formatInputWithDefaults(const Locale& locale, UErrorCode& status) {
    CHECK_ERROR(status);

    U_ASSERT(hasFormattableInput());
    U_ASSERT(!hasOutput());

    // Try as decimal number first
    if (input.isNumeric()) {
        StringPiece asDecimal = input.getDecimalNumber(status);
        CHECK_ERROR(status);
        if (asDecimal != nullptr) {
            setOutput(formatNumberWithDefaults(locale, asDecimal, status));
            return;
        }
    }

    switch (input.getType()) {
    case Formattable::Type::kDate: {
        formatDateWithDefaults(locale, input.getDate(), stringOutput, status);
        enterState(OutputState::STRING);
        break;
    }
    case Formattable::Type::kDouble: {
        setOutput(formatNumberWithDefaults(locale, input.getDouble(), status));
        break;
    }
    case Formattable::Type::kLong: {
        setOutput(formatNumberWithDefaults(locale, input.getLong(), status));
        break;
    }
    case Formattable::Type::kInt64: {
        setOutput(formatNumberWithDefaults(locale, input.getInt64(), status));
        break;
    }
    case Formattable::Type::kString: {
        setOutput(input.getString());
        break;
    }
    default: {
        // No default formatters for other types; use fallback
        promoteFallbackToOutput();
    }
    }
}

// Called when string output is required; forces output to be produced
// if none is present (including formatting number output as a string)
void ExpressionContext::formatToString(const Locale& locale, UErrorCode& status) {
    CHECK_ERROR(status);

    switch (outState) {
        case OutputState::STRING: {
            return; // Nothing to do
        }
        case OutputState::NUMBER: {
            setOutput(numberOutput.toString(status));
            return;
        }
        default: {
            break;
        }
    }
    switch (inState) {
        case InputState::FALLBACK: {
            setInput(fallback);
            setOutput(fallback);
            break;
        }
        case InputState::NO_OPERAND:
            // No operand and a function call hasn't cleared the state --
            // use fallback
        case InputState::OBJECT_INPUT: {
            setFallback();
            promoteFallbackToOutput();
            break;
        }
        case InputState::FORMATTABLE_INPUT: {
            formatInputWithDefaults(locale, status);
            // Force number to string, in case the result was a number
            formatToString(locale, status);
            break;
        }
    }
    CHECK_ERROR(status);
    U_ASSERT(hasStringOutput());
}

void ExpressionContext::clearFunctionName() {
    U_ASSERT(hasPendingFunctionName);
    hasPendingFunctionName = false;
}

const FunctionName& ExpressionContext::getFunctionName() {
    U_ASSERT(hasPendingFunctionName);
    return pendingFunctionName;
}

// Function options
// ----------------

void ExpressionContext::setObjectOption(const UnicodeString& key, const UObject* value, UErrorCode& status) {
    CHECK_ERROR(status);

    // The const_cast is safe because no methods that allow
    // writing to `value` are exposed
    U_ASSERT(functionObjectOptions.isValid());
    functionObjectOptions->put(key, const_cast<UObject*>(value), status);
}


int32_t ExpressionContext::optionsCount() const { return functionOptionsLen; }

// Takes a vector of ResolvedFunctionOptions
void ExpressionContext::adoptFunctionOptions(UVector* opt, UErrorCode& status) {
    CHECK_ERROR(status);
    U_ASSERT(opt != nullptr);

    functionOptions.adoptInstead(copyVectorToArray<ResolvedFunctionOption>(*opt, functionOptionsLen));
    if (!functionOptions.isValid()) {
        status = U_MEMORY_ALLOCATION_ERROR;
    }
    delete opt;
}

UBool ExpressionContext::getNumericOption(const UnicodeString& key, Formattable& result) const {
    if (!getFunctionOption(key, result)) {
        return false;
    }
    if (!result.isNumeric()) {
        return false;
    }
    return true;
}

UBool ExpressionContext::getStringOption(const UnicodeString& key, UnicodeString& value) const {
    Formattable result;
    if (!getFunctionOption(key, result) || result.getType() != Formattable::kString) {
      return false;
    }
    value = result.getString();
    return true;
}

const UObject& ExpressionContext::getObjectOption(const UnicodeString& key) const {
    U_ASSERT(functionObjectOptions.isValid());
    const UObject* result = static_cast<const UObject*>(functionObjectOptions->get(key));
    U_ASSERT(result != nullptr);
    return *result;
}

const ResolvedFunctionOption* ExpressionContext::getResolvedFunctionOptions(int32_t& len) const {
    len = functionOptionsLen;
    U_ASSERT(functionOptions.isValid());
    return functionOptions.getAlias();
}

UBool ExpressionContext::getFunctionOption(const UnicodeString& key, Formattable& value) const {
    if (!functionOptions.isValid()) {
        U_ASSERT(functionOptionsLen == 0);
    }
    for (int32_t i = 0; i < functionOptionsLen; i++) {
        const ResolvedFunctionOption& opt = functionOptions[i];
        if (opt.getName() == key) {
            U_ASSERT(opt.getValue().getType() != Formattable::kObject);
            value = opt.getValue();
            return true;
        }
    }
    return false;
}

UBool ExpressionContext::hasObjectOption(const UnicodeString& key) const {
    return (functionObjectOptions->get(key) != nullptr);
}

bool ExpressionContext::tryStringAsNumberOption(const UnicodeString& key, double& value) const {
    // Check for a string option, try to parse it as a number if present
    UnicodeString tempValue;
    if (!getStringOption(key, tempValue)) {
        return false;
    }
    UErrorCode localErrorCode = U_ZERO_ERROR;
    LocalPointer<NumberFormat> numberFormat(NumberFormat::createInstance(context.messageFormatter().getLocale(), localErrorCode));
    if (U_FAILURE(localErrorCode)) {
        return false;
    }
    Formattable asNumber;
    numberFormat->parse(tempValue, asNumber, localErrorCode);
    if (U_FAILURE(localErrorCode)) {
        return false;
    }
    value = asNumber.getDouble(localErrorCode);
    if (U_FAILURE(localErrorCode)) {
        return false;
    }
    return true;
}

UBool ExpressionContext::getInt64Option(const UnicodeString& key, int64_t& value) const {
    Formattable val;
    UBool isNumeric = getNumericOption(key, val);
    if (!isNumeric) {
        double doubleResult;
        if (tryStringAsNumberOption(key, doubleResult)) {
            value = (int64_t) doubleResult;
            return true;
        }
        return false;
    }
    UErrorCode localErrorCode = U_ZERO_ERROR;
    value = val.getInt64(localErrorCode);
    if (U_SUCCESS(localErrorCode)) {
        return true;
    }
    // Option was numeric but couldn't be converted to int64_t -- could be overflow
    return false;
}

UBool ExpressionContext::getDoubleOption(const UnicodeString& key, double& value) const {
    Formattable val;
    UBool isNumeric = getNumericOption(key, val);
    if (!isNumeric) {
        return tryStringAsNumberOption(key, value);
    }
    UErrorCode localErrorCode = U_ZERO_ERROR;
    value = val.getDouble(localErrorCode);
    // The conversion must succeed, since the result is numeric
    U_ASSERT(U_SUCCESS(localErrorCode));
    return true;
}


// Functions
// -------------

void ExpressionContext::setFunctionName(const FunctionName& fn) {
    U_ASSERT(!hasFunctionName());
    hasPendingFunctionName = true;
    pendingFunctionName = fn;
}

bool ExpressionContext::hasFunctionName() const {
    return hasPendingFunctionName;
}

void ExpressionContext::returnFromFunction() {
    U_ASSERT(hasFunctionName());
    clearFunctionName();
    clearFunctionOptions();
}

void ExpressionContext::clearFunctionOptions() {
    U_ASSERT(functionOptions.isValid() || functionOptionsLen == 0);
    functionOptions.adoptInstead(nullptr);
    functionOptionsLen = 0;
}

ResolvedFunctionOption::~ResolvedFunctionOption() {}

// Precondition: pending function name is set and selector is defined
// Postcondition: selector != nullptr
std::unique_ptr<Selector> ExpressionContext::getSelector(UErrorCode& status) const {
    NULL_ON_ERROR(status);

    U_ASSERT(hasFunctionName());
    const SelectorFactory* selectorFactory = context.lookupSelectorFactory(pendingFunctionName, status);
    if (selectorFactory == nullptr) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return nullptr;
    }
    // Create a specific instance of the selector
    std::unique_ptr<Selector> result(selectorFactory->createSelector(context.messageFormatter().getLocale(), status));
    NULL_ON_ERROR(status);
    return result;
}

// Precondition: pending function name is set and formatter is defined
const Formatter& ExpressionContext::getFormatter(UErrorCode& status) {
    U_ASSERT(hasFunctionName());
    U_ASSERT(hasFormatter());
    return *(context.maybeCachedFormatter(pendingFunctionName, status));
}

bool ExpressionContext::hasFormatter() const {
    U_ASSERT(hasFunctionName());
    return context.isFormatter(pendingFunctionName);
}

bool ExpressionContext::hasSelector() const {
    if (!hasFunctionName()) {
        return false;
    }
    return context.isSelector(pendingFunctionName);
}

void ExpressionContext::evalPendingSelectorCall(const UVector& keys, UVector& keysOut, UErrorCode& status) {
    U_ASSERT(hasSelector());
    LocalPointer<Selector> selectorImpl(getSelector(status));
    CHECK_ERROR(status);
    UErrorCode savedStatus = status;

    // Convert `keys` to an array
    int32_t keysLen = keys.size();
    UnicodeString* keysArr = new UnicodeString[keysLen];
    if (keysArr == nullptr) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
    for (int32_t i = 0; i < keysLen; i++) {
        const UnicodeString* k = static_cast<UnicodeString*>(keys[i]);
        U_ASSERT(k != nullptr);
        keysArr[i] = *k;
    }
    LocalArray<UnicodeString> adoptedKeys(keysArr);

    // Create an array to hold the output
    UnicodeString* prefsArr = new UnicodeString[keysLen];
    if (prefsArr == nullptr) {
        status = U_MEMORY_ALLOCATION_ERROR;
        return;
    }
    LocalArray<UnicodeString> adoptedPrefs(prefsArr);
    int32_t prefsLen = 0;

    // Call the selector
    selectorImpl->selectKey(*this, adoptedKeys.getAlias(), keysLen, adoptedPrefs.getAlias(), prefsLen, status);

    // Update errors
    if (savedStatus != status) {
        if (U_FAILURE(status)) {
            setFallback();
            status = U_ZERO_ERROR;
            setSelectorError(pendingFunctionName.toString(), status);
        } else {
            // Ignore warnings
            status = savedStatus;
        }
    }

    CHECK_ERROR(status);

    // Copy the resulting keys (if there was no error)
    keysOut.removeAllElements();
    for (int32_t i = 0; i < prefsLen; i++) {
        UnicodeString* k = message2::create<UnicodeString>(std::move(prefsArr[i]), status);
        if (k == nullptr) {
            status = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
        keysOut.adoptElement(k, status);
        CHECK_ERROR(status);
    }

    // Clean up state
    returnFromFunction();
}

// Calls the pending formatter
void ExpressionContext::evalFormatterCall(const FunctionName& functionName, UErrorCode& status) {
    CHECK_ERROR(status);

    FunctionName savedFunctionName;
    bool hadFunctionName = hasFunctionName();
    if (hadFunctionName) {
      savedFunctionName = pendingFunctionName;
      clearFunctionName();
    }
    setFunctionName(functionName);
    CHECK_ERROR(status);
    if (hasFormatter()) {
        const Formatter& formatterImpl = getFormatter(status);
        CHECK_ERROR(status);
        UErrorCode savedStatus = status;
        formatterImpl.format(*this, status);
        // Update errors
        if (savedStatus != status) {
            if (U_FAILURE(status)) {
                // Convey any error generated by the formatter
                // as a formatting error
                setFallback();
                status = U_ZERO_ERROR;
                setFormattingError(functionName.toString(), status);
            } else {
                // Ignore warnings
                status = savedStatus;
            }
        }
        // Ignore the output if any errors occurred
        if (context.getErrors().hasFormattingError()) {
            clearOutput();
        }
        returnFromFunction();
        if (hadFunctionName) {
            setFunctionName(savedFunctionName);
        }
        return;
    }
    // No formatter with this name -- set error
    if (context.isSelector(functionName)) {
        setFormattingError(functionName.toString(), status);
    } else {
        context.getErrors().setUnknownFunction(functionName, status);
    }
    setFallback();
}

// Default formatters
// ------------------

number::FormattedNumber formatNumberWithDefaults(const Locale& locale, double toFormat, UErrorCode& errorCode) {
    return number::NumberFormatter::withLocale(locale).formatDouble(toFormat, errorCode);
}

number::FormattedNumber formatNumberWithDefaults(const Locale& locale, int32_t toFormat, UErrorCode& errorCode) {
    return number::NumberFormatter::withLocale(locale).formatInt(toFormat, errorCode);
}

number::FormattedNumber formatNumberWithDefaults(const Locale& locale, int64_t toFormat, UErrorCode& errorCode) {
    return number::NumberFormatter::withLocale(locale).formatInt(toFormat, errorCode);
}

number::FormattedNumber formatNumberWithDefaults(const Locale& locale, StringPiece toFormat, UErrorCode& errorCode) {
    return number::NumberFormatter::withLocale(locale).formatDecimal(toFormat, errorCode);
}

DateFormat* defaultDateTimeInstance(const Locale& locale, UErrorCode& errorCode) {
    NULL_ON_ERROR(errorCode);
    LocalPointer<DateFormat> df(DateFormat::createDateTimeInstance(DateFormat::SHORT, DateFormat::SHORT, locale));
    if (!df.isValid()) {
        errorCode = U_MEMORY_ALLOCATION_ERROR;
        return nullptr;
    }
    return df.orphan();
}

void formatDateWithDefaults(const Locale& locale, UDate date, UnicodeString& result, UErrorCode& errorCode) {
    CHECK_ERROR(errorCode);

    LocalPointer<DateFormat> df(defaultDateTimeInstance(locale, errorCode));
    CHECK_ERROR(errorCode);
    df->format(date, result, 0, errorCode);
}

// Errors
// -------

void ExpressionContext::setFormattingError(const UnicodeString& formatterName, UErrorCode& status) {
    context.getErrors().setFormattingError(formatterName, status);
}

void ExpressionContext::setSelectorError(const UnicodeString& selectorName, UErrorCode& status) {
    context.getErrors().setSelectorError(selectorName, status);
}

ExpressionContext::~ExpressionContext() {}
FormattingContext::~FormattingContext() {}

} // namespace message2
U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */
