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
#include <boost/core/noncopyable.hpp>

#include "aliases.hpp"

namespace jacques {

/*
 * A simple, generic LRU cache, where values of type `ValT` as
 * associated to keys of type `KeyT`.
 */
template <typename KeyT, typename ValT>
class LruCache final :
    boost::noncopyable
{
public:
    /*
     * Builds an LRU cache which can contain at most `maxSize` elements.
     */
    explicit LruCache(const Size maxSize) :
        _maxSize {maxSize}
    {
        assert(maxSize > 0);
    }

    // size of the cache (not its capacity)
    Size size() const noexcept
    {
        return _keyToEntryIt.size();
    }


    /*
     * Inserts an element within the cache, also making it the most
     * recently used, and possibly evicting the least recently used
     * element.
     */
    void insert(KeyT key, ValT val)
    {
        if (this->size() == _maxSize) {
            // remove least recently used
            _keyToEntryIt.erase(_entries.back().first);
            _entries.pop_back();
        }

        assert(!this->contains(key));
        _entries.push_front(std::make_pair(key, std::move(val)));
        _keyToEntryIt.insert(std::make_pair(std::move(key), _entries.begin()));
    }

    /*
     * Returns the cached value associated to the key `key`, returning
     * `nullptr` if this cache doesn't contain such a value.
     *
     * When this method returns a value, it's marked as the most
     * recently used in this cache.
     */
    const ValT *get(const KeyT& key)
    {
        const auto mapIt = _keyToEntryIt.find(key);

        if (mapIt == _keyToEntryIt.end()) {
            return nullptr;
        }

        const auto& entryIter = mapIt->second;

        // put it back to the front (MRU)
        _entries.splice(_entries.begin(), _entries, entryIter);
        return &entryIter->second;
    }

    /*
     * Returns whether or not this cache contains a value for which the
     * key is `key`.
     */
    bool contains(const KeyT& key) const
    {
        return _keyToEntryIt.find(key) != _keyToEntryIt.end();
    }

    // invalidates the cache: removes everything
    void invalidate()
    {
        _entries.clear();
        _keyToEntryIt.clear();
    }

    /*
     * Invalidates any cached value associated to the key `key` (removes
     * it from this cache).
     */
    void invalidate(const KeyT& key)
    {
        const auto mapIt = _keyToEntryIt.find(key);

        if (mapIt == _keyToEntryIt.end()) {
            return;
        }

        _entries.erase(mapIt->second);
        _keyToEntryIt.erase(mapIt);
    }

private:
    using Entry = std::pair<KeyT, ValT>;
    using EntryList = std::list<Entry>;

private:
    Size _maxSize;

    // entries: least recently used is at the back
    EntryList _entries;

    // link from key to entry in the list above
    std::unordered_map<KeyT, typename EntryList::iterator> _keyToEntryIt;
};

} // namespace jacques

#endif // _JACQUES_LRU_CACHE_HPP
