/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#ifndef _JACQUES_DATA_MEM_MAPPED_FILE_HPP
#define _JACQUES_DATA_MEM_MAPPED_FILE_HPP

#include <string>
#include <cstdlib>
#include <optional>
#include <boost/filesystem.hpp>
#include <sys/mman.h>

#include "aliases.hpp"
#include "data-len.hpp"

namespace jacques {

class MemMappedFile final
{
public:
    explicit MemMappedFile(boost::filesystem::path path,
                           const std::optional<int>& fd = std::nullopt);
    ~MemMappedFile();

    MemMappedFile(const MemMappedFile&) = delete;
    MemMappedFile& operator=(const MemMappedFile&) = delete;

public:
    enum class Advice {
        NORMAL,
        RANDOM,
    };

public:
    void map(Index offsetBytes, const DataLen& len);
    void advice(Advice advice);

    const std::uint8_t *addr() const noexcept
    {
        return static_cast<std::uint8_t *>(_mapAddr);
    }

    Index offsetBytes() const noexcept
    {
        return _mapOffsetBytes;
    }

    const DataLen& len() const noexcept
    {
        return _mapLen;
    }

    const DataLen& fileLen() const noexcept
    {
        return _fileLen;
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
    DataLen _mmapLen = 0;
    int _mmapAdvice = MADV_NORMAL;
    int _fd = -1;
    bool _closeFd = false;
    DataLen _fileLen;
    Index _mmapOffsetGranularityBytes;
    void *_mapAddr = nullptr;
    DataLen _mapLen = 0;
    Index _mapOffsetBytes = 0;
};

} // namespace jacques

#endif // _JACQUES_DATA_MEM_MAPPED_FILE_HPP
