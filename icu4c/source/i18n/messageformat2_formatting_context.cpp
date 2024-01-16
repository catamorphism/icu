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

ExpressionContext::ExpressionContext(MessageContext& c, const UnicodeString& fallback)
    : context(c), contents(FormattedValue(fallback)) {}

ExpressionContext ExpressionContext::create() const {
    return ExpressionContext(context, UnicodeString(REPLACEMENT));
}

ExpressionContext::ExpressionContext(ExpressionContext&& other) : context(other.context) {
    hasPendingFunctionName = other.hasPendingFunctionName;
    if (hasPendingFunctionName) {
	pendingFunctionName = std::move(other.pendingFunctionName);
    }
    fallback = std::move(other.fallback);
    contents = std::move(other.contents);
    functionOptions = std::move(other.functionOptions);
    functionOptionsLen = other.functionOptionsLen;
}

// State
// ---------

bool ExpressionContext::isFallback() const {
    return contents.isFallback();
}

void ExpressionContext::setFallback() {
    contents = FormattedValue(fallback);
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

void ExpressionContext::setContents(FormattedValue&& val) {
    contents = std::move(val);
}

UBool ExpressionContext::canFormat() const {
    return (!(contents.isFallback() || contents.isNullOperand()));
}

const FormattedValue& ExpressionContext::getContents() const {
    U_ASSERT(!(contents.isFallback() || contents.isNullOperand()));
    return contents;
}

// Called when output is required and the contents are an unevaluated `Formattable`;
// formats the source `Formattable` to a string with defaults, if it can be
// formatted with a default formatter
static FormattedValue formatWithDefaults(const Locale& locale, const Formattable& input, const UnicodeString& fallback, UErrorCode& status) {
    if (U_FAILURE(status)) {
        return {};
    }

    // Try as decimal number first
    if (input.isNumeric()) {
        // Note: the ICU Formattable has to be created here since the StringPiece
        // refers to state inside the Formattable; so otherwise we'll have a reference
        // to a temporary object
        icu::Formattable icuFormattable = input.asICUFormattable();
        StringPiece asDecimal = icuFormattable.getDecimalNumber(status);
        if (U_FAILURE(status)) {
            return {};
        }
        if (asDecimal != nullptr) {
            return FormattedValue(formatNumberWithDefaults(locale, asDecimal, status), input);
        }
    }

    switch (input.getType()) {
    case Formattable::Type::kDate: {
        UnicodeString result;
        formatDateWithDefaults(locale, input.getDate(), result, status);
        return FormattedValue(std::move(result), input);
    }
    case Formattable::Type::kDouble: {
        return FormattedValue(formatNumberWithDefaults(locale, input.getDouble(), status), input);
    }
    case Formattable::Type::kLong: {
        return FormattedValue(formatNumberWithDefaults(locale, input.getLong(), status), input);
    }
    case Formattable::Type::kInt64: {
        return FormattedValue(formatNumberWithDefaults(locale, input.getInt64(), status), input);
    }
    case Formattable::Type::kString: {
        return FormattedValue(UnicodeString(input.getString()), input);
    }
    default: {
        // No default formatters for other types; use fallback
        return FormattedValue(UnicodeString(fallback), input);
    }
    }
}

// Called when string output is required; forces output to be produced
// if none is present (including formatting number output as a string)
UnicodeString ExpressionContext::formatToString(const Locale& locale, UErrorCode& status) {
    if (contents.isEvaluated() && contents.isString()) {
        return contents.getString();
    }
    if (contents.isFallback()) {
        return contents.getString();
    }
    if (contents.isNullOperand()) {
        // No operand and a function call hasn't cleared the state --
        // use fallback
        setFallback();
        return fallback;
    }
    return formatToString(locale, getContents(), status);
}

// Called when string output is required; forces output to be produced
// if none is present (including formatting number output as a string)
UnicodeString ExpressionContext::formatToString(const Locale& locale, const FormattedValue& val, UErrorCode& status) {
    if (U_FAILURE(status)) {
        return {};
    }

    // Evaluated value: either just return the string, or format the number
    // as a string and return it
    if (val.isEvaluated()) {
        if (val.isString() || val.isFallback()) {
            return val.getString();
        } else {
            return val.getNumber().toString(status);
        }
    }
    // Unevaluated value: first evaluate it fully, then format
    return formatToString(locale, formatWithDefaults(locale, val.asFormattable(), fallback, status), status);
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

int32_t ExpressionContext::optionsCount() const { return functionOptionsLen; }

// Takes a vector of ResolvedFunctionOptions
void ExpressionContext::adoptFunctionOptions(UVector* opt, UErrorCode& status) {
    CHECK_ERROR(status);
    U_ASSERT(opt != nullptr);

    functionOptions.adoptInstead(moveVectorToArray<ResolvedFunctionOption>(*opt, functionOptionsLen));
    if (!functionOptions.isValid()) {
        status = U_MEMORY_ALLOCATION_ERROR;
    }
    delete opt;
}

const ResolvedFunctionOption* ExpressionContext::getResolvedFunctionOptions(int32_t& len) const {
    len = functionOptionsLen;
    U_ASSERT(functionOptions.isValid());
    return functionOptions.getAlias();
}

UBool ExpressionContext::getFunctionOption(const UnicodeString& key, Formattable& option) const {
    if (!functionOptions.isValid()) {
        U_ASSERT(functionOptionsLen == 0);
    }
    for (int32_t i = 0; i < functionOptionsLen; i++) {
        const ResolvedFunctionOption& opt = functionOptions[i];
        if (opt.getName() == key) {
            option = opt.getValue();
            return true;
        }
    }
    return false;
}

    /*
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
    */

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

ResolvedFunctionOption::ResolvedFunctionOption(ResolvedFunctionOption&& other) {
    name = std::move(other.name);
    value = std::move(other.value);
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
FormattedValue ExpressionContext::evalFormatterCall(const FunctionName& functionName, UErrorCode& status) {
    if (U_FAILURE(status)) {
        return {};
    }

    FunctionName savedFunctionName;
    bool hadFunctionName = hasFunctionName();
    if (hadFunctionName) {
      savedFunctionName = pendingFunctionName;
      clearFunctionName();
    }
    setFunctionName(functionName);
    if (hasFormatter()) {
        const Formatter& formatterImpl = getFormatter(status);
        if (U_FAILURE(status)) {
            return {};
        }
        UErrorCode savedStatus = status;
        FormattedValue result = formatterImpl.format(*this, status);
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
            result = FormattedValue(fallback);
        }
        returnFromFunction();
        if (hadFunctionName) {
            setFunctionName(savedFunctionName);
        }
        return result;
    }
    // No formatter with this name -- set error
    if (context.isSelector(functionName)) {
        setFormattingError(functionName.toString(), status);
    } else {
        context.getErrors().setUnknownFunction(functionName, status);
    }
    setFallback();
    return FormattedValue(fallback);
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
