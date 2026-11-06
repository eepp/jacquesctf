/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#ifndef _JACQUES_INSPECT_CMD_UI_VIEWS_PROGRESS_VIEW_HPP
#define _JACQUES_INSPECT_CMD_UI_VIEWS_PROGRESS_VIEW_HPP

#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <boost/filesystem/path.hpp>

#include "view.hpp"
#include "data/ds-file.hpp"
#include "data/pkt-index-entry.hpp"
#include "data/er.hpp"

namespace jacques {

/*
 * View which shows the progress of a long operation.
 *
 * A progress view shows, from top to bottom:
 *
 * 1. The subject of the operation: either a path (see subjectPath())
 *    or some text (see subjectText()).
 *
 * 2. A progress bar (see progress()).
 *
 * 3. Label/value rows (see rows()).
 */
class ProgressView final :
    public View
{
public:
    explicit ProgressView(const Rect& rect, std::string_view title, const Stylist& stylist);

    /*
     * Sets the subject of the operation to the path `path`.
     */
    void subjectPath(const boost::filesystem::path& path);

    /*
     * Sets the subject of the operation to the text `text`.
     */
    void subjectText(std::string_view text);

    /*
     * Sets the progress of the operation to `ratio`, between 0 and 1.
     */
    void progress(double ratio);

    /*
     * Sets the label/value rows to `rows`.
     */
    void rows(std::vector<LabelValRow> rows);

private:
    void _redrawContent() override;
    void _drawSubject() const;
    void _drawBar() const;

private:
    std::optional<boost::filesystem::path> _subjectPath;
    std::string _subjectText;
    double _ratio = 0;
    std::vector<LabelValRow> _rows;
};

/*
 * Makes `view` show the progress of building the packet index of the
 * data stream file `dsf`, `lastEntry` being the last indexed packet,
 * if any.
 */
void setPktIndexBuildProgress(ProgressView& view, const DsFile& dsf,
                              const PktIndexEntry *lastEntry);

/*
 * Makes `view` show the progress of creating the checkpoints of the
 * packet `pktIndexEntry`, `lastEr` being the last decoded event
 * record, if any.
 */
void setPktCheckpointsBuildProgress(ProgressView& view, const PktIndexEntry& pktIndexEntry,
                                    const Er *lastEr);

} // namespace jacques

#endif // _JACQUES_INSPECT_CMD_UI_VIEWS_PROGRESS_VIEW_HPP
