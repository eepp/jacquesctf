/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <cassert>
#include <cctype>
#include <cstring>
#include <cctype>

#include "search-input-view.hpp"
#include "../stylist.hpp"

namespace jacques {

SearchInputView::SearchInputView(const Rect& rect, const Stylist& stylist) :
    InputView {rect, stylist}
{
}

void SearchInputView::_drawFormatText(const std::string& text)
{
    auto it = text.begin();

    if (*it == '/') {
        this->_stylist().searchInputViewPrefix(*this);
        this->_appendChar(*it);
        ++it;

        while (it != text.end()) {
            if (*it == '\\') {
                this->_stylist().searchInputViewEscape(*this);
                this->_appendChar(*it);
                ++it;

                if (it != text.end()) {
                    this->_appendChar(*it);
                    ++it;
                }

                continue;
            } else if (*it == '*') {
                this->_stylist().searchInputViewWildcard(*this);
            } else {
                this->_stylist().std(*this);
            }

            this->_appendChar(*it);
            ++it;
        }

        return;
    }

    if (it == text.end()) {
        return;
    }

    auto hasAddSub = false;

    if (*it == '+' || *it == '-') {
        this->_stylist().searchInputViewAddSub(*this);
        this->_appendChar(*it);
        ++it;
        hasAddSub = true;
    }

    if (it == text.end()) {
        return;
    }

    this->_stylist().searchInputViewPrefix(*this);

    if (*it == '#') {
        this->_appendChar(*it);
        ++it;

        if (it != text.end() && *it == '#') {
            this->_appendChar(*it);
            ++it;
        }
    } else if (*it == ':') {
        this->_appendChar(*it);
        ++it;

        if (it != text.end() && *it == '$') {
            this->_appendChar(*it);
            ++it;
        }
    } else if (*it == '@') {
        this->_appendChar(*it);
        ++it;
    } else if (*it == '$') {
        this->_appendChar(*it);
        ++it;
    } else if (*it == '*') {
        this->_appendChar(*it);
        ++it;

        if (it != text.end() && *it == '*') {
            this->_appendChar(*it);
            ++it;
        }
    } else if (*it == '%' && !hasAddSub) {
        this->_appendChar(*it);
        ++it;
    }

    this->_drawNumber(it, text.end());
}

void SearchInputView::_drawCurText(const std::string& text)
{
    if (text.empty()) {
        return;
    }

    this->_drawFormatText(text);
}

namespace {

bool isHexDigit(const int ch)
{
    return std::isxdigit(ch);
}

bool isDecDigit(const int ch)
{
    return std::isdigit(ch) || ch == ',';
}

bool isOctDigit(const int ch)
{
    return std::isdigit(ch) && ch != '8' && ch != '9';
}

} // namespace

void SearchInputView::_drawNumber(std::string::const_iterator it,
                                  const std::string::const_iterator endIt)
{
    std::function<bool (int)> checkDigitFunc = isDecDigit;

    if (it == endIt) {
        return;
    }

    this->_stylist().searchInputViewNumber(*this);

    if (*it == '0') {
        this->_appendChar(*it);
        ++it;
        checkDigitFunc = isOctDigit;

        if (it != endIt && (*it == 'x' || *it == 'X')) {
            this->_appendChar(*it);
            ++it;
            checkDigitFunc = isHexDigit;
        }
    }

    while (it != endIt) {
        if (checkDigitFunc(*it)) {
            this->_stylist().searchInputViewNumber(*this);
        } else {
            this->_stylist().searchInputViewError(*this);
        }

        this->_appendChar(*it);
        ++it;
    }
}

void SearchInputView::animateBorder(const Index index)
{
    const auto borderChCount = (this->contentRect().h - 2) * 2 + this->contentRect().w * 2;
    const auto altChCount = index % (borderChCount + 1);

    this->_drawBorder();
    this->_stylist().simpleInputViewBorder(*this);

    Point pt {0, 0};

    enum class Direction {
        RIGHT,
        DOWN,
        LEFT,
        UP,
    } direction = Direction::RIGHT;

    for (Index i = 0; i < altChCount; ++i) {
        this->_putChar(pt, ACS_BULLET);

        switch (direction) {
        case Direction::RIGHT:
            ++pt.x;
            break;

        case Direction::DOWN:
            ++pt.y;
            break;

        case Direction::LEFT:
            --pt.x;
            break;

        case Direction::UP:
            --pt.y;
            break;
        }

        if (pt.x == 0) {
            if (pt.y == this->contentRect().h - 1) {
                direction = Direction::UP;
            }
        } else if (pt.x == this->contentRect().w - 1) {
            if (pt.y == 0) {
                direction = Direction::DOWN;
            } else if (pt.y == this->contentRect().h - 1) {
                direction = Direction::LEFT;
            }
        }
    }
}

} // namespace jacques
