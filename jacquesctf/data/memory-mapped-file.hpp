/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_MEMORY_MAPPED_FILE_HPP
#define _JACQUES_MEMORY_MAPPED_FILE_HPP

#include <string>
#include <cstdlib>
#include <boost/core/noncopyable.hpp>
#include <boost/optional.hpp>
#include <boost/filesystem.hpp>
#include <sys/mman.h>

#include "aliases.hpp"
#include "data-size.hpp"

namespace jacques {

class MemoryMappedFile :
    boost::noncopyable
{
public:
    explicit MemoryMappedFile(const boost::filesystem::path& path,
                              const boost::optional<int>& fd = boost::none);
    ~MemoryMappedFile();

public:
    enum class Advice {
        NORMAL,
        RANDOM,
    };

public:
    void map(Index offsetBytes, const DataSize& size);
    void advice(Advice advice);

    const std::uint8_t *addr() const noexcept
    {
        return static_cast<std::uint8_t *>(_mapAddr);
    }

    Index offsetBytes() const noexcept
    {
        return _mapOffsetBytes;
    }

    const DataSize& size() const noexcept
    {
        return _mapSize;
    }

    const DataSize& fileSize() const noexcept
    {
        return _fileSize;
    }

    const boost::filesystem::path& path() const noexcept
    {
        return _path;
    }

private:
    void _unmap();
    void _advice();

private:
    const boost::filesystem::path _path;
    void *_mmapAddr = nullptr;
    DataSize _mmapSize = 0;
    int _mmapAdvice = MADV_NORMAL;
    int _fd = -1;
    bool _closeFd = false;
    DataSize _fileSize;
    Index _mmapOffsetGranularityBytes;
    void *_mapAddr = nullptr;
    DataSize _mapSize = 0;
    Index _mapOffsetBytes = 0;
};

} // namespace jacques

#endif // _JACQUES_MEMORY_MAPPED_FILE_HPP
