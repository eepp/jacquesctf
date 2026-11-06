/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#include <algorithm>
#include <functional>
#include <optional>

#include "scroll-view.hpp"

namespace jacques {

ScrollView::ScrollView(const Rect& rect, const std::string_view title,
                       const DecorationStyle decoStyle, const Stylist& stylist) :
    View {rect, title, decoStyle, stylist}
{
}

void ScrollView::_rowCount(const Size count)
{
    this->_updateCounts(count, this->contentRect().h);
    this->_invalidate();
}

void ScrollView::_index(const Index index)
{
    this->_firstVisibleRow(std::min(index, this->_maxFirstVisibleRow()));
    this->_invalidate();
}

bool ScrollView::_handleKey(const Key key)
{
    const auto specialKey = key.special();

    if (!specialKey) {
        return false;
    }

    const auto changed = std::invoke([this, specialKey]() -> std::optional<bool> {
        switch (*specialKey) {
        case SpecialKey::UP:
            return this->_goUp();

        case SpecialKey::DOWN:
            return this->_goDown();

        case SpecialKey::PAGE_UP:
            return this->_pageUp();

        case SpecialKey::PAGE_DOWN:
            return this->_pageDown();

        case SpecialKey::HOME:
            return this->_showFirstPage();

        case SpecialKey::END:
            return this->_showLastPage();

        default:
            // not a scrolling key
            return std::nullopt;
        }
    });

    if (!changed) {
        return false;
    }

    if (*changed) {
        this->_invalidate();
    }

    return true;
}

void ScrollView::_redrawContent()
{
    this->_drawRows();
    this->_hasMoreTop(this->_index() > 0);
    this->_hasMoreBottom(this->_index() + this->contentRect().h < this->_rowCount());
}

void ScrollView::_resized()
{
    // keeps the current top row, if possible
    this->_updateCounts(this->_rowCount(), this->contentRect().h);
}

} // namespace jacques
