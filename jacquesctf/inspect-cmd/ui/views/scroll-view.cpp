/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <cassert>

#include "scroll-view.hpp"

namespace jacques {

ScrollView::ScrollView(const Rect& rect, const std::string& title, const DecorationStyle decoStyle,
                       const Stylist& stylist) :
    View {rect, title, decoStyle, stylist}
{
}

void ScrollView::prev()
{
    if (_myIndex == 0) {
        return;
    }

    --_myIndex;
    this->_drawRowsSetHasMore();
}

void ScrollView::next()
{
    if (_theRowCount - _myIndex <= this->contentRect().h) {
        return;
    }

    ++_myIndex;
    this->_drawRowsSetHasMore();
}

void ScrollView::pageDown()
{
    const auto maxIndex = _theRowCount - std::min(_theRowCount, this->contentRect().h);

    _myIndex = std::min(maxIndex, _myIndex + this->contentRect().h);
    this->_drawRowsSetHasMore();
}

void ScrollView::pageUp()
{
    _myIndex -= std::min(_myIndex, this->contentRect().h);
    this->_drawRowsSetHasMore();
}

void ScrollView::_redrawContent()
{
    this->_drawRowsSetHasMore();
}

void ScrollView::_resized()
{
    if (_theRowCount - _myIndex <= this->contentRect().h) {
        if (_theRowCount < this->contentRect().h) {
            _myIndex = 0;
        } else {
            _myIndex = _theRowCount - this->contentRect().h;
        }
    }
}

void ScrollView::_drawRowsSetHasMore()
{
    this->_drawRows();
    this->_hasMoreTop(_myIndex > 0);
    this->_hasMoreBottom(_myIndex + this->contentRect().h < _theRowCount);
}

} // namespace jacques
