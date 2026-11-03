/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#include <cassert>

#include "scroll-view.hpp"

namespace jacques {

ScrollView::ScrollView(const Rect& rect, const std::string_view title,
                       const DecorationStyle decoStyle, const Stylist& stylist) :
    View {rect, title, decoStyle, stylist}
{
}

void ScrollView::prev()
{
    if (_myIndex == 0) {
        return;
    }

    --_myIndex;
    this->_invalidate();
}

void ScrollView::next()
{
    if (_theRowCount - _myIndex <= this->contentRect().h) {
        return;
    }

    ++_myIndex;
    this->_invalidate();
}

void ScrollView::pageDown()
{
    const auto maxIndex = _theRowCount - std::min(_theRowCount, this->contentRect().h);

    _myIndex = std::min(maxIndex, _myIndex + this->contentRect().h);
    this->_invalidate();
}

void ScrollView::pageUp()
{
    _myIndex -= std::min(_myIndex, this->contentRect().h);
    this->_invalidate();
}

void ScrollView::_redrawContent()
{
    this->_drawRows();
    this->_hasMoreTop(_myIndex > 0);
    this->_hasMoreBottom(_myIndex + this->contentRect().h < _theRowCount);
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

} // namespace jacques
