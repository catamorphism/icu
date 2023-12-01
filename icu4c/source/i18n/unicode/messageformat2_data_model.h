// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#ifndef U_HIDE_DEPRECATED_API

#ifndef MESSAGEFORMAT_DATA_MODEL_H
#define MESSAGEFORMAT_DATA_MODEL_H

#include "unicode/utypes.h"

#if U_SHOW_CPLUSPLUS_API

#if !UCONFIG_NO_FORMATTING

#include "unicode/fmtable.h"
#include "unicode/messageformat2_utils.h"
#include "unicode/unistr.h"

U_NAMESPACE_BEGIN

namespace message2 {

class ExpressionContext;
class MessageFormatDataModel;

namespace data_model {

    /**
     * The `VariableName` class represents the name of a variable in a message.
     *
     * `VariableName` is immutable, copyable and movable.
     *
     * @internal ICU 75.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    class U_I18N_API VariableName : public UObject {
    public:
        /**
         * Equality comparison.
         *
         * @param other    the object to be compared with.
         * @return        true if other is equal to this, false otherwise.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        inline bool operator== (const VariableName& other) const { return other.variableName == variableName; }
        /**
         * Constructor.
         *
         * @param s   The variable name, as a string
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        VariableName(const UnicodeString& s) : variableName(s) {}
        /**
         * Default constructor. (Needed for representing null operands)
         *
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        VariableName() {}
        /**
         * Returns the name of this variable, as a string.
         *
         * @return        Reference to the variable's name
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        const UnicodeString& identifier() const { return variableName; }
        /**
         * Returns the name of this variable, as a string prefixed by the
         * variable name sigil ('$')
         *
         * @return        String representation of the variable as it appears in a declaration
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
         UnicodeString declaration() const;
         /**
          * Destructor.
          *
          * @internal ICU 75.0 technology preview
          * @deprecated This API is for technology preview only.
          */
         virtual ~VariableName();

         // TODO
         VariableName& operator=(VariableName&&) = default;
         VariableName& operator=(const VariableName&) = default;
         VariableName(const VariableName&) = default;
    private:
        /* const */ UnicodeString variableName;
    }; // class VariableName


    /**
     * The `FunctionName` class represents the name of a function referred to
     * in a message.
     *
     * It corresponds to the `FunctionRef` interface defined in
     * https://github.com/unicode-org/message-format-wg/blob/main/spec/data-model.md#expressions
     *
     * `FunctionName` is immutable, copyable and movable.
     *
     * @internal ICU 75.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    class U_I18N_API FunctionName : public UMemory {
    public:
        /**
         * Type representing the function's kind, which is either ':' (the default)
         * or "open" ('+')/"close" ('-'), usually used for markup functions.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
         enum Sigil {
             OPEN,
             CLOSE,
             DEFAULT
         };
         /**
          * Converts the function name to a string that includes the sigil.
          *
          * @return A string beginning with the sigil, followed by the function's name.
          *
          * @internal ICU 75.0 technology preview
          * @deprecated This API is for technology preview only.
          */
         UnicodeString toString() const;
         /**
          * Constructor.
          *
          * @param s   The function name, as a string. Constructs a function name with the default sigil.
          *
          * @internal ICU 75.0 technology preview
          * @deprecated This API is for technology preview only.
          */
         FunctionName(UnicodeString s) : functionName(s), functionSigil(Sigil::DEFAULT) {}
         /**
          * Constructor.
          *
          * @param n   The function name, as a string.
          * @param s   The function sigil to use.
          *
          * @internal ICU 75.0 technology preview
          * @deprecated This API is for technology preview only.
          */
         FunctionName(UnicodeString n, Sigil s) : functionName(n), functionSigil(s) {}
         /**
          * Destructor.
          *
          * @internal ICU 75.0 technology preview
          * @deprecated This API is for technology preview only.
          */
         virtual ~FunctionName();

// TODO
         /**
          * Copy constructor.
          *
          * @param other   The function name to copy.
          *
          * @internal ICU 75.0 technology preview
          * @deprecated This API is for technology preview only.
          */
         FunctionName(const FunctionName& other) : functionName(other.functionName), functionSigil(other.functionSigil) {}
         FunctionName& operator=(FunctionName&& other) noexcept;
         FunctionName& operator=(const FunctionName&) = default;
         FunctionName() : functionSigil(Sigil::DEFAULT) {}
         // TODO
         // Defined so FunctionNames can be compared and used as keys in a map by the FunctionRegistry
         bool operator<(const FunctionName&) const;
         bool operator==(const FunctionName&) const;

    private:
         friend class Operator;
         friend class icu::message2::ExpressionContext;

         /* const */ UnicodeString functionName;
         /* const */ Sigil functionSigil;

         bool isEmpty() { return functionName.length() == 0; }

         UChar sigilChar() const;
    }; // class FunctionName

    /**
     * The `Literal` class corresponds to the `literal` nonterminal in the MessageFormat 2 grammar,
     * https://github.com/unicode-org/message-format-wg/blob/main/spec/message.abnf and the
     * `Literal` interface defined in
     *   // https://github.com/unicode-org/message-format-wg/blob/main/spec/data-model.md#expressions
     *
     * `Literal` is immutable, copyable and movable.
     *
     * @internal ICU 75.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    class U_I18N_API Literal : public UObject {
    public:
        /**
         * Returns the quoted representation of this literal (enclosed in '|' characters)
         *
         * @return A string representation of the literal enclosed in quote characters.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
         UnicodeString quotedString() const;
         /**
          * Returns the parsed string contents of this literal.
          *
          * @return A reference to a Formattable whose string contents are
          *         the parsed string contents of this literal.
          *
          * @internal ICU 75.0 technology preview
          * @deprecated This API is for technology preview only.
          */
         const Formattable& getContents() const { return contents; }
         /**
          * Returns the parsed string contents of this literal.
          *
          * @return A string representation of this literal.
          *
          * @internal ICU 75.0 technology preview
          * @deprecated This API is for technology preview only.
          */
         const UnicodeString& stringContents() const;
         /**
          * Determines if this literal appeared as a quoted literal in the message.
          *
          * @return true if and only if this literal appeared as a quoted literal in the
          *         message.
          *
          * @internal ICU 75.0 technology preview
          * @deprecated This API is for technology preview only.
          */
         UBool quoted() const { return isQuoted; }
         /**
          * Literal constructor.
          *
          *  @param q True if and only if this literal was parsed with the `quoted` nonterminal
          *           (appeared enclosed in '|' characters in the message text).
          *  @param s The string contents of this literal; escape sequences are assumed to have
          *           been interpreted already.
          *
          * @internal ICU 75.0 technology preview
          * @deprecated This API is for technology preview only.
          */
         Literal(UBool q, const UnicodeString& s) : isQuoted(q), contents(s) {}
// TODO
        Literal(const Literal& other) : isQuoted(other.isQuoted), contents(other.contents) {}
        Literal(Literal&& other) noexcept;
        Literal& operator=(Literal&&) noexcept;
        Literal& operator=(const Literal&);
        // Because Key uses `Literal` as its underlying representation,
        // this provides a default constructor for wildcard keys
        Literal() : isQuoted(false), contents(Formattable("")) {}
        // TODO
        // Defined so Keys can be compared
        bool operator<(const Literal& rhs) const;
        bool operator==(const Literal& rhs) const;

         /**
          * Destructor.
          *
          * @internal ICU 75.0 technology preview
          * @deprecated This API is for technology preview only.
          */
         virtual ~Literal();

    private:
        /* const */ bool isQuoted = false;
        // Contents is stored as a Formattable to avoid allocating
        // new Formattables during formatting, but it's guaranteed
        // to be a string
        /* const */ Formattable contents;
    };

    /**
     * The `Operand` class corresponds to the `operand` nonterminal in the MessageFormat 2 grammar,
     * https://github.com/unicode-org/message-format-wg/blob/main/spec/message.abnf .
     * It represents a `Literal | VariableRef` -- see the `operand?` field of the `FunctionRef`
     * interface defined at:
     * https://github.com/unicode-org/message-format-wg/blob/main/spec/data-model.md#expressions
     * with the difference that it can also represent a null operand (the absent operand in an
     * `annotation` with no operand).
     *
     * `Operand` is immutable and is copyable and movable.
     *
     * @internal ICU 75.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    class U_I18N_API Operand : public UObject {
    public:
        /**
         * Determines if this operand represents a variable.
         *
         * @return True if and only if the operand is a variable.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        UBool isVariable() const;
        /**
         * Determines if this operand represents a literal.
         *
         * @return True if and only if the operand is a literal.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        UBool isLiteral() const;
        /**
         * Determines if this operand is the null operand.
         *
         * @return True if and only if the operand is the null operand.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        UBool isNull() const;
        /**
         * Returns a reference to this operand's variable name.
         * Precondition: isVariable()
         *
         * @return A reference to the name of the variable
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        const VariableName& asVariable() const;
        /**
         * Returns a reference to this operand's literal contents.
         * Precondition: isLiteral()
         *
         * @return A reference to the literal
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        const Literal& asLiteral() const;
        // TODO
        Operand() : type(Type::NULL_OPERAND) {}
        Operand(const VariableName& v) : var(v), type(Type::VARIABLE) {}
        Operand(const Literal& l) : lit(l), type(Type::LITERAL) {}
        Operand(const Operand&);
        Operand& operator=(Operand&&) noexcept;
        Operand& operator=(const Operand&);

        /**
         * Destructor.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        virtual ~Operand();
    private:

        enum Type {
            VARIABLE,
            LITERAL,
            NULL_OPERAND
        };
        // This wastes some space, but it's simpler than defining a copy
        // constructor for a union
        // Should be const, but declared non-const to make the move assignment operator work
        VariableName var;
        Literal lit;
        Type type;

    }; // class Operand

    /**
     * The `Key` class corresponds to the `key` nonterminal in the MessageFormat 2 grammar,
     * https://github.com/unicode-org/message-format-wg/blob/main/spec/message.abnf .
     * It also corresponds to
     * the `Literal | CatchallKey` that is the
     * element type of the `keys` array in the `Variant` interface
     * defined in https://github.com/unicode-org/message-format-wg/blob/main/spec/data-model.md#messages
     *
     * A key is either a literal or the wildcard symbol (represented in messages as '*')
     *
     * `Key` is immutable, copyable and movable.
     *
     * @internal ICU 75.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    class U_I18N_API Key : public UObject {
    public:
        /**
        * Determines if this is a wildcard key
        *
        * @return True if and only if this is the wildcard key
        *
        * @internal ICU 75.0 technology preview
        * @deprecated This API is for technology preview only.
        */
        UBool isWildcard() const { return wildcard; }
        /**
         * Returns the contents of this key as a literal.
         * Precondition: !isWildcard()
         *
         * @return The literal contents of the key
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        const Literal& asLiteral() const;

// TODO
        Key(const Key& other) : wildcard(other.wildcard), contents(other.contents) {}
        // Wildcard constructor
        Key() : wildcard(true) {}
        // Concrete key constructor
        Key(const Literal& lit) : wildcard(false), contents(lit) {}
        Key& operator=(Key&& other) noexcept;
        // TODO
        // Defined so SelectorKeys can be compared
        bool operator<(const Key& rhs) const;
        bool operator==(const Key& rhs) const;

    private:
        UnicodeString toString() const;

        /* const */ bool wildcard; // True if this represents the wildcard "*"
        /* const */ Literal contents;
    }; // class Key

        /**
         * An immutable list of keys
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */

        using KeyList = std::vector<Key>;

    /**
     * The `SelectorKeys` class represents the key list for a single variant.
     * It corresponds to the `keys` array in the `Variant` interface
     * defined in https://github.com/unicode-org/message-format-wg/blob/main/spec/data-model.md#messages
     *
     * `SelectorKeys` is immutable, copyable and movable.
     *
     * @internal ICU 75.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    class U_I18N_API SelectorKeys : public UObject {
    public:
// TODO
        SelectorKeys();
// TODO
        virtual ~SelectorKeys();
// Note: these are public in order to make vector addition work
        // Copy constructor
        SelectorKeys(const SelectorKeys& other);
// TODO
        // Move assignment operator
        SelectorKeys& operator=(SelectorKeys&&) noexcept;
        // Copy assignment operator
        SelectorKeys& operator=(const SelectorKeys& other) {
            keys = KeyList(other.keys);
            return *this;
        }

        /**
         * Returns the underlying list of keys.
         *
         * @return A reference to the list of keys for this variant.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        const KeyList& getKeys() const;
        /**
         * The mutable `SelectorKeys::Builder` class allows the key list to be constructed
         * one key at a time.
         *
         * Builder is not copyable or movable.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        class U_I18N_API Builder : public UMemory {
        private:
            friend class SelectorKeys;
            std::vector<Key> keys;
        public:
            /**
             * Adds a single key to the list.
             *
             * @param key A reference to the key to be added. `key` is copied.
             * @return A reference to the builder.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
             Builder& add(const Key& key);
             /**
              * Constructs a new immutable `SelectorKeys` using the list of keys
              * set with previous `add()` calls.
              *
              * The builder object (`this`) can still be used after calling `build()`.
              *
              * @return          The new SelectorKeys object
              *
              * @internal ICU 75.0 technology preview
              * @deprecated This API is for technology preview only.
              */
             SelectorKeys build() const;
             /**
              * Destructor.
              *
              * @internal ICU 75.0 technology preview
              * @deprecated This API is for technology preview only.
              */
             virtual ~Builder();
             // TODO
            Builder();
        }; // class SelectorKeys::Builder

        // TODO
        // Defined for use by VariantMap
        bool operator<(const SelectorKeys& rhs) const;

    private:
        friend class Builder;


        // Non-const so copy assignment operator works, but actually const
        KeyList keys;
        SelectorKeys(const KeyList& ks) : keys(ks) {}
    }; // class SelectorKeys

    /**
     * The `Reserved` class represents a `reserved` annotation, as in the `reserved` nonterminal
     * in the MessageFormat 2 grammar or the `Reserved` interface
     * defined in
     * https://github.com/unicode-org/message-format-wg/blob/main/spec/data-model.md#expressions
     *
     * `Reserved` is immutable, copyable and movable.
     *
     * @internal ICU 75.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    class Reserved : public UMemory {
    public:
        /**
         * A `Reserved` is a sequence of literals.
         *
         * @return The number of literals.
         *         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        int32_t numParts() const;
        /**
         * Indexes into the sequence.
         * Precondition: i < numParts()
         *
         * @param i Index of the part being accessed.
         * @return A reference to he i'th literal in the sequence
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        const Literal& getPart(int32_t i) const;
// TODO
        // Reserved needs a copy constructor in order to make Expression deeply copyable
        Reserved(const Reserved& other);
        Reserved(Reserved&& other);
        Reserved& operator=(Reserved&& other) noexcept;
        Reserved& operator=(const Reserved& other);
        Reserved() : parts(std::vector<Literal>()) { }

        /**
         * The mutable `Reserved::Builder` class allows the reserved sequence to be
         * constructed one part at a time.
         *
         * Builder is not copyable or movable.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        class U_I18N_API Builder : public UMemory {
        private:
            friend class Reserved;

            std::vector<Literal> parts;

        public:
            /**
             * Adds a single literal to the reserved sequence.
             *
             * @param part The literal to be added
             * @return A reference to the builder.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Builder& add(const Literal& part);
            /**
             * Constructs a new immutable `Reserved` using the list of parts
             * set with previous `add()` calls.
             *
             * The builder object (`this`) can still be used after calling `build()`.
             *
             * @return          The new Reserved object
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Reserved build() const;

            // TODO comment
            Builder();

             /**
              * Destructor.
              *
              * @internal ICU 75.0 technology preview
              * @deprecated This API is for technology preview only.
              */
             virtual ~Builder();
        }; // class Reserved::Builder
    private:
        friend class Operator;

        // Possibly-empty list of parts
        // `literal` reserved as a quoted literal; `reserved-char` / `reserved-escape`
        // strings represented as unquoted literals
        /* const */ std::vector<Literal> parts;

        Reserved(const std::vector<Literal>&);
    };

    /**
     * An immutable map from strings to function options
     *
     * @internal ICU 75.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    using OptionMap = OrderedMap<UnicodeString, Operand>;

} // namespace data_model

    template<>
    OrderedMap<UnicodeString, data_model::Operand>::Builder::~Builder();
    template<>
    OrderedMap<UnicodeString, data_model::Operand>::~OrderedMap();

namespace data_model {
    /**
     * The `Operator` class corresponds to the `FunctionRef | Reserved` type in the
     * `Expression` interface defined in
     * https://github.com/unicode-org/message-format-wg/blob/main/spec/data-model.md#patterns
     *
     * It represents the annotation that an expression can have: either a function name paired
     * with a map from option names to operands (possibly empty),
     * or a reserved sequence, which has no meaning and results in an error if the formatter
     * is invoked.
     *
     * `Operator` is immutable, copyable and movable.
     *
     * @internal ICU 75.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    class U_I18N_API Operator : public UObject {
    public:
        /**
         * Determines if this operator is a reserved annotation.
         *
         * @return true if and only if this operator represents a reserved sequence.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        UBool isReserved() const { return isReservedSequence; }
        /**
         * Accesses the function name.
         * Precondition: !isReserved()
         *
         * @return The function name of this operator.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        const FunctionName& getFunctionName() const;
        /**
         * Accesses the underlying reserved sequence.
         * Precondition: isReserved()
         *
         * @return The reserved sequence represented by this operator.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        const Reserved& asReserved() const;
        /**
         * Accesses function options.
         * Precondition: !isReserved()
         *
         * @return A reference to the function options for this operator.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        const OptionMap& getOptions() const;

        /**
         * The mutable `Operator::Builder` class allows the operator to be constructed
         * incrementally.
         *
         * Builder is not copyable or movable.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        class U_I18N_API Builder : public UMemory {
        private:
            friend class Operator;
            bool isReservedSequence = false;
            bool hasFunctionName = false;
            bool hasOptions = false;
            Reserved asReserved;
            FunctionName functionName;
            OptionMap::Builder options;
        public:
            /**
             * Sets this operator to be a reserved sequence.
             * If a function name and/or options were previously set,
             * clears them.
             *
             * @param reserved The reserved sequence to set as the contents of this Operator.
             *                 (Passed by move.)
             * @return A reference to the builder.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Builder& setReserved(Reserved&& reserved);
            /**
             * Sets this operator to be a function annotation and sets its name
             * to `func`.
             * If a reserved sequence was previously set, clears it.
             *
             * @param func The function name.
             * @return A reference to the builder.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Builder& setFunctionName(FunctionName&& func);
            /**
             * Sets this operator to be a function annotation and adds a
             * single option.
             * If a reserved sequence was previously set, clears it.
             *
             * @param key The name of the option.
             * @param value The value (right-hand side) of the option.
             * @param status Input/output error code.
             * @return A reference to the builder.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Builder& addOption(const UnicodeString &key, Operand&& value, UErrorCode& status);
            /**
             * Constructs a new immutable `Operator` using the `reserved` annotation
             * or the function name and options that were previously set.
             * If neither `setReserved()` nor `setFunctionName()` was previously
             * called, then `status` is set to U_INVALID_STATE_ERROR.
             *
             * The builder object (`this`) can still be used after calling `build()`.
             *
             * @param status    Input/output error code.
             * @return          The new Operator
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Operator build(UErrorCode& status) const;
             /**
              * Destructor.
              *
              * @internal ICU 75.0 technology preview
              * @deprecated This API is for technology preview only.
              */
             virtual ~Builder();
        }; // class Operator::Builder
        // TODO
        Operator& operator=(Operator&& other) noexcept;
        Operator& operator=(const Operator&);
        Operator() : isReservedSequence(true) {}
        // Copy constructor
        Operator(const Operator& other);
        /**
         * Destructor.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        virtual ~Operator();
    private:
        // Function call constructor
        Operator(const FunctionName& f, OptionMap&& l);
        // Reserved sequence constructor
        // Result is bogus if copy of `r` fails
        Operator(const Reserved& r) : isReservedSequence(true), functionName(FunctionName(UnicodeString(""))), reserved(Reserved(r)) {}

        /* const */ bool isReservedSequence;
        /* const */ FunctionName functionName;
        /* const */ OptionMap options;
        /* const */ Reserved reserved;
    }; // class Operator

    /**
     * The `Expression` class corresponds to the `expression` nonterminal in the MessageFormat 2
     * grammar and the `Expression` interface defined in
     * https://github.com/unicode-org/message-format-wg/blob/main/spec/data-model.md#patterns
     *
     * It represents either an operand with no annotation; an annotation with no operand;
     * or an operand annotated with an annotation.
     *
     * `Expression` is immutable, copyable and movable.
     *
     * @internal ICU 75.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    class U_I18N_API Expression : public UObject {
    public:
        /**
         * Checks if this expression is an annotation
         * with no operand.
         *
         * @return True if and only if the expression has
         *         an annotation and has no operand.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        UBool isStandaloneAnnotation() const;
        /**
         * Checks if this expression has a function
         * annotation (with or without an operand). A reserved
         * sequence is not a function annotation.
         *
         * @return True if and only if the expression has an annotation
         *         that is a function.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        UBool isFunctionCall() const;
        /**
         * Returns true if and only if this expression is
         * annotated with a reserved sequence.
         *
         * @return True if and only if the expression has an
         *         annotation that is a reserved sequence,
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        UBool isReserved() const;
        /**
         * Accesses the function or reserved sequence
         * annotating this expression.
         * Precondition: isFunctionCall() || isReserved()
         *
         * @return A reference to the operator of this expression.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        const Operator& getOperator() const;
        /**
         * Accesses the operand of this expression.
         *
         * @return A reference to the operand of this expression,
         *         which may be the null operand.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        const Operand& getOperand() const;

        /**
         * The mutable `Expression::Builder` class allows the operator to be constructed
         * incrementally.
         *
         * Builder is not copyable or movable.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        class U_I18N_API Builder : public UMemory {
        private:
            friend class Expression;
            bool hasOperand = false;
            bool hasOperator = false;
            Operand rand;
            Operator rator;
        public:
// TODO
          Builder() {}

            /**
             * Sets the operand of this expression.
             *
             * @param rAnd The operand to set. Passed by move.
             * @return A reference to the builder.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
             Builder& setOperand(Operand&& rAnd);
             /**
              * Sets the operator of this expression.
              *
              * @param rAtor The operator to set. Passed by move.
              * @return A reference to the builder.
              *
              * @internal ICU 75.0 technology preview
              * @deprecated This API is for technology preview only.
              */
             Builder& setOperator(Operator&& rAtor);
             /**
              * Constructs a new immutable `Expression` using the operand and operator that
              * were previously set. If neither `setOperand()` nor `setOperator()` was
              * previously called, or if `setOperand()` was called with the null operand
              * and `setOperator()` was never called, then `status` is set to
              * U_INVALID_STATE_ERROR.
              *
              * The builder object (`this`) can still be used after calling `build()`.
              *
              * @param status    Input/output error code.
              * @return          The new Expression.
              *
              * @internal ICU 75.0 technology preview
              * @deprecated This API is for technology preview only.
              */
             Expression build(UErrorCode& status) const;
             /**
              * Destructor.
              *
              * @internal ICU 75.0 technology preview
              * @deprecated This API is for technology preview only.
              */
             virtual ~Builder();
        }; // class Expression::Builder
        /**
         * Returns a new `Expression::Builder` object.
         *
         * @param status  Input/output error code.
         * @return        The new Builder, which is non-null if U_SUCCESS(status).
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
//        static Builder* builder(UErrorCode& status);

// TODO

        // Expression needs a copy constructor in order to make Pattern deeply copyable
        // (and for closures)
        Expression(const Expression& other);
        Expression();
        Expression& operator=(Expression&&) noexcept;
        Expression& operator=(const Expression&);

    private:
        /*
          Internally, an expression is represented as the application of an optional operator to an operand.
          The operand is always present; for function calls with no operand, it's represented
          as an operand for which `isNull()` is true.

                                      Operator               | Operand
                                      --------------------------------
          { |42| :fun opt=value } =>  (FunctionName=fun,     | Literal(quoted=true, contents="42")
                                      options={opt: value})
          { abcd }                =>  null                   | Literal(quoted=false, contents="abcd")
          { : fun opt=value }     =>  (FunctionName=fun,
                                      options={opt: value})  | NullOperand()
        */

        bool hasOperator;

        Expression(const Operator &rAtor, const Operand &rAnd) : hasOperator(true), rator(Operator(rAtor)), rand(Operand(rAnd)) {}
        Expression(const Operand &rAnd) : hasOperator(false), rator(), rand(Operand(rAnd)) {}
        Expression(const Operator &rAtor) : hasOperator(true), rator(Operator(rAtor)), rand(Operand()) {}
        /* const */ Operator rator; // Ignored if !hasOperator
        /* const */ Operand rand;
    }; // class Expression

    /**
     *  A `PatternPart` is a single element (text or expression) in a `Pattern`.
     * It corresponds to the `body` field of the `Pattern` interface
     *  defined in https://github.com/unicode-org/message-format-wg/blob/main/spec/data-model.md#patterns
     *
     * `PatternPart` is immutable, copyable and movable.
     *
     * @internal ICU 75.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    class U_I18N_API PatternPart : public UObject {
    public:
        /**
         * Checks if the part is a text part.
         *
         * @return True if and only if this is a text part.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        UBool isText() const { return isRawText; }
        /**
         * Accesses the expression of the part.
         * Precondition: !isText()
         *
         * @return A reference to the part's underlying expression.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        const Expression& contents() const;
        /**
         * Accesses the text contents of the part.
         * Precondition: isText()
         *
         * @return A reference to a string representing the part's text..
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        const UnicodeString& asText() const;
// TODO
        // PatternPart needs a copy constructor in order to make Pattern deeply copyable
        PatternPart(const PatternPart& other);
        PatternPart& operator=(PatternPart&&) noexcept;
        PatternPart& operator=(const PatternPart& other);
        virtual ~PatternPart();
        PatternPart() : isRawText(true) {}

        // Text
        PatternPart(const UnicodeString& t) : isRawText(true), text(t) {}
        // Expression
        PatternPart(Expression&& e) : isRawText(false), expression(e) {}

    private:
        /* const */ bool isRawText;
        // Not used if !isRawText
        /* const */ UnicodeString text;
        // Not used if isRawText
        /* const */ Expression expression;
    }; // class PatternPart

    /**
     *  A `Pattern` is a sequence of formattable parts.
     * It corresponds to the `Pattern` interface
     * defined in https://github.com/unicode-org/message-format-wg/blob/main/spec/data-model.md#patterns
     *
     * `Pattern` is immutable, copyable and movable.
     *
     * @internal ICU 75.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    class U_I18N_API Pattern : public UObject {
    public:
        // TODO: should only be for initializing
        // MessageFormatDataModel::Builder
        Pattern() : parts(std::vector<PatternPart>()) {}

        /**
         * Returns the size.
         *
         * @return The number of parts in the pattern.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        int32_t numParts() const { return parts.size(); }
        /**
         * Returns the `i`th part in the pattern.
         * Precondition: i < numParts()
         *
         * @param i Index of the part being accessed.
         * @return  A reference to the part at index `i`.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        const PatternPart& getPart(int32_t i) const;

// TODO
        // Pattern needs a copy constructor in order to make MessageFormatDataModel::build() be a copying rather than
        // moving build
        Pattern(const Pattern& other);
        Pattern& operator=(const Pattern& other);
        Pattern& operator=(Pattern&& other) noexcept;

        /**
         * The mutable `Pattern::Builder` class allows the pattern to be
         * constructed one part at a time.
         *
         * Builder is not copyable or movable.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        class U_I18N_API Builder : public UMemory {
        private:
            friend class Pattern;

            // Note this is why PatternPart and all its enclosed classes need
            // copy constructors: when the build() method is called on `parts`,
            // it should copy `parts` rather than moving it
            std::vector<PatternPart> parts;

        public:
            /**
             * Adds a single part to the pattern. Copies `part`.
             *
             * @param part The part to be added.
             * @return A reference to the builder.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Builder& add(const PatternPart& part);
            /**
             * Constructs a new immutable `Pattern` using the list of parts
             * set with previous `add()` calls.
             *
             * The builder object (`this`) can still be used after calling `build()`.
             *
             * @param status    Input/output error code.
             * @return          The pattern object
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Pattern build() const;
            // TODO
            Builder() = default;
             /**
              * Destructor.
              *
              * @internal ICU 75.0 technology preview
              * @deprecated This API is for technology preview only.
              */
             virtual ~Builder();
        }; // class Pattern::Builder


    private:
        // Possibly-empty list of parts
        /* const */ std::vector<PatternPart> parts;

        // Should only be called by Builder
        Pattern(const std::vector<PatternPart>& ps);
    }; // class Pattern

      using VariantMap = OrderedMap<SelectorKeys, Pattern>;

    /**
     *  A `Binding` pairs a variable name with an expression.
     * It corresponds to the `Declaration` interface
     * defined in https://github.com/unicode-org/message-format-wg/blob/main/spec/data-model.md#messages
     *
     * `Binding` is immutable and copyable. It is not movable.
     *
     * @internal ICU 75.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    class U_I18N_API Binding : public UObject {
    public:
        /**
         * Creates a new binding.  Adopts `e`, which must be non-null.
         *
         * @param var       The variable name of the declaration.
         * @param e         The expression (right-hand side) of the declaration.
         * @param status    Input/output error code.
         * @return          The new binding, which is non-null if U_SUCCESS(status).
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
//        static Binding* create(const VariableName& var, Expression* e, UErrorCode& status);
        /**
          * Accesses the right-hand side of the binding.
          *
          * @return A reference to the expression.
          *
          * @internal ICU 75.0 technology preview
          * @deprecated This API is for technology preview only.
          */
        const Expression& getValue() const;
        /**
          * Accesses the left-hand side of the binding.
          *
          * @return A reference to the variable name.
          *
          * @internal ICU 75.0 technology preview
          * @deprecated This API is for technology preview only.
          */
        const VariableName& getVariable() const { return var; }
// TODO
        Binding(const VariableName& v, const Expression& e) : var(v), value(e){}
        // This needs a copy constructor so that `Bindings` is deeply-copyable,
        // which is in turn so that MessageFormatDataModel::build() can be copying
        // (it has to copy the builder's locals)
        Binding(const Binding& other);
        Binding& operator=(const Binding& other);

        /**
          * Destructor.
          *
          * @internal ICU 75.0 technology preview
          * @deprecated This API is for technology preview only.
          */
        virtual ~Binding();
    private:
        /* const */ VariableName var;
        /* const */ Expression value;
    }; // class Binding

        /**
         * An immutable list of variable bindings
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        using Bindings = std::vector<Binding>;
        /**
         * An immutable list of expressions
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        using ExpressionList = std::vector<Expression>;
    } // namespace data_model

// These explicit instantiations have to come before the
// destructor definitions
template<>
OrderedMap<data_model::SelectorKeys, data_model::Pattern>::Builder::~Builder();
template<>
OrderedMap<data_model::SelectorKeys, data_model::Pattern>::~OrderedMap();

} // namespace message2

// Explicit instantiations in source/i18n/messageformat2_utils.cpp
// See numberformatter.h for another example

// (MSVC treats imports/exports of explicit instantiations differently.)
#ifndef _MSC_VER
extern template class message2::OrderedMap<UnicodeString, message2::data_model::Operand>;
extern template class message2::OrderedMap<message2::data_model::SelectorKeys, message2::data_model::Pattern>;
#endif

namespace message2 {

using namespace data_model;

// -----------------------------------------------------------------------
// Public MessageFormatDataModel class

/**
 *
 * The `MessageFormatDataModel` class describes a parsed representation of the text of a message.
 * This representation is public as higher-level APIs for messages will need to know its public
 * interface: for example, to re-instantiate a parsed message with different values for imported
variables.
 *
 * The MessageFormatDataModel API implements <a target="github"
href="https://github.com/unicode-org/message-format-wg/blob/main/spec/data-model.md">the
 * specification of the abstract syntax (data model representation)</a> for MessageFormat.
 *
 * `MessageFormatDataModel` is immutable, copyable and movable.
 *
 * @internal ICU 75.0 technology preview
 * @deprecated This API is for technology preview only.
 */
class U_I18N_API MessageFormatDataModel : public UMemory {
/*
  Classes that represent nodes in the data model are nested inside the
  `MessageFormatDataModel` class.

  Classes such as `Expression`, `Pattern` and `VariantMap` are immutable and
  are constructed using the builder pattern.

  Most classes representing nodes have copy constructors. This is because builders
  contain immutable data that must be copied when calling `build()`, since the builder
  could go out of scope before the immutable result of the builder does. Copying is
  also necessary to prevent unexpected mutation if intermediate builders are saved
  and mutated again after calling `build()`.

  The copy constructors perform a deep copy, for example by copying the entire
  list of options for an `Operator` (and copying the entire underlying vector.)
  Some internal fields should be `const`, but are declared as non-`const` to make
  the copy constructor simpler to implement. (These are noted throughout.) In
  other words, those fields are `const` except during the execution of a copy
  constructor.

  On the other hand, intermediate `Builder` methods that return a `Builder&`
  mutate the state of the builder, so in code like:

  Expression::Builder& exprBuilder = Expression::builder()-> setOperand(foo);
  Expression::Builder& exprBuilder2 = exprBuilder.setOperator(bar);

  the call to `setOperator()` would mutate `exprBuilder`, since `exprBuilder`
  and `exprBuilder2` are references to the same object.

  An alternate choice would be to make `build()` destructive, so that copying would
  be unnecessary. Or, both copying and moving variants of `build()` could be
  provided. Copying variants of the intermediate `Builder` methods could be
  provided as well, if this proved useful.
*/
      public:
// TODO
    MessageFormatDataModel();
    MessageFormatDataModel(MessageFormatDataModel&&) noexcept;
    MessageFormatDataModel& operator=(MessageFormatDataModel&&) noexcept;
    MessageFormatDataModel &operator=(const MessageFormatDataModel &);

    /**
     * Accesses the local variable declarations for this data model.
     *
     * @return A reference to a list of bindings for local variables.
     *
     * @internal ICU 75.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    const Bindings& getLocalVariables() const;
    /**
     * Determines what type of message this is.
     *
     * @return true if and only if this data model represents a `selectors` message
     *         (if it represents a `match` construct with selectors and variants).
     *
     *
     * @internal ICU 75.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    UBool hasSelectors() const;
    /**
     * Accesses the selectors.
     * Precondition: hasSelectors()
     *
     * @return A reference to the selector list.
     *
     * @internal ICU 75.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    const ExpressionList& getSelectors() const;
    /**
     * Accesses the variants.
     * Precondition: hasSelectors()
     *
     * @return A reference to the variant map.
     *
     * @internal ICU 75.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    const VariantMap& getVariants() const;
    /**
     * Accesses the pattern (in a message without selectors).
     * Precondition: !hasSelectors()
     *
     * @return A reference to the pattern.
     *
     * @internal ICU 75.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    const Pattern& getPattern() const;

    /**
     * The mutable `MessageFormatDataModel::Builder` class allows the data model to be
     * constructed incrementally.
     *
     * @internal ICU 75.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    class U_I18N_API Builder;

    /**
     * Destructor.
     *
     * @internal ICU 75.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    virtual ~MessageFormatDataModel();

    /**
     * The mutable `MessageFormatDataModel::Builder` class allows the data model to be
     * constructed incrementally.
     *
     * @internal ICU 75.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    class U_I18N_API Builder : public UMemory {
    private:
        friend class MessageFormatDataModel;

        void buildSelectorsMessage();
        bool hasPattern = true;
        bool hasSelectors = false;
        Pattern pattern;
        ExpressionList selectors;
        VariantMap::Builder variants;
        Bindings locals;

    public:
// TODO
        Builder();

        /**
         * Adds a local variable declaration.
         *
         * @param variableName The variable name of the declaration.
         * @param expression The expression to which `variableName` should be bound.
         * @return A reference to the builder.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        Builder& addLocalVariable(const VariableName& variableName, const Expression& expression);
        /**
         * Adds a selector expression. Copies `expression`.
         * If a pattern was previously set, clears the pattern.
         *
         * @param selector Expression to add as a selector.
         * @param status Input/output error code.
         * @return A reference to the builder.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        Builder& addSelector(const Expression& selector);
        /**
         * Adds a single variant.
         * If a pattern was previously set using `setPattern()`, clears the pattern.
         *
         * @param keys Keys for the variant. Passed by move.
         * @param pattern Pattern for the variant. Passed by move.
         * @return A reference to the builder.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        Builder& addVariant(SelectorKeys&& keys, Pattern&& pattern);
        /**
         * Sets the body of the message as a pattern.
         * If selectors and/or variants were previously set, clears them.
         *
         * @param pattern Pattern to represent the body of the message.
         *                Passed by move.
         * @return A reference to the builder.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        Builder& setPattern(Pattern&& pattern);
        /**
         * Constructs a new immutable data model.
         * If `setPattern()` has not been called and if `addSelector()` and
         * `addVariant()` were not each called at least once,
         * `status` is set to `U_INVALID_STATE_ERROR`.
         * If `addSelector()` was called and `addVariant()` was never called,
         * or vice versa, then `status` is set to U_INVALID_STATE_ERROR.
         * Otherwise, either a Pattern or Selectors message is constructed
         * based on the pattern that was previously set, or selectors and variants
         * that were previously set.
         *
         * The builder object (`this`) can still be used after calling `build()`.
         *
         * @param status Input/output error code.
         * @return       The new MessageFormatDataModel
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        MessageFormatDataModel build(UErrorCode& status) const;
    }; // class Builder

private:
    /* const */ bool hasPattern;

    // The expressions that are being matched on.
    // Ignored if `hasPattern`
    /* const */ ExpressionList selectors;

    // The list of `when` clauses (case arms).
    // Ignored if `hasPattern`
    /* const */ VariantMap variants;

    // The pattern forming the body of the message.
    // Ignored if !hasPattern
    /* const */ Pattern pattern;

    // Bindings for local variables
    /* const */ Bindings bindings;

    MessageFormatDataModel(const Builder& builder);
    }; // class MessageFormatDataModel

} // namespace message2

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */

#endif /* U_SHOW_CPLUSPLUS_API */

#endif // MESSAGEFORMAT_DATA_MODEL_H

#endif // U_HIDE_DEPRECATED_API
// eof

