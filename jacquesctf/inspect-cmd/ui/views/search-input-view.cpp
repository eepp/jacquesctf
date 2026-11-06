/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#include <cassert>
#include <cctype>
#include <cstring>
#include <functional>
#include <string>
#include <string_view>

#include "search-input-view.hpp"
#include "../stylist.hpp"
#include "unicode.hpp"

namespace jacques {

SearchInputView::SearchInputView(const Rect& rect, const Stylist& stylist) :
    InputView {rect, stylist}
{
}

void SearchInputView::_writeCh(Point& contentPos, const char32_t ch) const
{
    std::string utf8Ch;

    unicode::appendUtf8(utf8Ch, ch);
    contentPos.x = this->_write(contentPos, utf8Ch);
}

void SearchInputView::_drawFormatText(Point contentPos, const std::string& utf8Text)
{
    // work with code points
    const auto text = std::invoke([&utf8Text] {
        std::u32string text;
        std::string_view rest {utf8Text};

        while (!rest.empty()) {
            text += unicode::popFrontUtf8(rest);
        }

        return text;
    });

    auto it = text.begin();

    if (*it == '/') {
        this->_style(Stylist::Role::ACCENT, true);
        this->_writeCh(contentPos, *it);
        ++it;

        while (it != text.end()) {
            if (*it == '\\') {
                this->_style(Stylist::Role::STD, true);
                this->_writeCh(contentPos, *it);
                ++it;

                if (it != text.end()) {
                    this->_writeCh(contentPos, *it);
                    ++it;
                }

                continue;
            } else if (*it == '*') {
                this->_style(Stylist::Role::SYMBOL, true);
            } else {
                this->_style(Stylist::Role::STD);
            }

            this->_writeCh(contentPos, *it);
            ++it;
        }

        return;
    }

    if (it == text.end()) {
        return;
    }

    auto hasAddSub = false;

    if (*it == '+' || *it == '-') {
        this->_style(Stylist::Role::ACCENT, true);
        this->_writeCh(contentPos, *it);
        ++it;
        hasAddSub = true;
    }

    if (it == text.end()) {
        return;
    }

    this->_style(Stylist::Role::ACCENT, true);

    if (*it == '#') {
        this->_writeCh(contentPos, *it);
        ++it;

        if (it != text.end() && *it == '#') {
            this->_writeCh(contentPos, *it);
            ++it;
        }
    } else if (*it == ':') {
        this->_writeCh(contentPos, *it);
        ++it;

        if (it != text.end() && *it == '$') {
            this->_writeCh(contentPos, *it);
            ++it;
        }
    } else if (*it == '@') {
        this->_writeCh(contentPos, *it);
        ++it;
    } else if (*it == '$') {
        this->_writeCh(contentPos, *it);
        ++it;
    } else if (*it == '*') {
        this->_writeCh(contentPos, *it);
        ++it;

        if (it != text.end() && *it == '*') {
            this->_writeCh(contentPos, *it);
            ++it;
        }
    } else if (*it == '%' && !hasAddSub) {
        this->_writeCh(contentPos, *it);
        ++it;
    }

    this->_drawNumber(contentPos, it, text.end());
}

void SearchInputView::_drawCurText(const Point& contentPos, const std::string& text)
{
    if (text.empty()) {
        return;
    }

    this->_drawFormatText(contentPos, text);
}

namespace {

bool isHexDigit(const char32_t ch)
{
    return ch < 0x80 && std::isxdigit(static_cast<int>(ch));
}

bool isDecDigit(const char32_t ch)
{
    return (ch < 0x80 && std::isdigit(static_cast<int>(ch))) || ch == U',';
}

bool isOctDigit(const char32_t ch)
{
    return ch >= U'0' && ch <= U'7';
}

} // namespace

void SearchInputView::_drawNumber(Point& contentPos, std::u32string::const_iterator it,
                                  const std::u32string::const_iterator endIt)
{
    std::function<bool (char32_t)> checkDigitFunc = isDecDigit;

    if (it == endIt) {
        return;
    }

    this->_style(Stylist::Role::NUMBER);

    if (*it == '0') {
        this->_writeCh(contentPos, *it);
        ++it;
        checkDigitFunc = isOctDigit;

        if (it != endIt && (*it == 'x' || *it == 'X')) {
            this->_writeCh(contentPos, *it);
            ++it;
            checkDigitFunc = isHexDigit;
        }
    }

    while (it != endIt) {
        if (checkDigitFunc(*it)) {
            this->_style(Stylist::Role::NUMBER);
        } else {
            this->_style(Stylist::Role::ALERT, true);
        }

        this->_writeCh(contentPos, *it);
        ++it;
    }
}

void SearchInputView::animateBorder(const Index index)
{
    const auto borderChCount = (this->contentRect().h - 2) * 2 + this->contentRect().w * 2;
    const auto altChCount = index % (borderChCount + 1);

    this->_drawBorder();
    this->_style(Stylist::Role::INPUT_BORDER);

    Point pt {0, 0};

    enum class Direction {
        RIGHT,
        DOWN,
        LEFT,
        UP,
    } direction = Direction::RIGHT;

    for (Index i = 0; i < altChCount; ++i) {
        this->_write(pt, glyphs::BULLET);

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
