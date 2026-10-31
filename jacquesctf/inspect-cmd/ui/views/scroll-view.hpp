/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#ifndef _JACQUES_INSPECT_CMD_UI_VIEWS_SCROLL_VIEW_HPP
#define _JACQUES_INSPECT_CMD_UI_VIEWS_SCROLL_VIEW_HPP

#include "view.hpp"

namespace jacques {

class ScrollView :
    public View
{
protected:
    explicit ScrollView(const Rect& rect, std::string_view title, DecorationStyle decoStyle,
                        const Stylist& stylist);

public:
    void prev();
    void next();
    void pageUp();
    void pageDown();

protected:
    virtual void _drawRows() = 0;
    void _resized() override;
    void _redrawContent() override;

    void _rowCount(const Size count) noexcept
    {
        _theRowCount = count;
    }

    Size _rowCount() const noexcept
    {
        return _theRowCount;
    }

    void _index(const Index index) noexcept
    {
        _myIndex = index;
    }

    Index _index() const noexcept
    {
        return _myIndex;
    }

    Index _contentRectYFromIndex(const Index index) const noexcept
    {
        return index - _myIndex;
    }

private:
    void _drawRowsSetHasMore();

private:
    Index _myIndex = 0;
    Size _theRowCount = 0;
};

} // namespace jacques

#endif // _JACQUES_INSPECT_CMD_UI_VIEWS_SCROLL_VIEW_HPP
