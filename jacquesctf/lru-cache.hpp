/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_LRU_CACHE_HPP
#define _JACQUES_LRU_CACHE_HPP

#include <cassert>
#include <unordered_map>
#include <utility>
#include <list>

#include "aliases.hpp"

namespace jacques {

/*
 * A simple, generic LRU cache, where values of type `ValueT` as
 * associated to keys of type `KeyT`.
 */
template <typename KeyT, typename ValueT>
class LruCache
{
public:
    /*
     * Builds an LRU cache which can contain at most `maxSize` elements.
     */
    LruCache(const Size maxSize) :
        _maxSize {maxSize}
    {
        assert(maxSize > 0);
    }

    // size of the cache (not its capacity)
    Size size() const noexcept
    {
        return _keyToEntryIter.size();
    }


    /*
     * Inserts an element within the cache, also making it the most
     * recently used, and possibly evicting the least recently used
     * element.
     */
    void insert(const KeyT& key, const ValueT& value)
    {
        if (this->size() == _maxSize) {
            // remove least recently used
            _keyToEntryIter.erase(_entries.back().first);
            _entries.pop_back();
        }

        assert(!this->contains(key));
        _entries.push_front({key, value});
        _keyToEntryIter.insert({key, std::begin(_entries)});
    }

    /*
     * Returns the cached value associated to the key `key`, returning
     * `nullptr` if this cache does not contain such a value. When a
     * value is returned, it is marked as the most recently used in this
     * cache.
     */
    const ValueT *get(const KeyT& key)
    {
        const auto mapIt = _keyToEntryIter.find(key);

        if (mapIt == std::end(_keyToEntryIter)) {
            return nullptr;
        }

        const auto& entryIter = mapIt->second;

        // put it back to the front (MRU)
        _entries.splice(std::begin(_entries), _entries, entryIter);
        return &entryIter->second;
    }

    /*
     * Returns whether or not this cache contains a value for which the
     * key is `key`.
     */
    bool contains(const KeyT& key) const
    {
        return _keyToEntryIter.find(key) != std::end(_keyToEntryIter);
    }

    // invalidates the cache: removes everything
    void invalidate()
    {
        _entries.clear();
        _keyToEntryIter.clear();
    }

    /*
     * Invalidates any cached value associated to the key `key` (removes
     * it from this cache).
     */
    void invalidate(const KeyT& key)
    {
        const auto mapIt = _keyToEntryIter.find(key);

        if (mapIt == std::end(_keyToEntryIter)) {
            return;
        }

        _entries.erase(mapIt->second);
        _keyToEntryIter.erase(mapIt);
    }

private:
    using Entry = std::pair<KeyT, ValueT>;
    using EntryList = std::list<Entry>;

private:
    Size _maxSize;

    // entries: least recently used is at the back
    EntryList _entries;

    // link from key to entry in the list above
    std::unordered_map<KeyT, typename EntryList::iterator> _keyToEntryIter;
};

} // namespace jacques

#endif // _JACQUES_LRU_CACHE_HPP
