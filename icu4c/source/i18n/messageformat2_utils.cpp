// © 2016 and later: Unicode, Inc. and others.
// License & terms of use: http://www.unicode.org/copyright.html

#include "unicode/utypes.h"

#if !UCONFIG_NO_FORMATTING

#include "unicode/messageformat2_data_model.h"
#include "unicode/messageformat2_utils.h"
#include "messageformat2_macros.h"

U_NAMESPACE_BEGIN namespace message2 {

using namespace data_model;

template<class K, class V>
int32_t OrderedMap<K, V>::size() const {
    return keys.size();
}

// Copy constructor
template<class K, class V>
OrderedMap<K, V>::OrderedMap(const OrderedMap<K, V>& other) noexcept : contents(other.contents), keys(other.keys) {}

// Default constructor
template<class K, class V>
OrderedMap<K, V>::OrderedMap() {}

// Precondition: `key` is not already in the map. (The caller must
// check this)
template<class K, class V>
typename OrderedMap<K, V>::Builder& OrderedMap<K, V>::Builder::add(const K& key, V&& value) noexcept {
    // Check that the key is not already in the map.
    // (If not for this check, the invariant that keys->size()
    // == contents->count() could be violated.)
    U_ASSERT(contents.count(key) == 0);
    keys.push_back(key);
    contents[key] = std::move(value);
    return *this;
}

// Precondition: `key` is not already in the map. (The caller must
// check this)
template<class K, class V>
typename OrderedMap<K, V>::Builder& OrderedMap<K, V>::Builder::add(K&& key, V&& value) noexcept {
    // Check that the key is not already in the map.
    // (If not for this check, the invariant that keys->size()
    // == contents->count() could be violated.)
    U_ASSERT(contents.count(key) == 0);

    keys.push_back(std::move(key));
    contents[key] = std::move(value);
    return *this;
}

// This is provided so that builders can check for duplicate keys
// (for example, adding duplicate options is an error)
template<class K, class V>
UBool OrderedMap<K, V>::Builder::has(const K& key) const {
    return contents.count(key) > 0;
}

// Copying `build()` (leaves `this` valid)
template<class K, class V>
OrderedMap<K, V> OrderedMap<K, V>::Builder::build() const noexcept {
    return OrderedMap(contents, keys);
}

// Only called by builder()
template<class K, class V>
OrderedMap<K, V>::Builder::Builder() {}

template<class K, class V>
OrderedMap<K, V>::OrderedMap(const std::map<K, V>& cs, const std::vector<K>& ks) noexcept : contents(cs), keys(ks) {
    // It would be an error if `cs` and `ks` had different sizes
    U_ASSERT(cs.size() == ks.size());
}

template<class K, class V>
const K& OrderedMap<K, V>::Iterator::first() const {
    U_ASSERT(pos < (int32_t) keys.size());
    return keys[pos];
}

template<class K, class V>
const V& OrderedMap<K, V>::Iterator::second() const {
    U_ASSERT(pos < (int32_t) keys.size());
    const K& k = keys[pos];
    return contents.at(k);
}

template<class K, class V>
typename OrderedMap<K, V>::Iterator& OrderedMap<K, V>::Iterator::operator++() {
    pos++;
    return *this;
}

template<class K, class V>
typename OrderedMap<K, V>::Iterator OrderedMap<K, V>::begin() const noexcept {
    return Iterator(0, contents, keys);
}

template<class K, class V>
typename OrderedMap<K, V>::Iterator OrderedMap<K, V>::end() const noexcept {
    return Iterator(keys.size(), contents, keys);
}

template<class K, class V>
OrderedMap<K, V>::Iterator::Iterator(int32_t p, const std::map<K, V>& c, const std::vector<K>& k) : pos(p), contents(c), keys(k) {}

template<class K, class V>
bool OrderedMap<K, V>::Iterator::operator==(const OrderedMap<K, V>::Iterator& rhs) const {
    return (pos == rhs.pos);
}

template<class K, class V>
bool OrderedMap<K, V>::Iterator::operator!=(const OrderedMap<K, V>::Iterator& rhs) const {
    return !(*this == rhs);
}


template<class K, class V>
OrderedMap<K, V>& OrderedMap<K, V>::operator=(OrderedMap<K, V>&& other) noexcept {
    this->~OrderedMap();

    contents = std::move(other.contents);
    keys = std::move(other.keys);

    return *this;
}

template<class K, class V>
OrderedMap<K, V>& OrderedMap<K, V>::operator=(const OrderedMap<K, V>& other) noexcept {
    if (this != &other) {
        this->~OrderedMap();

        contents = other.contents;
        keys = other.keys;
    }
    return *this;
}

// Declare all instantiations of OrderedMap and ImmutableVector that
// appear in the data model API
// See https://stackoverflow.com/a/495056/1407170
// (and see number_fluent.cpp for another example)
template class OrderedMap<UnicodeString, Operand>;
template class OrderedMap<SelectorKeys, Pattern>;

} // namespace message2

U_NAMESPACE_END

#endif /* #if !UCONFIG_NO_FORMATTING */

// eof

