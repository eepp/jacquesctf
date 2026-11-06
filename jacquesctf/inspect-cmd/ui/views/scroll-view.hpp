/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#ifndef _JACQUES_INSPECT_CMD_UI_VIEWS_SCROLL_VIEW_HPP
#define _JACQUES_INSPECT_CMD_UI_VIEWS_SCROLL_VIEW_HPP

#include "inspect-common/common-inspect-table-view.hpp"
#include "view.hpp"

namespace jacques {

/*
 * Base class of a view which shows rows and scrolls them with Up,
 * Down, Page Up, Page Down, Home, and End (see _handleKey()).
 *
 * The scrolling logic is the one of a table view without a selection
 * (see `CommonInspectTableView`).
 */
class ScrollView :
    public View,
    private CommonInspectTableView
{
protected:
    explicit ScrollView(const Rect& rect, std::string_view title, DecorationStyle decoStyle,
                        const Stylist& stylist);

    virtual void _drawRows() = 0;
    void _resized() override;
    void _redrawContent() override;
    bool _handleKey(Key key) override;

    /*
     * Sets the row count to `count`.
     *
     * If the row count changes, then this view shows the first row
     * at the top.
     */
    void _rowCount(Size count);

    Size _rowCount() const noexcept
    {
        return this->_totalRowCount();
    }

    /*
     * Shows the row `index` at the top (or the last page if there
     * aren't enough rows after it).
     */
    void _index(Index index);

    /*
     * Index of the row at the top.
     */
    Index _index() const noexcept
    {
        return this->_firstVisibleRow();
    }

    Index _contentRectYFromIndex(const Index index) const noexcept
    {
        return index - this->_firstVisibleRow();
    }
};

} // namespace jacques

#endif // _JACQUES_INSPECT_CMD_UI_VIEWS_SCROLL_VIEW_HPP
