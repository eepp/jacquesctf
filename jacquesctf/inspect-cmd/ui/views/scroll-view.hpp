/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#ifndef _JACQUES_INSPECT_CMD_UI_VIEWS_SCROLL_VIEW_HPP
#define _JACQUES_INSPECT_CMD_UI_VIEWS_SCROLL_VIEW_HPP

#include <boost/variant.hpp>

#include "view.hpp"

namespace jacques {

class ScrollView :
    public View
{
protected:
    explicit ScrollView(const Rect& rect, const std::string& title, DecorationStyle decoStyle,
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
