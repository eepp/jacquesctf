/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_PACKET_INDEX_BUILD_PROGRESS_VIEW_HPP
#define _JACQUES_PACKET_INDEX_BUILD_PROGRESS_VIEW_HPP

#include <boost/optional.hpp>

#include "view.hpp"
#include "data-stream-file.hpp"

namespace jacques {

class PacketIndexBuildProgressView :
    public View
{
public:
    explicit PacketIndexBuildProgressView(const Rectangle& rect,
                                          const Stylist& stylist);
    void dataStreamFile(const DataStreamFile& dsf);
    void packetIndexEntry(const PacketIndexEntry& entry);

protected:
    void _resized() override;
    void _redrawContent() override;

private:
    void _clearRow(Index y);
    void _drawFile();
    void _drawProgress();

private:
    Index _index = 0;
    Index _offsetBytes = 0;
    boost::optional<Index> _seqNum = boost::none;
    const DataStreamFile *_dsf = nullptr;
};

} // namespace jacques

#endif // _JACQUES_PACKET_INDEX_BUILD_PROGRESS_VIEW_HPP
