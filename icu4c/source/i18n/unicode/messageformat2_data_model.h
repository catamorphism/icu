// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#include "unicode/utypes.h"

#ifndef U_HIDE_DEPRECATED_API

#ifndef MESSAGEFORMAT_DATA_MODEL_H
#define MESSAGEFORMAT_DATA_MODEL_H

#if U_SHOW_CPLUSPLUS_API

#if !UCONFIG_NO_FORMATTING

#include "unicode/localpointer.h"
#include "unicode/messageformat2_data_model_names.h"

#include <algorithm>
#include <optional>
#include <variant>
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

    namespace data_model {
        class Literal;
        class Operator;
    } // namespace data_model
} // namespace message2

/// @cond DOXYGEN_IGNORE
// Export an explicit template instantiation of the LocalPointer that is used as a
// data member of various MessageFormatDataModel classes.
// (When building DLLs for Windows this is required.)
// (See measunit_impl.h, datefmt.h, collationiterator.h, erarules.h and others
// for similar examples.)
#if U_PF_WINDOWS <= U_PLATFORM && U_PLATFORM <= U_PF_CYGWIN
#if defined(_MSC_VER)
// Ignore warning 4661 as LocalPointerBase does not use operator== or operator!=
#pragma warning(push)
#pragma warning(disable: 4661)
#endif
template class U_I18N_API LocalPointerBase<message2::data_model::Literal>;
template class U_I18N_API LocalArray<message2::data_model::Literal>;
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
#endif
/// @endcond

namespace message2 {
    class Checker;
    class MessageFormatDataModel;
    class MessageFormatter;
    class Serializer;


  namespace data_model {
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
             * Non-member swap function.
             * @param r1 will get r2's contents
             * @param r2 will get r1's contents
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            friend inline void swap(Reserved& r1, Reserved& r2) noexcept {
                using std::swap;

                swap(r1.parts, r2.parts);
                swap(r1.len, r2.len);
            }
            /**
             * Copy constructor.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Reserved(const Reserved& other);
            /**
             * Assignment operator
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Reserved& operator=(Reserved) noexcept;
            /**
             * Default constructor.
             * Puts the Reserved into a valid but undefined state.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Reserved() { parts = LocalArray<Literal>(); }
            /**
             * Destructor.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            virtual ~Reserved();
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
             * Non-member swap function.
             * @param l1 will get l2's contents
             * @param l2 will get l1's contents
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            friend inline void swap(Literal& l1, Literal& l2) noexcept {
                using std::swap;

                swap(l1.thisIsQuoted, l2.thisIsQuoted);
                swap(l1.contents, l2.contents);
            }
            /**
             * Assignment operator.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Literal& operator=(Literal) noexcept;
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
            virtual UBool isNull() const;
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
            Operand() : contents(std::nullopt) {}
            /**
             * Variable operand constructor.
             *
             * @param v The variable name; an operand corresponding
             *        to a reference to `v` is returned.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            explicit Operand(const VariableName& v) : contents(v) {}
            /**
             * Literal operand constructor.
             *
             * @param l The literal to use for this operand; an operand
             *        corresponding to `l` is returned.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            explicit Operand(const Literal& l) : contents(l) {}
            /**
             * Non-member swap function.
             * @param o1 will get o2's contents
             * @param o2 will get o1's contents
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            friend inline void swap(Operand& o1, Operand& o2) noexcept {
                using std::swap;
                (void) o1;
                (void) o2;
                swap(o1.contents, o2.contents);
            }
            /**
             * Assignment operator.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            virtual Operand& operator=(Operand) noexcept;
            /**
             * Copy constructor.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Operand(const Operand&);
            /**
             * Destructor.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            virtual ~Operand();
        private:
            std::optional<std::variant<VariableName, Literal>> contents;
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
            UBool isWildcard() const { return !contents.has_value(); }
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
            Key(const Key& other) : contents(other.contents) {}
            /**
             * Wildcard constructor; constructs a Key representing the
             * catchall or wildcard key, '*'.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Key() : contents(std::nullopt) {}
            /**
             * Literal key constructor.
             *
             * @param lit A Literal to use for this key. The result matches the
             *        literal `lit`.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            explicit Key(const Literal& lit) : contents(lit) {}
            /**
             * Non-member swap function.
             * @param k1 will get k2's contents
             * @param k2 will get k1's contents
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            friend inline void swap(Key& k1, Key& k2) noexcept {
                using std::swap;

                swap(k1.contents, k2.contents);
            }
            /**
             * Assignment operator
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Key& operator=(Key) noexcept;
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
            /* const */ std::optional<Literal> contents;
        }; // class Key
  } // namespace data_model
} // namespace message2

/// @cond DOXYGEN_IGNORE
// Export an explicit template instantiation of the LocalPointer that is used as a
// data member of various MessageFormatDataModel classes.
// (When building DLLs for Windows this is required.)
// (See measunit_impl.h, datefmt.h, collationiterator.h, erarules.h and others
// for similar examples.)
#if U_PF_WINDOWS <= U_PLATFORM && U_PLATFORM <= U_PF_CYGWIN
#if defined(_MSC_VER)
// Ignore warning 4661 as LocalPointerBase does not use operator== or operator!=
#pragma warning(push)
#pragma warning(disable: 4661)
#endif
template class U_I18N_API LocalPointerBase<message2::data_model::Key>;
template class U_I18N_API LocalArray<message2::data_model::Key>;
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
#endif
/// @endcond

namespace message2 {
  namespace data_model {
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
             * Non-member swap function.
             * @param s1 will get s2's contents
             * @param s2 will get s1's contents
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            friend inline void swap(SelectorKeys& s1, SelectorKeys& s2) noexcept {
                using std::swap;

                swap(s1.len, s2.len);
                swap(s1.keys, s2.keys);
            }
            /**
             * Copy constructor.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            SelectorKeys(const SelectorKeys& other);
            /**
             * Assignment operator.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            SelectorKeys& operator=(SelectorKeys other) noexcept;
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

        typedef std::pair<UnicodeString, Operand> Option;
    } // namespace data_model
} // namespace message2

  /// @cond DOXYGEN_IGNORE
// Export an explicit template instantiation of the LocalPointer that is used as a
// data member of various MessageFormatDataModel classes.
// (When building DLLs for Windows this is required.)
// (See measunit_impl.h, datefmt.h, collationiterator.h, erarules.h and others
// for similar examples.)
#if U_PF_WINDOWS <= U_PLATFORM && U_PLATFORM <= U_PF_CYGWIN
#if defined(_MSC_VER)
// Ignore warning 4661 as LocalPointerBase does not use operator== or operator!=
#pragma warning(push)
#pragma warning(disable: 4661)
#endif
template class U_I18N_API LocalPointerBase<message2::data_model::Option>;
template class U_I18N_API LocalArray<message2::data_model::Option>;
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
#endif
/// @endcond

namespace message2 {
  namespace data_model {
        // Internal only
        #ifndef U_IN_DOXYGEN
        // Options
        // This is a wrapper class around a vector of options that provides lookup operations
        class U_I18N_API OptionMap : public UObject {
        public:
            int32_t size() const;
            // Needs to take an error code b/c an earlier copy might have failed
            Option getOption(int32_t, UErrorCode&) const;

            friend inline void swap(OptionMap& m1, OptionMap& m2) noexcept {
                using std::swap;

                swap(m1.options, m2.options);
                swap(m1.len, m2.len);
            }
            OptionMap() : len(0) {}
            OptionMap(const OptionMap&);
            OptionMap& operator=(OptionMap);
        private:
            friend class message2::Serializer;
            friend class Operator;

            bool bogus = false;
            OptionMap(const UVector&, UErrorCode&);
            LocalArray<Option> options;
            int32_t len;
        }; // class OptionMap
        #endif

      // Internal use only
      typedef std::pair<FunctionName, OptionMap> Callable;

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
            UBool isReserved() const { return std::holds_alternative<Reserved>(contents); }
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
                const Callable* f = std::get_if<Callable>(&contents);
                // This case should never happen, as the precondition is !isReserved()
                if (f == nullptr) { return {}; }
                const OptionMap& opts = f->second;
                return toStdVector<Option>(opts.options.getAlias(), opts.len);
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
             * Non-member swap function.
             * @param o1 will get o2's contents
             * @param o2 will get o1's contents
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            friend inline void swap(Operator& o1, Operator& o2) noexcept {
                using std::swap;

                swap(o1.contents, o2.contents);
            }
            /**
             * Assignment operator.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Operator& operator=(Operator) noexcept;
            /**
             * Default constructor.
             * Puts the Operator into a valid but undefined state.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Operator() : contents(Reserved()) {}
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
            Operator(const Reserved& r) : contents(r) {}

            const OptionMap& getOptionsInternal() const;

            /* const */ std::variant<Reserved, Callable> contents;
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
             * Non-member swap function.
             * @param e1 will get e2's contents
             * @param e2 will get e1's contents
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            friend inline void swap(Expression& e1, Expression& e2) noexcept {
                using std::swap;

                swap(e1.rator, e2.rator);
                swap(e1.rand, e2.rand);
            }
            /**
             * Copy constructor.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Expression(const Expression& other);
            /**
             * Assignment operator.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Expression& operator=(Expression) noexcept;
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

            Expression(const Operator &rAtor, const Operand &rAnd) : rator(rAtor), rand(rAnd) {}
            Expression(const Operand &rAnd) : rator(std::nullopt), rand(Operand(rAnd)) {}
            Expression(const Operator &rAtor) : rator(rAtor), rand() {}
            /* const */ std::optional<Operator> rator;
            /* const */ Operand rand;
        }; // class Expression

        class PatternPart;
  } // namespace data_model
} // namespace message2

  /// @cond DOXYGEN_IGNORE
// Export an explicit template instantiation of the LocalPointer that is used as a
// data member of various MessageFormatDataModel classes.
// (When building DLLs for Windows this is required.)
// (See measunit_impl.h, datefmt.h, collationiterator.h, erarules.h and others
// for similar examples.)
#if U_PF_WINDOWS <= U_PLATFORM && U_PLATFORM <= U_PF_CYGWIN
#if defined(_MSC_VER)
// Ignore warning 4661 as LocalPointerBase does not use operator== or operator!=
#pragma warning(push)
#pragma warning(disable: 4661)
#endif
template class U_I18N_API LocalPointerBase<message2::data_model::PatternPart>;
template class U_I18N_API LocalArray<message2::data_model::PatternPart>;
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
#endif
/// @endcond

namespace message2 {
  namespace data_model {
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
             * Non-member swap function.
             * @param p1 will get p2's contents
             * @param p2 will get p1's contents
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            friend inline void swap(Pattern& p1, Pattern& p2) noexcept {
                using std::swap;

                swap(p1.len, p2.len);
                swap(p1.parts, p2.parts);
            }
            /**
             * Copy constructor.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Pattern(const Pattern& other) noexcept;
            /**
             * Assignment operator
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Pattern& operator=(Pattern) noexcept;
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
            UBool isText() const { return std::holds_alternative<UnicodeString>(piece); }
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
             * Non-member swap function.
             * @param p1 will get p2's contents
             * @param p2 will get p1's contents
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            friend inline void swap(PatternPart& p1, PatternPart& p2) noexcept {
                using std::swap;

                swap(p1.piece, p2.piece);
            }
            /**
             * Copy constructor.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            PatternPart(const PatternPart& other);
            /**
             * Assignment operator.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            PatternPart& operator=(PatternPart) noexcept;
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
            explicit PatternPart(const UnicodeString& t) : piece(t) {}
            /**
             * Expression part constructor. Returns an Expression pattern
             * part with expression `e`.
             *
             * @param e An Expression.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            explicit PatternPart(Expression&& e) : piece(e) {}
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

            std::variant<UnicodeString, Expression> piece;
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
             * Non-member swap function.
             * @param v1 will get v2's contents
             * @param v2 will get v1's contents
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            friend inline void swap(Variant& v1, Variant& v2) noexcept {
                using std::swap;

                swap(v1.k, v2.k);
                swap(v1.p, v2.p);
            }
            /**
             * Assignment operator
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Variant& operator=(Variant other) noexcept;
            /**
             * Default constructor.
             * Returns a Variant in a valid but undefined state.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Variant() = default;
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

      typedef std::pair<VariableName, Expression> Binding;

    } // namespace data_model
} // namespace message2

  /// @cond DOXYGEN_IGNORE
// Export an explicit template instantiation of the LocalPointer that is used as a
// data member of various MessageFormatDataModel classes.
// (When building DLLs for Windows this is required.)
// (See measunit_impl.h, datefmt.h, collationiterator.h, erarules.h and others
// for similar examples.)
#if U_PF_WINDOWS <= U_PLATFORM && U_PLATFORM <= U_PF_CYGWIN
#if defined(_MSC_VER)
// Ignore warning 4661 as LocalPointerBase does not use operator== or operator!=
#pragma warning(push)
#pragma warning(disable: 4661)
#endif
template class U_I18N_API LocalPointerBase<message2::data_model::Expression>;
template class U_I18N_API LocalPointerBase<message2::data_model::Variant>;
template class U_I18N_API LocalPointerBase<message2::data_model::Binding>;
template class U_I18N_API LocalArray<message2::data_model::Expression>;
template class U_I18N_API LocalArray<message2::data_model::Variant>;
template class U_I18N_API LocalArray<message2::data_model::Binding>;
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
#endif
/// @endcond

namespace message2 {
    using namespace data_model;


    // Internal only

    class MessageFormatDataModel;

    class Matcher {
    private:

        friend class MessageFormatDataModel;

        Matcher(Expression* ss, int32_t ns, Variant* vs, int32_t nv) : selectors(ss), numSelectors(ns), variants(vs), numVariants(nv) {
            if (selectors == nullptr) {
                numSelectors = 0;
            }
            if (variants == nullptr) {
                numVariants = 0;
            }
        }
        Matcher() {}
        // The expressions that are being matched on.
        LocalArray<Expression> selectors;
        // The number of selectors
        int32_t numSelectors = 0;
        // The list of `when` clauses (case arms).
        LocalArray<Variant> variants;
        // The number of variants
        int32_t numVariants = 0;
    }; // class Matcher

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
            const Matcher* match = std::get_if<Matcher>(&body);
            // Should never happen, given the check for hasSelectors()
            if (match == nullptr) {
                return {};
            }
            return toStdVector<Expression>(match->selectors.getAlias(), match->numSelectors);
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
            if (hasSelectors()) {
                const Matcher* match = std::get_if<Matcher>(&body);
                // Should never happen, given the check for hasSelectors()
                if (match == nullptr) {
                    return {};
                }
                return toStdVector<Variant>(match->variants.getAlias(), match->numVariants);
            }
            return {};
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
         * Non-member swap function.
         * @param m1 will get m2's contents
         * @param m2 will get m1's contents
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        friend inline void swap(MessageFormatDataModel& m1, MessageFormatDataModel& m2) noexcept {
            using std::swap;

            if (m1.bogus) {
                m2.bogus = true;
                return;
            }
            if (m2.bogus) {
                m1.bogus = true;
                return;
            }
            swap(m1.body, m2.body);
            swap(m1.bindings, m2.bindings);
            swap(m1.bindingsLen, m2.bindingsLen);
        }
        /**
         * Assignment operator
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        MessageFormatDataModel& operator=(MessageFormatDataModel) noexcept;
        /**
         * Copy constructor.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        MessageFormatDataModel(const MessageFormatDataModel& other);
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

        bool hasPattern() const { return std::holds_alternative<Pattern>(body); }

        bool bogus = false; // Set if a copy constructor fails

        // A message body is either a matcher (selector list and variant list),
        // or a single pattern
        std::variant<Matcher, Pattern> body;

        // Bindings for local variables
        /* const */ LocalArray<Binding> bindings;
        int32_t bindingsLen = 0;

        const Binding* getLocalVariablesInternal() const;
        const Expression* getSelectorsInternal() const;
        const Variant* getVariantsInternal() const;
        int32_t numSelectors() const {
            const Matcher* matcher = std::get_if<Matcher>(&body);
            return (matcher == nullptr ? 0 : matcher->numSelectors);
        }
        int32_t numVariants() const {
            const Matcher* matcher = std::get_if<Matcher>(&body);
            return (matcher == nullptr ? 0 : matcher->numVariants);
        }

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

