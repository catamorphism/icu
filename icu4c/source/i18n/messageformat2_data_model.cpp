// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/messageformat2_data_model.h"
#include "unicode/messageformat2.h"
#include "messageformat2_macros.h"
#include "uvector.h" // U_ASSERT

#if U_PF_WINDOWS <= U_PLATFORM && U_PLATFORM <= U_PF_CYGWIN && defined(_MSC_VER)
// Ignore warning 4661 as LocalPointerBase does not use operator== or operator!=
#pragma warning(disable: 4661)
#endif

U_NAMESPACE_BEGIN

namespace message2 {

// Implementation

//------------------ SelectorKeys

const Key* SelectorKeys::getKeysInternal() const {
    return keys.getAlias();
}

// Lexically order key lists
bool SelectorKeys::operator<(const SelectorKeys& other) const {
    // Handle key lists of different sizes first --
    // this case does have to be handled (even though it would
    // reflect a data model error) because of the need to produce
    // partial output
    if (len < other.len) {
        return true;
    }
    if (len > other.len) {
        return false;
    }

    for (int32_t i = 0; i < len; i++) {
        if (keys[i] < other.keys[i]) {
            return true;
        }
        if (!(keys[i] == other.keys[i])) {
            return false;
        }
    }
    // If we've reached here, all keys must be equal
    return false;
}

SelectorKeys::Builder::Builder(UErrorCode& status) {
    if (U_FAILURE(status)) {
        keys = nullptr;
        return;
    }
    LocalPointer<UVector> keysTemp(new UVector(status));
    if (U_FAILURE(status)) {
        keys = nullptr;
        return;
    }
    keys = keysTemp.orphan();
    keys->setDeleter(uprv_deleteUObject);
}

SelectorKeys::Builder& SelectorKeys::Builder::add(Key&& key, UErrorCode& status) noexcept {
    if (U_SUCCESS(status)) {
        Key* k = Key::create(std::move(key), status);
        keys->adoptElement(k, status);
    }
    return *this;
}

SelectorKeys SelectorKeys::Builder::build(UErrorCode& status) const {
    if (U_FAILURE(status)) {
        return {};
    }
    U_ASSERT(keys != nullptr);
    return SelectorKeys(*keys, status);
}

SelectorKeys::Builder::~Builder() {
    if (keys != nullptr) {
        delete keys;
    }
}

SelectorKeys::SelectorKeys(const UVector& ks, UErrorCode& status) : len(ks.size()) {
    if (U_FAILURE(status)) {
        return;
    }
    Key* result = new Key[len];
    if (result == nullptr) {
        status = U_MEMORY_ALLOCATION_ERROR;
        len = 0;
        return;
    }
    for (int32_t i = 0; i < len; i++) {
        U_ASSERT(ks[i] != nullptr);
        result[i] = *(static_cast<Key*>(ks[i]));
    }
    keys = LocalArray<Key>(result);
}

SelectorKeys& SelectorKeys::operator=(const SelectorKeys& other) {
    len = other.len;
    Key* result = new Key[len];
    if (result == nullptr) {
        // Set length to 0 to prevent the
        // keys array from being accessed
        len = 0;
    } else {
        for (int32_t i = 0; i < len; i++) {
            result[i] = other.keys[i];
        }
        keys = LocalArray<Key>(result);
        if (!keys.isValid()) {
            // Set length to 0 to prevent
            // the keys array from being accessed
            len = 0;
        }
    }
    return *this;
}

SelectorKeys& SelectorKeys::operator=(SelectorKeys&& other) noexcept {
    len = other.len;
    keys = LocalArray<Key>(other.keys.orphan());
    other.len = 0;

    return *this;
}

SelectorKeys::SelectorKeys(SelectorKeys&& other) noexcept {
    len = other.len;
    keys = LocalArray<Key>(other.keys.orphan());
    other.len = 0;
}

SelectorKeys::SelectorKeys(const SelectorKeys& other) {
    if (other.keys == nullptr) {
        len = 0;
        keys = nullptr;
        return;
    }
    len = other.len;
    Key* result = new Key[len];
    if (result == nullptr) {
        len = 0;
        keys = nullptr;
        return;
    }
    for (int32_t i = 0; i < len; i++) {
        result[i] = other.keys[i];
    }
    keys = LocalArray<Key>(result);
    if (!keys.isValid()) {
        len = 0;
    }
}

SelectorKeys::~SelectorKeys() {
    len = 0;
}

//------------------ VariableName

UnicodeString VariableName::declaration() const {
    UnicodeString result(DOLLAR);
    result += variableName;
    return result;
}

VariableName::~VariableName() {}

//------------------ Literal

bool Literal::operator<(const Literal& other) const {
    // Ignore quoting for the purposes of ordering
    return contents < other.contents;
}

bool Literal::operator==(const Literal& other) const {
    // Ignore quoting for the purposes of ordering
    return contents == other.contents;
}

UnicodeString Literal::quoted() const {
    UnicodeString result(PIPE);
    result += unquoted();
    result += PIPE;
    return result;
}

const UnicodeString& Literal::unquoted() const { return contents; }

Literal& Literal::operator=(Literal&& other) noexcept {
    thisIsQuoted = other.thisIsQuoted;
    contents = std::move(other.contents);

    return *this;
}

Literal& Literal::operator=(const Literal& other) {
    if (this != &other) {
        thisIsQuoted = other.thisIsQuoted;
        contents = other.contents;
    }
    return *this;
}

Literal::Literal(Literal&& other) noexcept {
    thisIsQuoted = other.thisIsQuoted;
    contents = std::move(other.contents);
}


Literal::~Literal() {
    thisIsQuoted = false;
}

//------------------ Operand

Operand::Operand(const Operand& other) : var(other.var), lit(other.lit), type(other.type) {}

Operand& Operand::operator=(Operand&& other) noexcept {
    this->~Operand();
    switch (other.type) {
        case Type::VARIABLE: {
            var = std::move(other.var);
            break;
        }
        case Type::LITERAL: {
            lit = std::move(other.lit);
            break;
        }
        default: {
            break;
        }
    }
    type = other.type;
    return *this;
}

Operand& Operand::operator=(const Operand& other) {
    if (this != &other) {
        switch (other.type) {
        case Type::VARIABLE: {
            var = other.var;
            break;
        }
        case Type::LITERAL: {
            lit = other.lit;
            break;
        }
        default: {
            break;
        }
        }
        type = other.type;
    }
    return *this;
}

UBool Operand::isVariable() const { return type == Type::VARIABLE; }
UBool Operand::isLiteral() const { return type == Type::LITERAL; }
UBool Operand::isNull() const { return type == Type::NULL_OPERAND; }

const Literal& Operand::asLiteral() const {
    U_ASSERT(isLiteral());
    return lit;
}

const VariableName& Operand::asVariable() const {
    U_ASSERT(isVariable());
    return var;
}

Operand::~Operand() {}

//---------------- Key

/* static */ Key* Key::create(Key&& k, UErrorCode& status) {
    NULL_ON_ERROR(status);
    Key* result = new Key(std::move(k));
    if (result == nullptr) {
        status = U_MEMORY_ALLOCATION_ERROR;
    }
    return result;
}

Key& Key::operator=(Key&& other) noexcept {
    wildcard = other.wildcard;
    if (!other.wildcard) {
        contents = std::move(other.contents);
        other.contents = Literal();
        other.wildcard = true;
    } else {
        contents = Literal();
    }
    return *this;
}

Key& Key::operator=(const Key& other) {
    wildcard = other.wildcard;
    if (!other.wildcard) {
        contents = other.contents;
    }
    return *this;
}

bool Key::operator<(const Key& other) const {
    // Arbitrarily treat * as greater than all concrete keys
    if (wildcard) {
        return false;
    }
    if (other.wildcard) {
        return true;
    }
    return (asLiteral() < other.asLiteral());
}

bool Key::operator==(const Key& other) const {
    if (wildcard) {
        return other.wildcard;
    }
    return (asLiteral() == other.asLiteral());
}

UnicodeString Key::toString() const {
    if (isWildcard()) {
        return UnicodeString(ASTERISK);
    }
    return contents.unquoted();
}

const Literal& Key::asLiteral() const {
    U_ASSERT(!isWildcard());
    return contents;
}

Key::~Key() {}

// ------------ Reserved

// Copy constructor
Reserved::Reserved(const Reserved& other) noexcept : parts(other.parts) {}

Reserved::Reserved(const std::vector<Literal>& ps) noexcept : parts(ps) {}

int32_t Reserved::numParts() const {
    return parts.size();
}

const Literal& Reserved::getPart(int32_t i) const {
    U_ASSERT(i < numParts());
    return parts[i];
}

Reserved::Builder::Builder() : parts(std::vector<Literal>()) {}

Reserved Reserved::Builder::build() const noexcept {
    return Reserved(parts);
}

Reserved::Builder& Reserved::Builder::add(const Literal& part) noexcept {
    parts.push_back(part);
    return *this;
}

Reserved& Reserved::operator=(Reserved&& other) noexcept {
    this->~Reserved();

    parts = std::move(other.parts);

    return *this;
}

Reserved& Reserved::operator=(const Reserved& other) noexcept {
    if (this != &other) {
        parts = other.parts;
    }
    return *this;
}

Reserved::Builder::~Builder() {}

//------------------------ Operator

const FunctionName& Operator::getFunctionName() const {
    U_ASSERT(!isReserved());
    return functionName;
}

UChar FunctionName::sigilChar() const {
    switch (functionSigil) {
    case Sigil::OPEN: { return PLUS; }
    case Sigil::CLOSE: { return HYPHEN; }
    default: { return COLON; }
    }
}

UnicodeString FunctionName::toString() const {
    UnicodeString result;
    result += sigilChar();
    result += functionName;
    return result;
}

FunctionName::~FunctionName() {}

FunctionName& FunctionName::operator=(FunctionName&& other) noexcept {
    this->~FunctionName();

    functionName = other.functionName;
    functionSigil = other.functionSigil;

    return *this;
}

bool FunctionName::operator<(const FunctionName& other) const {
    // If sigils are different, arbitrarily order open < close < default
    switch (other.functionSigil) {
    case OPEN: {
        if (functionSigil != OPEN) {
            return false;
        }
        break;
    }
    case CLOSE: {
        if (functionSigil == OPEN) {
            return true;
        }
        if (functionSigil == DEFAULT) {
            return false;
            break;
        }
        break;
    }
    case DEFAULT: {
        if (functionSigil != DEFAULT) {
            return true;
        }
        break;
    }
    }

    // Sigils are equal; compare names
    return (functionName < other.functionName);
}

const Reserved& Operator::asReserved() const {
    U_ASSERT(isReserved());
    return reserved;
}

const OptionMap& Operator::getOptions() const {
    U_ASSERT(!isReserved());
    return options;
}

Operator::Builder& Operator::Builder::setReserved(Reserved&& reserved) {
    isReservedSequence = true;
    hasFunctionName = false;
    asReserved = std::move(reserved);
    return *this;
}

Operator::Builder& Operator::Builder::setFunctionName(FunctionName&& func) {
    isReservedSequence = false;
    hasFunctionName = true;
    functionName = std::move(func);
    return *this;
}

Operator::Builder& Operator::Builder::addOption(const UnicodeString &key, Operand&& value, UErrorCode& errorCode) noexcept {
    THIS_ON_ERROR(errorCode);

    isReservedSequence = false;
    hasOptions = true;
    // If the option name is already in the map, emit a data model error
    if (options.has(key)) {
        errorCode = U_DUPLICATE_OPTION_NAME_ERROR;
    } else {
        options.add(key, std::move(value));
    }
    return *this;
}

Operator Operator::Builder::build(UErrorCode& errorCode) const noexcept {
    Operator result;
    if (U_FAILURE(errorCode)) {
        return result;
    }
    // Must be either reserved or function, not both; enforced by methods
    if (isReservedSequence) {
        // Methods enforce that the function name and options are unset
        // if `setReserved()` is called, so if they were valid, that
        // would indicate a bug.
        U_ASSERT(!hasOptions && !hasFunctionName);
        result = Operator(asReserved);
    } else {
        if (!hasFunctionName) {
            // Neither function name nor reserved was set
            // There is no default, so this case could occur if the
            // caller creates a builder and doesn't make any calls
            // before calling build().
            errorCode = U_INVALID_STATE_ERROR;
            return result;
        }
        result = Operator(functionName, options.build());
    }
    return result;
}

Operator::Operator(const Operator& other) noexcept : isReservedSequence(other.isReservedSequence),
                                            functionName(other.functionName),
                                            options(isReservedSequence ? OptionMap()
                                                    : OptionMap(other.options)),
                                            reserved(isReservedSequence ? Reserved(other.reserved)
                                                     : Reserved()) {}

Operator& Operator::operator=(Operator&& other) noexcept {
    this->~Operator();

    isReservedSequence = other.isReservedSequence;
    if (!other.isReservedSequence) {
        functionName = std::move(other.functionName);
        options = std::move(other.options);
    } else {
        reserved = std::move(other.reserved);
    }
    return *this;
}

Operator& Operator::operator=(const Operator& other) noexcept {
    if (this != &other) {
        isReservedSequence = other.isReservedSequence;
        if (!other.isReservedSequence) {
            functionName = other.functionName;
            options = other.options;
        } else {
            reserved = other.reserved;
        }
    }
    return *this;
}

// Function call constructor
Operator::Operator(const FunctionName& f, OptionMap&& l) : isReservedSequence(false), functionName(f), options(l) {}

Operator::Builder::~Builder() {}

Operator::~Operator() {}

// ------------ Expression


UBool Expression::isStandaloneAnnotation() const {
    return rand.isNull();
}

// Returns true for function calls with operands as well as
// standalone annotations.
// Reserved sequences are not function calls
UBool Expression::isFunctionCall() const {
    return (hasOperator && !rator.isReserved());
}

UBool Expression::isReserved() const {
    return (hasOperator && rator.isReserved());
}

const Operator& Expression::getOperator() const {
    U_ASSERT(hasOperator);
    return rator;
}

// May return null operand
const Operand& Expression::getOperand() const { return rand; }

Expression::Builder& Expression::Builder::setOperand(Operand&& rAnd) {
    hasOperand = true;
    rand = std::move(rAnd);
    return *this;
}

Expression::Builder& Expression::Builder::setOperator(Operator&& rAtor) {
    hasOperator = true;
    rator = std::move(rAtor);
    return *this;
}

Expression Expression::Builder::build(UErrorCode& errorCode) const {
    Expression result;

    if (U_FAILURE(errorCode)) {
        return result;
    }

    if ((!hasOperand || rand.isNull()) && !hasOperator) {
        errorCode = U_INVALID_STATE_ERROR;
        return result;
    }

    if (hasOperand && hasOperator) {
        result = Expression(rator, rand);
    } else if (hasOperand && !hasOperator) {
        result = Expression(rand);
    } else {
        // rator is valid, rand is not valid
        result = Expression(rator);
    }
    return result;
}

Expression::Expression() : hasOperator(false) {}

Expression::Expression(const Expression& other) : hasOperator(other.hasOperator), rator(other.rator), rand(other.rand) {}

Expression& Expression::operator=(Expression&& other) noexcept {
    this->~Expression();

    hasOperator = other.hasOperator;
    if (other.hasOperator) {
        rator = std::move(other.rator);
    }
    rand = std::move(other.rand);

    return *this;
}

Expression& Expression::operator=(const Expression& other) {
    if (this != &other) {
        hasOperator = other.hasOperator;
        if (other.hasOperator) {
            rator = other.rator;
        }
        rand = other.rand;
    }
    return *this;
}

Expression::Builder::~Builder() {}

// ----------- PatternPart

// PatternPart needs a copy constructor in order to make Pattern deeply copyable
// If !isRawText and the copy of the other expression fails,
// then isBogus() will be true for this PatternPart
PatternPart::PatternPart(const PatternPart& other) : isRawText(other.isText()), text(other.text), expression(other.expression) {}

const Expression& PatternPart::contents() const {
    U_ASSERT(!isText());
    return expression;
}

// Precondition: isText();
const UnicodeString& PatternPart::asText() const {
    U_ASSERT(isText());
    return text;
}

PatternPart& PatternPart::operator=(PatternPart&& other) noexcept {
    this->~PatternPart();

    isRawText = other.isRawText;
    text = other.text;
    if (!isRawText) {
        expression = std::move(other.expression);
    }
    return *this;
}

PatternPart& PatternPart::operator=(const PatternPart& other) {
    if (this != &other) {
        this->~PatternPart();

        isRawText = other.isRawText;
        text = other.text;
        if (!isRawText) {
            expression = other.expression;
        }
    }
    return *this;
}

PatternPart::~PatternPart() {}

// ---------------- Pattern

Pattern::Pattern(const std::vector<PatternPart>& ps) : parts(ps) {}

// Copy constructor
// If the copy of the other list fails,
// then isBogus() will be true for this Pattern
Pattern::Pattern(const Pattern& other) noexcept : parts(other.parts) {}

const PatternPart& Pattern::getPart(int32_t i) const {
    U_ASSERT(i < numParts());
    return parts[i];
}

Pattern Pattern::Builder::build() const noexcept {
    return Pattern(parts);
}

Pattern::Builder& Pattern::Builder::add(const PatternPart& part) noexcept {
    parts.push_back(part);
    return *this;
}

Pattern& Pattern::operator=(Pattern&& other) noexcept {
    parts = std::move(other.parts);

    return *this;
}

Pattern& Pattern::operator=(const Pattern& other) noexcept {
    if (this != &other) {
        parts = other.parts;
    }
    return *this;
}


Pattern::Builder::~Builder() {}

// ---------------- Binding

const Expression& Binding::getValue() const { return value; }

Binding::Binding(const Binding& other) : var(other.var), value(other.value) {}

Binding& Binding::operator=(const Binding& other) {
    if (this != &other) {
        var = other.var;
        value = other.value;
    }

    return *this;
}

Binding::~Binding() {}

// --------------- Variant

/* static */ Variant* Variant::create(SelectorKeys&& s, Pattern&& p, UErrorCode& status) {
    NULL_ON_ERROR(status);
    Variant* result = new Variant(std::move(s), std::move(p));
    if (result == nullptr) {
        status = U_MEMORY_ALLOCATION_ERROR;
    }
    return result;
}

Variant& Variant::operator=(const Variant& other) {
    if (this != &other) {
        k = other.k;
        p = other.p;
    }
    return *this;
}

Variant::~Variant() {}

// --------------- MessageFormatDataModel

const Bindings& MessageFormatDataModel::getLocalVariables() const {
    U_ASSERT(!bogus);

    return bindings;
}

// The `hasSelectors()` method is provided so that `getSelectors()`,
// `getVariants()` and `getPattern()` can rely on preconditions
// rather than taking error codes as arguments.
UBool MessageFormatDataModel::hasSelectors() const {
    U_ASSERT(!bogus);
    return !hasPattern();
}

const ExpressionList& MessageFormatDataModel::getSelectors() const {
    U_ASSERT(hasSelectors());
    return selectors;
}

const Pattern& MessageFormatDataModel::getPattern() const {
    U_ASSERT(!hasSelectors());
    return pattern;
}

MessageFormatDataModel::Builder::Builder() {}

// Invalidate pattern and create selectors/variants if necessary
void MessageFormatDataModel::Builder::buildSelectorsMessage(UErrorCode& status) {
    CHECK_ERROR(status);

    if (hasPattern) {
        selectors = ExpressionList();
        variants = new UVector(status);
        if (U_FAILURE(status)) {
            variants = nullptr;
        } else {
            variants->setDeleter(uprv_deleteUObject);
        }
        hasPattern = false;
    }
    hasPattern = false;
    hasSelectors = true;
}

MessageFormatDataModel::Builder& MessageFormatDataModel::Builder::addLocalVariable(VariableName&& variableName,
                                                                                   Expression&& expression) noexcept {
    locals.push_back(Binding(std::move(variableName), std::move(expression)));

    return *this;
}

/*
  selector must be non-null
*/
MessageFormatDataModel::Builder& MessageFormatDataModel::Builder::addSelector(Expression&& selector, UErrorCode& status) noexcept {
    THIS_ON_ERROR(status);

    buildSelectorsMessage(status);
    selectors.push_back(std::move(selector));

    return *this;
}

/*
  `pattern` must be non-null
*/
MessageFormatDataModel::Builder& MessageFormatDataModel::Builder::addVariant(SelectorKeys&& keys, Pattern&& pattern, UErrorCode& errorCode) noexcept {
    buildSelectorsMessage(errorCode);
    Variant* v = Variant::create(std::move(keys), std::move(pattern), errorCode);
    if (U_SUCCESS(errorCode)) {
        variants->adoptElement(v, errorCode);
    }
    return *this;
}

MessageFormatDataModel::Builder& MessageFormatDataModel::Builder::setPattern(Pattern&& pat) {
    pattern = std::move(pat);
    hasPattern = true;
    hasSelectors = false;
    // Invalidate variants
    if (variants != nullptr) {
        variants->removeAllElements();
    }
    return *this;
}

MessageFormatDataModel::MessageFormatDataModel(const MessageFormatDataModel::Builder& builder, UErrorCode& errorCode) noexcept {
    CHECK_ERROR(errorCode);

    numVariants = builder.variants == nullptr ? 0 : builder.variants->size();
    Variant* variantArray = nullptr;
    if (!hasPattern()) {
        selectors = ExpressionList(builder.selectors);
        variantArray = new Variant[numVariants];
        if (variantArray == nullptr) {
            errorCode = U_MEMORY_ALLOCATION_ERROR;
            return;
        }
        for (int32_t i = 0; i < numVariants; i++) {
            variantArray[i] = *(static_cast<Variant*>(builder.variants->elementAt(i)));
        }
        pattern = Pattern();
    } else {
        selectors = ExpressionList();
        pattern = builder.pattern;
    }
    variants.adoptInstead(variantArray);
    bindings = Bindings(builder.locals);
}

MessageFormatDataModel::MessageFormatDataModel(MessageFormatDataModel&& other) noexcept
    : numVariants(other.numVariants),
      selectors(hasPattern() ? ExpressionList() : ExpressionList(other.selectors)),
      variants(hasPattern() ? LocalArray<Variant>() : LocalArray<Variant>(other.variants.orphan())),
      pattern(hasPattern() ? other.pattern : Pattern()),
      bindings(other.bindings) {}

MessageFormatDataModel::MessageFormatDataModel() : numVariants(0) {}

MessageFormatDataModel& MessageFormatDataModel::operator=(MessageFormatDataModel&& other) noexcept {
    U_ASSERT(!other.bogus);
    numVariants = other.numVariants;
    if (hasPattern()) {
        pattern = std::move(other.pattern);
    } else {
        selectors = std::move(other.selectors);
        variants = std::move(other.variants);
    }
    bindings = std::move(other.bindings);
    return *this;
}

MessageFormatDataModel& MessageFormatDataModel::operator=(const MessageFormatDataModel& other) noexcept {
    if (this != &other) {
        U_ASSERT(!other.bogus);

        this->~MessageFormatDataModel();

        numVariants = other.numVariants;
        if (hasPattern()) {
            pattern = other.pattern;
        } else {
            selectors = other.selectors;
            Variant* variantArray = new Variant[numVariants];
            if (variantArray == nullptr) {
                bogus = true;
                return *this;
            }
            for (int32_t i = 0; i < numVariants; i++) {
                variantArray[i] = other.variants[i];
            }
            variants.adoptInstead(variantArray);
        }
        bindings = other.bindings;
    }

    return *this;
}

MessageFormatDataModel MessageFormatDataModel::Builder::build(UErrorCode& errorCode) const noexcept {
    MessageFormatDataModel result;
    if (U_FAILURE(errorCode)) {
        return result;
    }
    if (!hasPattern && !hasSelectors) {
        errorCode = U_INVALID_STATE_ERROR;
    }
    return MessageFormatDataModel(*this, errorCode);
}

MessageFormatDataModel::~MessageFormatDataModel() {}
MessageFormatDataModel::Builder::~Builder() {
    if (variants != nullptr) {
        delete variants;
    }
}

template<>
OrderedMap<SelectorKeys, Pattern>::Builder::~Builder() {}
template<>
OrderedMap<SelectorKeys, Pattern>::~OrderedMap() {}
template<>
OrderedMap<UnicodeString, Operand>::Builder::~Builder() {}
template<>
OrderedMap<UnicodeString, Operand>::~OrderedMap() {}

} // namespace message2

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */
