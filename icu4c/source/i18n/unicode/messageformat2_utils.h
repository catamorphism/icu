// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#ifndef U_HIDE_DEPRECATED_API

#ifndef MESSAGEFORMAT_UTILS_H
#define MESSAGEFORMAT_UTILS_H

#include "unicode/utypes.h"

#if U_SHOW_CPLUSPLUS_API

#if !UCONFIG_NO_FORMATTING

#include "unicode/unistr.h"

#include <map>
#include <vector>

U_NAMESPACE_BEGIN

namespace message2 {

    /**
     * The `OrderedMap` class represents an immutable map that's polymorphic in
     * its keys and values, constructed using the builder pattern.
     * It's used to represent various nodes in the MessageFormat data model that may have a
     * variable number of named components. The map records the order in which
     * keys were added and iterates over its elements in that order.
     * The template can't be instantiated with an arbitrary type;
     * explicit instantiations of it are exported only for the types
     * (`UnicodeString`, `Operand`) and (`SelectorKeys`, `Pattern`).
     *
     * The class is immutable, movable and copyable.
     *
     * @internal ICU 75.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    template<class K, class V>
    class OrderedMap : public UObject {

    public:
        /**
         * The `OrderedMap::Iterator` class allows the keys and values of an `OrderedMap` to be iterated over.
         *
         * The class is mutable and is not movable or copyable.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        class Iterator {
        public:
            /**
             * Gets the key for the current iterator position. `this` must not be
             * `OrderedMap::iterator::end()`.
             *
             * @return The key of the key-value pair at the current position in iteration.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            const K& first() const;
            /**
             * Gets the value for the current iterator position.  `this` must not be
             * `OrderedMap::iterator::end()`.
             *
             * @return The value in the key-value pair at the current position in iteration.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            const V& second() const;
            /**
             * Advances the iterator to the next item.
             *
             * @return A reference to this iterator.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Iterator& operator++();
            /**
             * Equality operator. Used for comparing iterator references to
             * `OrderedMap::end()`. Results are undefined otherwise.
             *
             * @return True if both operands are `OrderedMap::end()`. False otherwise.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            bool operator==(const Iterator&) const;
            /**
             * Inequality operator. Used for comparing iterator references to
             * `OrderedMap::end()`. Results are undefined otherwise.
             *
             * @return True if one operand is `OrderedMap::end()` and the other is not.
             * False otherwise.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            bool operator!=(const Iterator&) const;

        private:
            friend class OrderedMap<K, V>;

            int32_t pos;
            const std::map<K, V> contents;
            const std::vector<K> keys;
            Iterator(int32_t, const std::map<K, V>&, const std::vector<K>&);
        };

        /**
         * Iterator begin. Returns an iterator positioned at the first value in the
         * map. (The order is determined by the order in which the keys were added.)
         * If the map is empty, returns `OrderedMap::end()`.
         *
         * @return An iterator starting at the first element, or `OrderedMap::end()`
         * if there are no elements.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        Iterator begin() const noexcept;

        /**
         * Iterator end. Returns a special value that an iterator can be compared to
         * in order to check if all elements have been exhausted.
         *
         * @return The iterator value indicating the end of iteration.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        Iterator end() const noexcept;

        /**
         * Size accessor.
         *
         * @return   The number of elements in this map.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        int32_t size() const;

        /**
         * Default constructor.
         * Puts the OrderedMap into a valid but undefined state.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        OrderedMap();

        /**
         * Copy constructor. Performs a deep copy (`V` must have
         * a copy constructor.)
         *
         * @param other   The OrderedMap to copy.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        OrderedMap(const OrderedMap& other);
        /**
         * Copy assignment operator. Performs a deep copy (`V` must have
         * a copy constructor.)
         *
         * @param other   The OrderedMap to copy.
         * @return A reference to `this`
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        OrderedMap& operator=(const OrderedMap&);
        /**
         * Move assignment operator. `V` must have a move constructor.
         * `other` is left in a valid but undefined state.
         *
         * @param other   The OrderedMap to move.
         * @return A reference to `this`
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        OrderedMap& operator=(OrderedMap&& other) noexcept;

        /**
         * The mutable `OrderedMap::Builder` class allows the map to be constructed
         * one key/value pair at a time. Builder is not copyable or movable.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        class Builder : public UMemory {
        public:
            /**
             * Adds to the map.
             * Precondition: !has(key)
             *
             * @param key    The name to be added. It is an internal error to
             *               call `add()` with a key that has already been added.
             * @param value  The value to be associated with the name. Passed by move.
             * @return A reference to the builder.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Builder& add(const K& key, V&& value);
            /**
             * Adds to the map.
             * Precondition: !has(key)
             *
             * @param key    The name to be added. It is an internal error to
             *               call `add()` with a key that has already been added.
             * @param value  The value to be associated with the name. Passed by move.
             * @return A reference to the builder.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Builder& add(K&& key, V&& value);
            /**
             * Checks if a key is in the map.
             *
             * @param key Reference to a key.
             * @return    True if and only if `key` is mapped to a value in the map.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            UBool has(const K& key) const;
            /**
             * Constructs a new `OrderedMap` using the keys and values
             * set with previous `add()` calls.
             *
             * The builder object (`this`) can still be used after calling `build()`.
             *
             * @param status    Input/output error code.
             * @return          The new OrderedMap, which is non-null if U_SUCCESS(status).
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            OrderedMap<K, V> build() const;
            /**
             * Default constructor.
             * Returns a Builder with no keys or values.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            Builder();
            /**
             * Destructor.
             *
             * @internal ICU 75.0 technology preview
             * @deprecated This API is for technology preview only.
             */
            virtual ~Builder();
        private:
            friend class OrderedMap;

            // The underlying map
            std::map<K, V> contents;
            // Maintain a list of keys that encodes the order in which
            // keys are added. (The STL has no equivalent to Java's standard
            // `OrderedMap` class.)
            std::vector<K> keys;
        }; // class OrderedMap<V>::Builder

        /**
         * Destructor.
         *
         * @internal ICU 75.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        virtual ~OrderedMap();

    private:
        // The underlying map
        std::map<K, V> contents;
        // List of keys in insertion order
        std::vector<K> keys;

        OrderedMap(const std::map<K, V>& cs, const std::vector<K>& ks);
    }; // class OrderedMap<V>

} // namespace message2

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */

#endif /* U_SHOW_CPLUSPLUS_API */

#endif // MESSAGEFORMAT_UTILS_H

#endif // U_HIDE_DEPRECATED_API
// eof

