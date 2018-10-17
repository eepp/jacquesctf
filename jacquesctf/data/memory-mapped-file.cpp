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

#include "memory-mapped-file.hpp"
#include "io-error.hpp"

namespace jacques {

namespace bfs = boost::filesystem;

MemoryMappedFile::MemoryMappedFile(const bfs::path& path,
                                   const boost::optional<int>& fd) :
    _path {path}
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

    _fileSize = DataSize::fromBytes(bfs::file_size(path));
    _mmapOffsetGranularityBytes = sysconf(_SC_PAGE_SIZE);
    assert(_mmapOffsetGranularityBytes >= 1);
}

MemoryMappedFile::~MemoryMappedFile()
{
    if (_fd >= 0 && _closeFd) {
        (void) close(_fd);
    }
}

void MemoryMappedFile::_unmap()
{
    if (_mmapAddr) {
        munmap(_mmapAddr, static_cast<size_t>(_mmapSize.bytes()));
        _mmapAddr = nullptr;
        _mmapSize = 0;
        _mapAddr = nullptr;
        _mapSize = 0;
        _mapOffsetBytes = 0;
    }
}

void MemoryMappedFile::map(const Index offsetBytes, const DataSize& size)
{
    this->_unmap();

    const auto mmapOffsetBytes = offsetBytes &
                                 ~(_mmapOffsetGranularityBytes - 1);
    const auto sizeBetweenOffsetsBytes = offsetBytes - mmapOffsetBytes;

    _mmapSize = DataSize::fromBytes(std::min(_fileSize.bytes() - mmapOffsetBytes,
                                             size.bytes() + sizeBetweenOffsetsBytes));

    if (_mmapSize == 0) {
        return;
    }

    _mmapAddr = mmap(NULL, static_cast<size_t>(_mmapSize.bytes()),
                     PROT_READ, MAP_PRIVATE, _fd,
                     static_cast<off_t>(mmapOffsetBytes));

    if (_mmapAddr == MAP_FAILED) {
        std::ostringstream ss;

        ss << "Cannot memory-map region [" << mmapOffsetBytes << ", " <<
              (mmapOffsetBytes + _mmapSize.bytes() - 1) << "] of file `" <<
              _path.string() << "`.";
        _mmapAddr = nullptr;
        _mmapSize = 0;
        throw IOError {_path, ss.str()};
    }

    _mapAddr = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(_mmapAddr) +
                                        sizeBetweenOffsetsBytes);
    _mapSize = DataSize::fromBytes(std::min(_fileSize.bytes() - mmapOffsetBytes,
                                            size.bytes()));
    _mapOffsetBytes = offsetBytes;
    this->_advice();
}

void MemoryMappedFile::_advice()
{
    if (_mmapAddr && _mmapSize > 0) {
        (void) madvise(_mmapAddr, static_cast<size_t>(_mmapSize.bytes()),
                       _mmapAdvice);
    }
}

void MemoryMappedFile::advice(const Advice advice)
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
