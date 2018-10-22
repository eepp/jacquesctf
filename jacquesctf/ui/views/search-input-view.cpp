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
#include "stylist.hpp"

namespace jacques {

SearchInputView::SearchInputView(const Rectangle& rect,
                                 const Stylist& stylist) :
    InputView {rect, stylist}
{
}

void SearchInputView::_drawFormatText(const std::string& text)
{
    auto it = std::begin(text);
    const auto endIt = std::end(text);

    if (*it == '/') {
        this->_stylist().searchInputViewPrefix(*this);
        this->_appendChar(*it);
        ++it;

        while (it != endIt) {
            if (*it == '\\') {
                this->_stylist().searchInputViewEscape(*this);
                this->_appendChar(*it);
                ++it;

                if (it != endIt) {
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

    if (it == endIt) {
        return;
    }

    bool hasAddSub = false;

    if (*it == '+' || *it == '-') {
        this->_stylist().searchInputViewAddSub(*this);
        this->_appendChar(*it);
        ++it;
        hasAddSub = true;
    }

    if (it == endIt) {
        return;
    }

    this->_stylist().searchInputViewPrefix(*this);

    if (*it == '#') {
        this->_appendChar(*it);
        ++it;

        if (it != endIt && *it == '#') {
            this->_appendChar(*it);
            ++it;

            if (it != endIt && *it == '#') {
                this->_appendChar(*it);
                ++it;
            }
        }
    } else if (*it == ':') {
        this->_appendChar(*it);
        ++it;

        if (it != endIt && *it == '$') {
            this->_appendChar(*it);
            ++it;
        }
    } else if (*it == '$') {
        this->_appendChar(*it);
        ++it;
    } else if (*it == '*') {
        this->_appendChar(*it);
        ++it;

        if (it != endIt && *it == '*') {
            this->_appendChar(*it);
            ++it;
        }
    } else if (*it == '%' && !hasAddSub) {
        this->_appendChar(*it);
        ++it;
    }

    this->_drawNumber(it, endIt);
}

void SearchInputView::_drawCurrentText(const std::string& text)
{
    if (text.empty()) {
        return;
    }

    this->_drawFormatText(text);
}

static int isHexDigit(const int ch)
{
    return std::isxdigit(ch);
}

static int isDecDigit(const int ch)
{
    return std::isdigit(ch);
}

static int isOctDigit(const int ch)
{
    return std::isdigit(ch) && ch != '8' && ch != '9';
}

void SearchInputView::_drawNumber(std::string::const_iterator it,
                                  const std::string::const_iterator endIt)
{
    std::function<int (int)> checkDigitFunc = isDecDigit;

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

} // namespace jacques
