/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_INSPECT_CMD_UI_VIEWS_PKT_INDEX_BUILD_PROGRESS_VIEW_HPP
#define _JACQUES_INSPECT_CMD_UI_VIEWS_PKT_INDEX_BUILD_PROGRESS_VIEW_HPP

#include <optional>

#include "view.hpp"
#include "data/ds-file.hpp"

namespace jacques {

class PktIndexBuildProgressView final :
    public View
{
public:
    explicit PktIndexBuildProgressView(const Rect& rect, const Stylist& stylist);
    void dsFile(const DsFile& dsf);
    void pktIndexEntry(const PktIndexEntry& entry);

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
    std::optional<Index> _seqNum = std::nullopt;
    const DsFile *_dsf = nullptr;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_CMD_UI_VIEWS_PKT_INDEX_BUILD_PROGRESS_VIEW_HPP
