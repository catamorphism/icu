// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/messageformat2_data_model.h"
#include "unicode/messageformat2.h"
#include "messageformat2_macros.h"
#include "hash.h"
#include "uvector.h" // U_ASSERT

#if U_PF_WINDOWS <= U_PLATFORM && U_PLATFORM <= U_PF_CYGWIN && defined(_MSC_VER)
// Ignore warning 4661 as LocalPointerBase does not use operator== or operator!=
#pragma warning(disable: 4661)
#endif

U_NAMESPACE_BEGIN namespace message2 {

// Implementation

//------------------ SelectorKeys

SelectorKeys::Builder& SelectorKeys::Builder::add(const Key& key) {
    keys.push_back(key);
    return *this;
}

const KeyList& SelectorKeys::getKeys() const {
    return keys;
}

SelectorKeys::Builder::Builder() : keys(std::vector<Key>()) {}

SelectorKeys::SelectorKeys() : keys(KeyList()) {}

SelectorKeys::SelectorKeys(const SelectorKeys& other) : keys(KeyList(other.keys)) {}

SelectorKeys::Builder::~Builder() {}

SelectorKeys::~SelectorKeys() {}

//------------------ VariableName

UnicodeString VariableName::declaration() const {
    UnicodeString result(DOLLAR);
    result += variableName;
    return result;
}

VariableName::~VariableName() {}

//------------------ Literal

UnicodeString Literal::quotedString() const {
    UnicodeString result(PIPE);
    result += stringContents();
    result += PIPE;
    return result;
}

const UnicodeString& Literal::stringContents() const {
    U_ASSERT(contents.getType() == Formattable::Type::kString);
    return contents.getString();
}

Literal& Literal::operator=(Literal&& other) noexcept {
    this->~Literal();

    isQuoted = other.isQuoted;
    U_ASSERT(other.contents.getType() == Formattable::Type::kString);
    contents = std::move(other.contents);

    return *this;
}

Literal::Literal(Literal&& other) noexcept {
    isQuoted = other.isQuoted;
    U_ASSERT(other.contents.getType() == Formattable::Type::kString);
    contents = std::move(other.contents);
}


Literal::~Literal() {}

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

Key& Key::operator=(Key&& other) noexcept {
    this->~Key();

    wildcard = other.wildcard;
    if (!other.wildcard) {
        contents = std::move(other.contents);
    }
    return *this;
}

UnicodeString Key::toString() const {
    if (isWildcard()) {
        return UnicodeString(ASTERISK);
    }
    return contents.stringContents();
}

const Literal& Key::asLiteral() const {
    U_ASSERT(!isWildcard());
    return contents;
}

//---------------- VariantMap

int32_t VariantMap::size() const {
    return contents->size();
}

// TODO
// k is copied
UBool VariantMap::next(int32_t &pos, SelectorKeys& k, const Pattern*& v) const {
    UnicodeString unused;
    if (!contents->next(pos, unused, v)) {
        return false;
    }
    k = keyLists[pos - 1];
    return true;
}

VariantMap::Builder& VariantMap::Builder::add(SelectorKeys&& key, Pattern&& value, UErrorCode& errorCode) {
    THIS_ON_ERROR(errorCode);
    // Stringify `key`
    UnicodeString keyResult;
    concatenateKeys(key, keyResult);
    contents->add(keyResult, std::move(value), errorCode);
    keyLists.push_back(key);
    return *this;
}

VariantMap* VariantMap::Builder::build(UErrorCode& errorCode) const {
    NULL_ON_ERROR(errorCode);

    LocalPointer<OrderedMap<Pattern>> adoptedContents(contents->build(errorCode));
    NULL_ON_ERROR(errorCode);
    VariantMap* result = new VariantMap(adoptedContents.orphan(), keyLists);
    if (result == nullptr) {
        errorCode = U_MEMORY_ALLOCATION_ERROR;
    }
    return result;
}

/* static */ void VariantMap::Builder::concatenateKeys(const SelectorKeys& keys, UnicodeString& result) {
    const KeyList& ks = keys.getKeys();
    int32_t len = ks.size();
    for (int32_t i = 0; i < len; i++) {
        result += ks[i].toString();
        if (i != len - 1) {
            result += SPACE;
        }
    }
}

VariantMap::Builder::Builder(UErrorCode& errorCode) {
    // initialize `contents`
    // No value comparator needed
    contents.adoptInstead(OrderedMap<Pattern>::builder(errorCode));
    // initialize `keyLists`
    keyLists = std::vector<SelectorKeys>();
}

/* static */ VariantMap::Builder* VariantMap::builder(UErrorCode& errorCode) {
    NULL_ON_ERROR(errorCode);
    LocalPointer<VariantMap::Builder> result(new VariantMap::Builder(errorCode));
    NULL_ON_ERROR(errorCode);
    return result.orphan();
}

VariantMap::VariantMap(OrderedMap<Pattern>* vs, const std::vector<SelectorKeys>& ks) : contents(vs), keyLists(std::vector<SelectorKeys>(ks)) {
    // Check invariant: `vs` and `ks` have the same size
    U_ASSERT(vs->size() == (int32_t) ks.size());
}

VariantMap::Builder::~Builder() {}

// ------------ Reserved

// Copy constructor
Reserved::Reserved(const Reserved& other) : parts(other.parts) {}

Reserved::Reserved(const std::vector<Literal>& ps) :  parts(ps) {}

int32_t Reserved::numParts() const {
    return parts.size();
}

const Literal& Reserved::getPart(int32_t i) const {
    U_ASSERT(i < numParts());
    return parts[i];
}

Reserved::Builder::Builder() : parts(std::vector<Literal>()) {}

Reserved Reserved::Builder::build() const {
    return Reserved(parts);
}

Reserved::Builder& Reserved::Builder::add(const Literal& part) {
    parts.push_back(part);
    return *this;
}

Reserved& Reserved::operator=(Reserved&& other) noexcept {
    this->~Reserved();

    parts = std::move(other.parts);

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

const Reserved& Operator::asReserved() const {
    U_ASSERT(isReserved());
    return reserved;
}

const OptionMap& Operator::getOptions() const {
    U_ASSERT(!isReserved());
    return *options;
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

Operator::Builder& Operator::Builder::addOption(const UnicodeString &key, Operand&& value, UErrorCode& errorCode) {
    THIS_ON_ERROR(errorCode);

    isReservedSequence = false;
    if (!options.isValid()) {
        options.adoptInstead(OptionMap::builder(errorCode));
        THIS_ON_ERROR(errorCode);
    }
    // If the option name is already in the map, emit a data model error
    if (options->has(key)) {
        errorCode = U_DUPLICATE_OPTION_NAME_ERROR;
    } else {
        options->add(key, std::move(value), errorCode);
    }
    return *this;
}

Operator Operator::Builder::build(UErrorCode& errorCode) const {
    Operator result;
    if (U_FAILURE(errorCode)) {
        return result;
    }
    // Must be either reserved or function, not both; enforced by methods
    if (isReservedSequence) {
        // Methods enforce that the function name and options are unset
        // if `setReserved()` is called, so if they were valid, that
        // would indicate a bug.
        U_ASSERT(!options.isValid() && !hasFunctionName);
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
        if (options.isValid()) {
            LocalPointer<OptionMap> opts(options->build(errorCode));
            if (U_FAILURE(errorCode)) {
                return result;
            }
            result = Operator(functionName, opts.orphan());
        } else {
            // If option were never added, create a new empty options map
            LocalPointer<OptionMap::Builder> optsBuilder(OptionMap::builder(errorCode));
            if (U_SUCCESS(errorCode)) {
                LocalPointer<OptionMap> opts(optsBuilder->build(errorCode));
                if (U_SUCCESS(errorCode)) {
                    result = Operator(functionName, opts.orphan());
                }
            }
        }
    }
    return result;
}

Operator::Operator(const Operator& other) : isReservedSequence(other.isReservedSequence),
                                            functionName(other.functionName),
                                            options(isReservedSequence ? nullptr
                                                    : new OptionMap(*other.options)),
                                            reserved(isReservedSequence? Reserved(other.reserved)
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

// Function call constructor; adopts `f` and `l`, which must be non-null
Operator::Operator(const FunctionName& f, OptionMap *l) : isReservedSequence(false), functionName(f), options(l) {
    U_ASSERT(l != nullptr);
 }

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

PatternPart::~PatternPart() {}

// ---------------- Pattern

Pattern::Pattern(const std::vector<PatternPart>& ps) : parts(ps) {}

// Copy constructor
// If the copy of the other list fails,
// then isBogus() will be true for this Pattern
Pattern::Pattern(const Pattern& other) : parts(other.parts) {}

const PatternPart& Pattern::getPart(int32_t i) const {
    U_ASSERT(i < numParts());
    return parts[i];
}

Pattern Pattern::Builder::build() const {
    return Pattern(parts);
}

Pattern::Builder& Pattern::Builder::add(const PatternPart& part) {
    parts.push_back(part);
    return *this;
}

Pattern& Pattern::operator=(Pattern&& other) noexcept {
    this->~Pattern();

    parts = std::move(other.parts);

    return *this;
}

Pattern::Builder::~Builder() {}

// ---------------- Binding

const Expression& Binding::getValue() const { return value; }

Binding::Binding(const Binding& other) : var(other.var), value(other.value) {}

Binding::~Binding() {}

// --------------- MessageFormatDataModel

const Bindings& MessageFormatDataModel::getLocalVariables() const {
    return bindings;
}

// The `hasSelectors()` method is provided so that `getSelectors()`,
// `getVariants()` and `getPattern()` can rely on preconditions
// rather than taking error codes as arguments.
UBool MessageFormatDataModel::hasSelectors() const {
    if (hasPattern) {
        U_ASSERT(!variants.isValid());
        return false;
    }
    U_ASSERT(variants.isValid());
    return true;
}

const ExpressionList& MessageFormatDataModel::getSelectors() const {
    U_ASSERT(hasSelectors());
    return selectors;
}

const VariantMap& MessageFormatDataModel::getVariants() const {
    U_ASSERT(hasSelectors());
    return *variants;
}

const Pattern& MessageFormatDataModel::getPattern() const {
    U_ASSERT(!hasSelectors());
    return pattern;
}

MessageFormatDataModel::Builder::Builder(UErrorCode& errorCode) {
    CHECK_ERROR(errorCode);
    variants.adoptInstead(VariantMap::builder(errorCode));
}

// Invalidate pattern and create selectors/variants if necessary
void MessageFormatDataModel::Builder::buildSelectorsMessage(UErrorCode& errorCode) {
    CHECK_ERROR(errorCode);

    if (hasPattern) {
        selectors = ExpressionList();
        variants.adoptInstead(VariantMap::builder(errorCode));
        hasPattern = false;
    } else {
        U_ASSERT(variants.isValid());
    }
    hasPattern = false;
    hasSelectors = true;
}

MessageFormatDataModel::Builder& MessageFormatDataModel::Builder::addLocalVariable(const VariableName&variableName, const Expression& expression) {
    locals.push_back(Binding(variableName, expression));

    return *this;
}

/*
  selector must be non-null
*/
MessageFormatDataModel::Builder& MessageFormatDataModel::Builder::addSelector(const Expression& selector, UErrorCode& errorCode) {
    THIS_ON_ERROR(errorCode);

    buildSelectorsMessage(errorCode);
    selectors.push_back(selector);

    return *this;
}

/*
  `pattern` must be non-null
*/
MessageFormatDataModel::Builder& MessageFormatDataModel::Builder::addVariant(SelectorKeys&& keys, Pattern&& pattern, UErrorCode& errorCode) {
    THIS_ON_ERROR(errorCode);

    buildSelectorsMessage(errorCode);
    variants->add(std::move(keys), std::move(pattern), errorCode);

    return *this;
}

MessageFormatDataModel::Builder* MessageFormatDataModel::builder(UErrorCode& errorCode) {
    NULL_ON_ERROR(errorCode);
    LocalPointer<Builder> result(new Builder(errorCode));
    if (U_FAILURE(errorCode)) {
        return nullptr;
    }
    return result.orphan();
}

MessageFormatDataModel::Builder& MessageFormatDataModel::Builder::setPattern(Pattern&& pat) {
    pattern = std::move(pat);
    hasPattern = true;
    hasSelectors = false;
    // Invalidate variants
    variants.adoptInstead(nullptr);
    return *this;
}

MessageFormatDataModel::MessageFormatDataModel(const MessageFormatDataModel::Builder& builder, UErrorCode &errorCode)
    : hasPattern(builder.hasPattern),
      selectors(hasPattern ? ExpressionList() : ExpressionList(builder.selectors)),
      variants(hasPattern ? nullptr : builder.variants->build(errorCode)),
      pattern(hasPattern ? builder.pattern : Pattern()),
      bindings(Bindings(builder.locals)) {}

MessageFormatDataModel* MessageFormatDataModel::Builder::build(UErrorCode &errorCode) const {
    NULL_ON_ERROR(errorCode);

    // Initialize the data model
    LocalPointer<MessageFormatDataModel> dataModel(new MessageFormatDataModel(*this, errorCode));
    NULL_ON_ERROR(errorCode);
    return dataModel.orphan();
}

MessageFormatDataModel::~MessageFormatDataModel() {}
template<>
ImmutableVector<Binding>::Builder::~Builder() {}
template<>
ImmutableVector<Binding>::~ImmutableVector() {}
template<>
ImmutableVector<Expression>::Builder::~Builder() {}
template<>
ImmutableVector<Expression>::~ImmutableVector() {}
template<>
ImmutableVector<Key>::Builder::~Builder() {}
template<>
ImmutableVector<Key>::~ImmutableVector() {}
template<>
ImmutableVector<Literal>::Builder::~Builder() {}
template<>
ImmutableVector<Literal>::~ImmutableVector() {}
template<>
ImmutableVector<PatternPart>::Builder::~Builder() {}
template<>
ImmutableVector<PatternPart>::~ImmutableVector() {}
template<>
ImmutableVector<SelectorKeys>::Builder::~Builder() {}
template<>
ImmutableVector<SelectorKeys>::~ImmutableVector() {}
template<>
OrderedMap<Pattern>::Builder::~Builder() {}
template<>
OrderedMap<Pattern>::~OrderedMap() {}
template<>
OrderedMap<Operand>::Builder::~Builder() {}
template<>
OrderedMap<Operand>::~OrderedMap() {}

} // namespace message2

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */

