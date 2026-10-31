/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#ifndef _JACQUES_DATA_PKT_CHECKPOINTS_BUILD_LISTENER_HPP
#define _JACQUES_DATA_PKT_CHECKPOINTS_BUILD_LISTENER_HPP

#include <memory>
#include <yactfr/yactfr.hpp>

#include "er.hpp"
#include "pkt-index-entry.hpp"

namespace jacques {

class DsFile;

class PktCheckpointsBuildListener
{
protected:
    explicit PktCheckpointsBuildListener() noexcept = default;

public:
    virtual ~PktCheckpointsBuildListener() = default;

    void startBuild(const DsFile& dsFile, const PktIndexEntry& pktIndexEntry)
    {
        this->_startBuild(dsFile, pktIndexEntry);
    }

    void update(const Er& er)
    {
        this->_update(er);
    }

    void endBuild()
    {
        this->_endBuild();
    }

protected:
    virtual void _startBuild(const DsFile& dsFile, const PktIndexEntry& pktIndexEntry);
    virtual void _update(const Er& er);
    virtual void _endBuild();
};

} // namespace jacques

#endif // _JACQUES_DATA_PKT_CHECKPOINTS_BUILD_LISTENER_HPP
