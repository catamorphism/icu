// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#include "unicode/uconfig.h"

#ifndef U_HIDE_DEPRECATED_API

#ifndef MESSAGEFORMAT_DATA_MODEL_H
#define MESSAGEFORMAT_DATA_MODEL_H

#if U_SHOW_CPLUSPLUS_API

#if !UCONFIG_NO_FORMATTING

#include "unicode/fmtable.h"
#include "unicode/unistr.h"

#include <vector>

U_NAMESPACE_BEGIN

class UVector;

// Helpers

// Note: this _must_ be declared `inline` or else gcc will generate code
// for its instantiations, which needs to be avoided because it returns
// a std::vector
template<typename T>
static inline std::vector<T> toStdVector(const T* arr, int32_t len) {
    std::vector<T> result;
    for (int32_t i = 0; i < len; i++) {
        result.push_back(arr[i]);
    }
    return result;
}

namespace message2 {

    class Checker;
    class MessageFormatDataModel;
    class MessageFormatter;
    class Serializer;

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
             * Puts the VariableName into a valid but undefined state.
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
            /**
             * Move assignment operator:
             * The source VariableName will be left in a valid but undefined state.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            VariableName& operator=(VariableName&&) noexcept = default;
            /**
             * Copy assignment operator
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            VariableName& operator=(const VariableName&) = default;
            /**
             * Copy constructor.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
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
            /**
             * Copy constructor.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            FunctionName(const FunctionName& other) : functionName(other.functionName), functionSigil(other.functionSigil) {}
            /**
             * Copy assignment operator.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            FunctionName& operator=(const FunctionName&) = default;
            /**
             * Move assignment operator:
             * The source FunctionName will be left in a valid but undefined state.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            FunctionName& operator=(FunctionName&& other) noexcept;
            /**
             * Default constructor.
             * Puts the FunctionName into a valid but undefined state.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            FunctionName() : functionSigil(Sigil::DEFAULT) {}
            /**
             * Less than operator. Compares `this.toString()` with
             * `other.toString()`. This method is used in the implementation of
             * the `FunctionRegistry` class and is not expected to be useful otherwise.
             *
             * @param other The FunctionName to compare to this one.
             * @return true if the two `FunctionName`s have the same sigil
             * and the name string in `this` is less than the name string in
             * `other` (according to `UnicodeString`'s less-than operator).
             * Returns false otherwise.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            bool operator<(const FunctionName& other) const;
            /**
             * Equality operator. Compares `this.toString()` with
             * `other.toString()`. This method is used in the implementation of
             * the `FunctionRegistry` class and is not expected to be useful otherwise.
             *
             * @param other The FunctionName to compare to this one.
             * @return true if and only if `this.toString()` == `other.toString()`
             * Returns false otherwise.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            bool operator==(const FunctionName& other) const;

        private:
            /* const */ UnicodeString functionName;
            /* const */ Sigil functionSigil;

            UChar sigilChar() const;
        }; // class FunctionName

        class Literal;

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
        class U_I18N_API Reserved : public UMemory {
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
                UVector* parts; // Not a LocalPointer for the same reason as in `SelectorKeys::Builder`

            public:
                /**
                 * Adds a single literal to the reserved sequence.
                 *
                 * @param part The literal to be added. Passed by move.
                 * @param status Input/output error code
                 * @return A reference to the builder.
                 *
                 * @internal ICU 75.0 technology preview
                 * @deprecated This API is for technology preview only.
                 */
                Builder& add(Literal&& part, UErrorCode& status) noexcept;
                /**
                 * Constructs a new immutable `Reserved` using the list of parts
                 * set with previous `add()` calls.
                 *
                 * The builder object (`this`) can still be used after calling `build()`.
                 *
                 * param status Input/output error code
                 * @return          The new Reserved object
                 *
                 * @internal ICU 75.0 technology preview
                 * @deprecated This API is for technology preview only.
                 */
                Reserved build(UErrorCode& status) const noexcept;
                /**
                 * Default constructor.
                 * Returns a builder with an empty Reserved sequence.
                 *
                 * param status Input/output error code
                 *
                 * @internal ICU 75.0 technology preview
                 * @deprecated This API is for technology preview only.
                 */
                Builder(UErrorCode& status);
                /**
                 * Destructor.
                 *
                 * @internal ICU 75.0 technology preview
                 * @deprecated This API is for technology preview only.
                 */
                virtual ~Builder();
            }; // class Reserved::Builder
            /**
             * Copy constructor.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Reserved(const Reserved& other);
            /**
             * Copy assignment operator
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Reserved& operator=(const Reserved& other);
            /**
             * Move assignment operator:
             * The source Reserved will be left in a valid but undefined state.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Reserved& operator=(Reserved&& other) noexcept;
            /**
             * Default constructor.
             * Puts the Reserved into a valid but undefined state.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Reserved() { parts = LocalArray<Literal>(); }
        private:
            friend class Builder;
            friend class Operator;

            // Possibly-empty list of parts
            // `literal` reserved as a quoted literal; `reserved-char` / `reserved-escape`
            // strings represented as unquoted literals
            /* const */ LocalArray<Literal> parts;
            int32_t len = 0;
            Reserved(const UVector& parts, UErrorCode& status) noexcept;
            // Helper
            static void initLiterals(Reserved&, const Reserved&);
        };

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
            UnicodeString quoted() const;
            /**
             * Returns the parsed string contents of this literal.
             *
             * @return A string representation of this literal.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            const UnicodeString& unquoted() const;
            /**
             * Determines if this literal appeared as a quoted literal in the message.
             *
             * @return true if and only if this literal appeared as a quoted literal in the
             *         message.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            UBool isQuoted() const { return thisIsQuoted; }
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
            Literal(UBool q, const UnicodeString& s) : thisIsQuoted(q), contents(s) {}
            /**
             * Copy constructor.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Literal(const Literal& other) : thisIsQuoted(other.thisIsQuoted), contents(other.contents) {}
            /**
             * Copy assignment operator.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Literal& operator=(const Literal&);
            /**
             * Move assignment operator.
             * The source Literal will be left in a valid but undefined state.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Literal& operator=(Literal&&) noexcept;
            /**
             * Default constructor.
             * Puts the Literal into a valid but undefined state.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Literal() = default;
            /**
             * Less than operator. Compares `this.stringContents()` with
             * `other.stringContents()`. This method is used in representing
             * the mapping from key lists to patterns in a message with variants,
             * and is not expected to be useful otherwise.
             *
             * @param other The Literal to compare to this one.
             * @return true if the parsed string corresponding to this `Literal`
             * is less than the parsed string corresponding to the other `Literal`
             * (according to `UnicodeString`'s less-than operator).
             * Returns false otherwise.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            bool operator<(const Literal& other) const;
            /**
             * Equality operator. Compares `this.stringContents()` with
             * `other.stringContents()`. This method is used in representing
             * the mapping from key lists to patterns in a message with variants,
             * and is not expected to be useful otherwise.
             *
             * @param other The Literal to compare to this one.
             * @return true if the parsed string corresponding to this `Literal`
             * equals the parsed string corresponding to the other `Literal`
             * (according to `UnicodeString`'s equality operator).
             * Returns false otherwise.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            bool operator==(const Literal& other) const;
            /**
             * Destructor.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            virtual ~Literal();

        private:
            friend class Reserved::Builder;

            /* const */ bool thisIsQuoted = false;
            /* const */ UnicodeString contents;
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
            /**
             * Default constructor.
             * Creates a null Operand.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Operand() : type(Type::NULL_OPERAND) {}
            /**
             * Variable operand constructor.
             *
             * @param v The variable name; an operand corresponding
             *        to a reference to `v` is returned.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Operand(const VariableName& v) : var(v), type(Type::VARIABLE) {}
            /**
             * Literal operand constructor.
             *
             * @param l The literal to use for this operand; an operand
             *        corresponding to `l` is returned.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Operand(const Literal& l) : lit(l), type(Type::LITERAL) {}
            /**
             * Copy assignment operator.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Operand& operator=(const Operand&);
            /**
             * Copy constructor.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Operand(const Operand&);
            /**
             * Move assignment operator:
             * The source Operand will be left in a valid but undefined state.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Operand& operator=(Operand&&) noexcept;
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
            /**
             * Copy constructor.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Key(const Key& other) : wildcard(other.wildcard), contents(other.contents) {}
            /**
             * Wildcard constructor; constructs a Key representing the
             * catchall or wildcard key, '*'.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Key() : wildcard(true), contents(Literal()) {}
            /**
             * Literal key constructor.
             *
             * @param lit A Literal to use for this key. The result matches the
             *        literal `lit`.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Key(const Literal& lit) : wildcard(false), contents(lit) {}
            /**
             * Move assignment operator:
             * The source Key will be left in a valid but undefined state.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Key& operator=(Key&& other) noexcept;
            /**
             * Copy assignment operator
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Key& operator=(const Key& other);
            /**
             * Less than operator. Compares the literal of `this` with the literal of `other`.
             * This method is used in representing the mapping from key lists to patterns
             * in a message with variants, and is not expected to be useful otherwise.
             *
             * @param other The Key to compare to this one.
             * @return true if the two `Key`s are not wildcards and if `this.asLiteral()`
             * < `other.asLiteral()`.
             * Returns false otherwise.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            bool operator<(const Key& other) const;
            /**
             * Equality operator. Compares the literal of `this` with the literal of `other`.
             * This method is used in representing the mapping from key lists to patterns
             * in a message with variants, and is not expected to be useful otherwise.
             *
             * @param other The Key to compare to this one.
             * @return true if either both `Key`s are wildcards, or `this.asLiteral()`
             * == `other.asLiteral()`.
             * Returns false otherwise.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            bool operator==(const Key& other) const;
            /**
             * Destructor.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            virtual ~Key();
        private:
            /* const */ bool wildcard; // True if this represents the wildcard "*"
            /* const */ Literal contents;
        }; // class Key

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
            /**
             * Returns the underlying list of keys.
             *
             * @return The list of keys for this variant.
             *         Returns an empty list if allocating this `SelectorKeys`
             *         object previously failed.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            std::vector<Key> getKeys() const {
                return toStdVector<Key>(keys.getAlias(), len);
            }
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
                UVector* keys; // This is a raw pointer and not a LocalPointer<UVector> to avoid undefined behavior warnings,
                               // since UVector is forward-declared
                               // The vector owns its elements
            public:
                /**
                 * Adds a single key to the list.
                 *
                 * @param key    The key to be added. Passed by move
                 * @param status Input/output error code
                 * @return A reference to the builder.
                 *
                 * @internal ICU 75.0 technology preview
                 * @deprecated This API is for technology preview only.
                 */
                Builder& add(Key&& key, UErrorCode& status) noexcept;
                /**
                 * Constructs a new immutable `SelectorKeys` using the list of keys
                 * set with previous `add()` calls.
                 *
                 * The builder object (`this`) can still be used after calling `build()`.
                 *
                 * @param status    Input/output error code
                 * @return          The new SelectorKeys object
                 *
                 * @internal ICU 75.0 technology preview
                 * @deprecated This API is for technology preview only.
                 */
                SelectorKeys build(UErrorCode& status) const;
                /**
                 * Default constructor.
                 * Returns a Builder with an empty list of keys.
                 *
                 * @param status Input/output error code
                 *
                 * @internal ICU 75.0 technology preview
                 * @deprecated This API is for technology preview only.
                 */
                Builder(UErrorCode& status);
                /**
                 * Destructor.
                 *
                 * @internal ICU 75.0 technology preview
                 * @deprecated This API is for technology preview only.
                 */
                virtual ~Builder();
            }; // class SelectorKeys::Builder
            /**
             * Less than operator. Compares the two key lists lexicographically.
             * This method makes it possible for a `SelectorKeys` to be used as a map
             * key, which allows variants to be represented as a map. It is not expected
             * to be useful otherwise.
             *
             * @param other The SelectorKeys to compare to this one.
             * @return true if `this` is less than `other`, comparing the two key lists
             * lexicographically.
             * Returns false otherwise.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            bool operator<(const SelectorKeys& other) const;
            /**
             * Default constructor.
             * Puts the SelectorKeys into a valid but undefined state.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            SelectorKeys() : len(0) {}
            /**
             * Copy constructor.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            SelectorKeys(const SelectorKeys& other);
            /**
             * Move assignment operator:
             * The source SelectorKeys will be left in a valid but undefined state.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            SelectorKeys& operator=(SelectorKeys&&) noexcept;
            /**
             * Copy assignment operator.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            SelectorKeys& operator=(const SelectorKeys& other);
            /**
             * Destructor.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            virtual ~SelectorKeys();

        private:
            friend class Builder;
            friend class message2::Checker;
            friend class message2::MessageFormatter;
            friend class message2::Serializer;

            /* const */ LocalArray<Key> keys;
            /* const */ int32_t len;

            const Key* getKeysInternal() const;
            SelectorKeys(const UVector& ks, UErrorCode& status);
        }; // class SelectorKeys


    } // namespace data_model


    namespace data_model {
        class Operator;

        /**
         *  An `Option` pairs an option name with an Operand.
         *
         * `Option` is immutable, copyable and movable.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        class U_I18N_API Option : public UObject {
        public:
            /**
             * Accesses the right-hand side of the option.
             *
             * @return A reference to the operand.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            const Operand& getValue() const { return rand; }
            /**
             * Accesses the left-hand side of the option.
             *
             * @return A reference to the option name.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            const UnicodeString& getName() const { return name; }
            /**
             * Constructor. Returns an `Option` representing the
             * named option "name=rand".
             *
             * @param n The name of the option.
             * @param r The value of the option.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Option(const UnicodeString& n, Operand&& r) : name(n), rand(std::move(r)) {}
            /**
             * Copy assignment operator
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Option& operator=(const Option& other);
            /**
             * Default constructor.
             * Returns an Option in a valid but undefined state.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Option() = default;
            /**
             * Copy constructor.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Option(const Option& other);
            /**
             * Move assignment operator
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Option& operator=(Option&& other) noexcept;
            /**
             * Destructor.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            virtual ~Option();
        private:
            /* const */ UnicodeString name;
            /* const */ Operand rand;
        }; // class Option

        // Internal only
        #ifndef U_IN_DOXYGEN
        // Options
        // This is a wrapper class around a vector of options that provides lookup operations
        class U_I18N_API OptionMap : public UObject {
        public:
            int32_t size() const;
            // Needs to take an error code b/c an earlier copy might have failed
            Option getOption(int32_t, UErrorCode&) const;

            OptionMap() : len(0) {}
            OptionMap(const OptionMap&);
            OptionMap& operator=(OptionMap&&);
            OptionMap& operator=(const OptionMap&);
        private:
            friend class message2::Serializer;
            friend class Operator;

            bool bogus = false;
            OptionMap(const UVector&, UErrorCode&);
            LocalArray<Option> options;
            int32_t len;
        }; // class OptionMap
        #endif

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
             * @return A vector of function options for this operator.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            std::vector<Option> getOptions() const {
                return toStdVector<Option>(options.options.getAlias(), options.len);
            }
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
                UVector* options; // Not a LocalPointer for the same reason as in `SelectorKeys::Builder`
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
                Builder& addOption(const UnicodeString &key, Operand&& value, UErrorCode& status) noexcept;
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
                Operator build(UErrorCode& status) const noexcept;
                /**
                 * Default constructor.
                 * Returns a Builder with no function name or reserved sequence set.
                 *
                 * @param status    Input/output error code.
                 *
                 * @internal ICU 75.0 technology preview
                 * @deprecated This API is for technology preview only.
                 */
                Builder(UErrorCode& status);
                /**
                 * Destructor.
                 *
                 * @internal ICU 75.0 technology preview
                 * @deprecated This API is for technology preview only.
                 */
                virtual ~Builder();
            }; // class Operator::Builder
            /**
             * Copy constructor.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Operator(const Operator& other) noexcept;
            /**
             * Copy assignment operator.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Operator& operator=(const Operator&) noexcept;
            /**
             * Move assignment operator:
             * The source Operator will be left in a valid but undefined state.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Operator& operator=(Operator&& other) noexcept;
            /**
             * Default constructor.
             * Puts the Operator into a valid but undefined state.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Operator() : isReservedSequence(true) {}
            /**
             * Destructor.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            virtual ~Operator();
        private:
            friend class message2::MessageFormatter;
            friend class message2::Serializer;

            // Function call constructor
            Operator(const FunctionName& f, const UVector& options, UErrorCode&);
            // Reserved sequence constructor
            Operator(const Reserved& r) : isReservedSequence(true), functionName(FunctionName(UnicodeString(""))), reserved(Reserved(r)) {}

            const OptionMap& getOptionsInternal() const;

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
                 * Default constructor.
                 * Returns a Builder with no operator or operand set.
                 *
                 * @internal ICU 75.0 technology preview
                 * @deprecated This API is for technology preview only.
                 */
                Builder() = default;
                /**
                 * Destructor.
                 *
                 * @internal ICU 75.0 technology preview
                 * @deprecated This API is for technology preview only.
                 */
                virtual ~Builder();
            }; // class Expression::Builder

            /**
             * Copy constructor.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Expression(const Expression& other);
            /**
             * Copy assignment operator.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Expression& operator=(const Expression&);
            /**
             * Move assignment operator:
             * The source Expression will be left in a valid but undefined state.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Expression& operator=(Expression&&) noexcept;
            /**
             * Default constructor.
             * Puts the Expression into a valid but undefined state.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Expression();
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

        class PatternPart;

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
            /**
             * Returns the size.
             *
             * @return The number of parts in the pattern.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            int32_t numParts() const { return len; }
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

                UVector* parts;  // Not a LocalPointer for the same reason as in `SelectorKeys::Builder`

            public:
                /**
                 * Adds a single part to the pattern. Copies `part`.
                 *
                 * @param part The part to be added.
                 * @param status Input/output error code.
                 * @return A reference to the builder.
                 *
                 * @internal ICU 75.0 technology preview
                 * @deprecated This API is for technology preview only.
                 */
                Builder& add(PatternPart&& part, UErrorCode& status) noexcept;
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
                Pattern build(UErrorCode& status) const noexcept;
                /**
                 * Default constructor.
                 * Returns a Builder with an empty sequence of PatternParts.
                 *
                 * @param status Input/output error code
                 *
                 * @internal ICU 75.0 technology preview
                 * @deprecated This API is for technology preview only.
                 */
                Builder(UErrorCode& status);
                /**
                 * Destructor.
                 *
                 * @internal ICU 75.0 technology preview
                 * @deprecated This API is for technology preview only.
                 */
                virtual ~Builder();
            }; // class Pattern::Builder

            /**
             * Default constructor.
             * Puts the Pattern into a valid but undefined state.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Pattern() : parts(LocalArray<PatternPart>()) {}
            /**
             * Copy constructor.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Pattern(const Pattern& other) noexcept;
            /**
             * Copy assignment operator
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Pattern& operator=(const Pattern& other) noexcept;
            /**
             * Move assignment operator:
             * The source Pattern will be left in a valid but undefined state.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Pattern& operator=(Pattern&& other) noexcept;

        private:
            friend class Builder;

            // Possibly-empty array of parts
            int32_t len = 0;
            LocalArray<PatternPart> parts;

            Pattern(const UVector& parts, UErrorCode& status);
            // Helper
            static void initParts(Pattern&, const Pattern&);
        }; // class Pattern

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
            /**
             * Copy constructor.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            PatternPart(const PatternPart& other);
            /**
             * Copy assignment operator.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            PatternPart& operator=(const PatternPart& other);
            /**
             * Move assignment operator:
             * The source PatternPart will be left in a valid but undefined state.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            PatternPart& operator=(PatternPart&&) noexcept;
            /**
             * Destructor.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            virtual ~PatternPart();
            /**
             * Text part constructor. Returns a text pattern part
             * with text `t`.
             *
             * @param t A text string.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            PatternPart(const UnicodeString& t) : isRawText(true), text(t) {}
            /**
             * Expression part constructor. Returns an Expression pattern
             * part with expression `e`.
             *
             * @param e An Expression.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            PatternPart(Expression&& e) : isRawText(false), expression(e) {}
            /**
             * Default constructor.
             * Puts the PatternPart into a valid but undefined state.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            PatternPart() = default;
        private:
            friend class Pattern::Builder;

            /* const */ bool isRawText = true;
            // Not used if !isRawText
            /* const */ UnicodeString text;
            // Not used if isRawText
            /* const */ Expression expression;
        }; // class PatternPart

        /**
         *  A `Variant` pairs a list of keys with a pattern
         * It corresponds to the `Variant` interface
         * defined in https://github.com/unicode-org/message-format-wg/tree/main/spec/data-model
         *
         * `Variant` is immutable, copyable and movable.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        class U_I18N_API Variant : public UObject {
        public:
            /**
             * Accesses the pattern of the variant.
             *
             * @return A reference to the pattern.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            const Pattern& getPattern() const { return p; }
            /**
             * Accesses the keys of the variant.
             *
             * @return A reference to the keys.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            const SelectorKeys& getKeys() const { return k; }
            /**
             * Constructor. Returns a variant that formats to `pattern`
             * when `keys` match the selector expressions in the enclosing
             * `match` construct.
             *
             * @param keys A reference to a `SelectorKeys`.
             * @param pattern A pattern (passed by move)
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Variant(const SelectorKeys& keys, Pattern&& pattern) : k(keys), p(std::move(pattern)) {}
            /**
             * Copy assignment operator
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Variant& operator=(const Variant& other);
            /**
             * Default constructor.
             * Returns a Variant in a valid but undefined state.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Variant() = default;
            /**
             * Move assignment operator
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Variant& operator=(Variant&&) noexcept;
            /**
             * Copy constructor.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Variant(const Variant&);
            /**
             * Destructor.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            virtual ~Variant();
        private:
            /* const */ SelectorKeys k;
            /* const */ Pattern p;
        }; // class Variant
    } // namespace data_model

        namespace data_model {
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
            /**
             * Constructor.
             * Precondition: i < numParts()
             *
             * @param v A variable name.
             * @param e An expression.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Binding(const VariableName& v, const Expression& e) : var(v), value(e){}
            /**
             * Copy constructor.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Binding(const Binding& other);
            /**
             * Copy assignment operator
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Binding& operator=(const Binding& other);
            /**
             * Default constructor.
             * Puts the Binding into a valid but undefined state.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Binding() = default;
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
    } // namespace data_model

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
        /**
         * Accesses the local variable declarations for this data model.
         *
         * @return A reference to a list of bindings for local variables.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        std::vector<Binding> getLocalVariables() const {
            if (bogus) {
                return std::vector<Binding>();
            }
            return toStdVector<Binding>(bindings.getAlias(), bindingsLen);
        }
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
        const std::vector<Expression> getSelectors() const {
            if (!hasSelectors()) {
                return {};
            }
            return toStdVector<Expression>(selectors.getAlias(), numSelectors);
        }
        /**
         * Accesses the variants.
         * Precondition: hasSelectors()
         *
         * @return A vector of variants.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        std::vector<Variant> getVariants() const {
            std::vector<Variant> result;

            if (hasSelectors()) {
                for (int32_t i = 0; i < numVariants; i++) {
                    result.push_back(variants[i]);
                }
            }
            return result;
        }
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
         * Default constructor.
         * Puts the MessageFormatDataModel into a valid but undefined state.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        MessageFormatDataModel();
        /**
         * Move assignment operator:
         * The source MessageFormatDataModel will be left in a valid but undefined state.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        MessageFormatDataModel& operator=(MessageFormatDataModel&&) noexcept;
        /**
         * Copy assignment operator:
         * The source MessageFormatDataModel will be left in a valid but undefined state.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        MessageFormatDataModel &operator=(const MessageFormatDataModel &);
        /**
         * Destructor.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        virtual ~MessageFormatDataModel();

        /**
         * The mutable `MessageFormatDataModel::Builder` class allows the data model to be
         * constructed incrementally. Builder is not copyable or movable.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        class U_I18N_API Builder : public UMemory {
        private:
            friend class MessageFormatDataModel;

            void buildSelectorsMessage(UErrorCode&);
            bool hasPattern = true;
            bool hasSelectors = false;
            Pattern pattern;
            // The following members are not LocalPointers for the same reason as in SelectorKeys::Builder
            UVector* selectors = nullptr;
            UVector* variants = nullptr;
            UVector* locals   = nullptr;

        public:
            /**
             * Adds a local variable declaration.
             *
             * @param variableName The variable name of the declaration.
             *                     Passed by move.
             * @param expression The expression to which `variableName` should be bound.
             *                   Passed by move.
             * @param status     Input/output error code
             * @return A reference to the builder.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Builder& addLocalVariable(VariableName&& variableName, Expression&& expression, UErrorCode& status) noexcept;
            /**
             * Adds a selector expression. Copies `expression`.
             * If a pattern was previously set, clears the pattern.
             *
             * @param selector Expression to add as a selector. Passed by move.
             * @param errorCode Input/output error code
             * @return A reference to the builder.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Builder& addSelector(Expression&& selector, UErrorCode& errorCode) noexcept;
            /**
             * Adds a single variant.
             * If a pattern was previously set using `setPattern()`, clears the pattern.
             *
             * @param keys Keys for the variant. Passed by move.
             * @param pattern Pattern for the variant. Passed by move.
             * @param errorCode Input/output error code
             * @return A reference to the builder.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Builder& addVariant(SelectorKeys&& keys, Pattern&& pattern, UErrorCode& errorCode) noexcept;
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
            MessageFormatDataModel build(UErrorCode& status) const noexcept;
            /**
             * Default constructor.
             * Returns a Builder with no pattern or selectors set.
             * Either `setPattern()` or both `addSelector()` and
             * `addVariant()` must be called before calling `build()`
             * on the resulting builder.
             *
             * @param status Input/output error code.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Builder(UErrorCode& status);
            /**
             * Destructor.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            virtual ~Builder();
        }; // class Builder

    private:
        friend class Checker;
        friend class MessageFormatter;
        friend class Serializer;

        bool hasPattern() const { return (numVariants == 0); }

        bool bogus = false; // Set if a copy constructor fails

        int32_t numVariants = 0;
        int32_t numSelectors = 0;

        // The expressions that are being matched on.
        // Ignored if `hasPattern`
        /* const */ LocalArray<Expression> selectors;

        // The list of `when` clauses (case arms).
        // Ignored if `hasPattern`
        /* const */ LocalArray<Variant> variants;

        // The pattern forming the body of the message.
        // Ignored if !hasPattern
        /* const */ Pattern pattern;

        // Bindings for local variables
        /* const */ LocalArray<Binding> bindings;
        int32_t bindingsLen = 0;

        const Binding* getLocalVariablesInternal() const;
        const Expression* getSelectorsInternal() const;
        const Variant* getVariantsInternal() const;

        // Helper
        void initBindings(const Binding*);

        MessageFormatDataModel(const Builder& builder, UErrorCode&) noexcept;
    }; // class MessageFormatDataModel

} // namespace message2

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */

#endif /* U_SHOW_CPLUSPLUS_API */

#endif // MESSAGEFORMAT_DATA_MODEL_H

#endif // U_HIDE_DEPRECATED_API
// eof

