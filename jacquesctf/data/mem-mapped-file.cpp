/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <cassert>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sstream>

#include "mem-mapped-file.hpp"
#include "io-error.hpp"

namespace jacques {

namespace bfs = boost::filesystem;

MemMappedFile::MemMappedFile(bfs::path path, const boost::optional<int>& fd) :
    _path {std::move(path)}
{
    if (fd) {
        _fd = *fd;
    } else {
        _fd = open(path.string().c_str(), O_RDONLY);

        if (_fd < 0) {
            throw IOError {path, "Cannot open file."};
        }

        _closeFd = true;
    }

    _fileLen = DataLen::fromBytes(bfs::file_size(path));
    _mmapOffsetGranularityBytes = sysconf(_SC_PAGE_SIZE);
    assert(_mmapOffsetGranularityBytes >= 1);
}

MemMappedFile::~MemMappedFile()
{
    if (_fd >= 0 && _closeFd) {
        static_cast<void>(close(_fd));
    }
}

void MemMappedFile::_unmap()
{
    if (_mmapAddr) {
        munmap(_mmapAddr, static_cast<size_t>(_mmapLen.bytes()));
        _mmapAddr = nullptr;
        _mmapLen = 0;
        _mapAddr = nullptr;
        _mapLen = 0;
        _mapOffsetBytes = 0;
    }
}

void MemMappedFile::map(const Index offsetBytes, const DataLen& len)
{
    this->_unmap();

    const auto mmapOffsetBytes = offsetBytes & ~(_mmapOffsetGranularityBytes - 1);
    const auto lenBetweenOffsetsBytes = offsetBytes - mmapOffsetBytes;

    _mmapLen = DataLen::fromBytes(std::min(_fileLen.bytes() - mmapOffsetBytes,
                                           len.bytes() + lenBetweenOffsetsBytes));

    if (_mmapLen == 0) {
        return;
    }

    _mmapAddr = mmap(NULL, static_cast<size_t>(_mmapLen.bytes()), PROT_READ, MAP_PRIVATE, _fd,
                     static_cast<off_t>(mmapOffsetBytes));

    if (_mmapAddr == MAP_FAILED) {
        std::ostringstream ss;

        ss << "Cannot memory-map region [" << mmapOffsetBytes << ", " <<
              (mmapOffsetBytes + _mmapLen.bytes() - 1) << "] of file `" <<
              _path.string() << "`.";
        _mmapAddr = nullptr;
        _mmapLen = 0;
        throw IOError {_path, ss.str()};
    }

    _mapAddr = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(_mmapAddr) +
                                        lenBetweenOffsetsBytes);
    _mapLen = DataLen::fromBytes(std::min(_fileLen.bytes() - mmapOffsetBytes, len.bytes()));
    _mapOffsetBytes = offsetBytes;
    this->_advice();
}

void MemMappedFile::_advice()
{
    if (_mmapAddr && _mmapLen > 0) {
        static_cast<void>(madvise(_mmapAddr, static_cast<size_t>(_mmapLen.bytes()), _mmapAdvice));
    }
}

void MemMappedFile::advice(const Advice advice)
{
    switch (advice) {
    case Advice::NORMAL:
        _mmapAdvice = MADV_NORMAL;
        break;

    case Advice::RANDOM:
        _mmapAdvice = MADV_RANDOM;
        break;
    }

    this->_advice();
}

} // namespace jacques
