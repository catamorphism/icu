// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#ifndef U_HIDE_DEPRECATED_API

#ifndef MESSAGEFORMAT_UTILS_H
#define MESSAGEFORMAT_UTILS_H

#include "unicode/utypes.h"

#if U_SHOW_CPLUSPLUS_API

#if !UCONFIG_NO_FORMATTING

#include <map>
#include <vector>

#include "unicode/localpointer.h"
#include "unicode/unistr.h"

union UElement;

U_NAMESPACE_BEGIN

namespace message2 {

// TODO update comment
/**
 * The `OrderedMap` class represents a polymorphic hash table with string
 * keys, constructed using the builder pattern. It's used to represent
 * various nodes in the MessageFormat data model that may have a
 * variable number of named components. The map records the order in which
 * keys were added and iterates over its elements in that order.
 * The template can't be instantiated with an arbitrary type;
 * explicit instantiations of it are exported only for the types
 * `Operand` and `Pattern`.
 *
 * The class is immutable, movable and copyable.
 *
 * @internal ICU 74.0 technology preview
 * @deprecated This API is for technology preview only.
 */
template<class K, class V>
class OrderedMap : public UObject {

public:
// TODO
    class Iterator {
        public:
        const K& first() const;
        const V& second() const;
        Iterator& operator++();
        bool operator==(const Iterator&) const;
        bool operator!=(const Iterator&) const;
        private:
        friend class OrderedMap<K, V>;

        int32_t pos;
        const std::map<K, V> contents;
        const std::vector<K> keys;
        Iterator(int32_t, const std::map<K, V>&, const std::vector<K>&);
    };

    Iterator begin() const noexcept;
    Iterator end() const noexcept;
    OrderedMap();

    /**
     * Size accessor.
     *
     * @return   The number of elements in this map.
     *
     * @internal ICU 74.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    int32_t size() const;

    /**
     * Copy constructor. Performs a deep copy (`V` must have
     * a copy constructor.)
     *
     * @param other   The OrderedMap to copy.
     *
     * @internal ICU 74.0 technology preview
     * @deprecated This API is for technology preview only.
     */
    OrderedMap(const OrderedMap& other);
// TODO
    OrderedMap& operator=(OrderedMap&&) noexcept;
    OrderedMap& operator=(const OrderedMap&);

    /**
     * The mutable `OrderedMap::Builder` class allows the map to be constructed
     * one key/value pair at a time. Builder is not copyable or movable.
     *
     * @internal ICU 74.0 technology preview
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
         * @internal ICU 74.0 technology preview
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
         * @internal ICU 74.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        Builder& add(K&& key, V&& value);
        /**
         * Checks if a key is in the map.
         *
         * @param key Reference to a key.
         * @return    True if and only if `key` is mapped to a value in the map.
         *
         * @internal ICU 74.0 technology preview
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
         * @internal ICU 74.0 technology preview
         * @deprecated This API is for technology preview only.
         */
        OrderedMap<K, V> build() const;
// TODO
        Builder();

        /**
         * Destructor.
         *
         * @internal ICU 74.0 technology preview
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
     * @internal ICU 74.0 technology preview
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

