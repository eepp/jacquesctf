/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <iostream>
#include <cinttypes>
#include <cstdio>
#include <algorithm>
#include <cstdlib>

#include "../stylist.hpp"
#include "pkt-data-view.hpp"
#include "utils.hpp"
#include "../../state/msg.hpp"
#include "data/content-pkt-region.hpp"
#include "data/padding-pkt-region.hpp"
#include "data/error-pkt-region.hpp"
#include "../screens/inspect-screen.hpp"
#include "utils.hpp"

namespace jacques {

PktDataView::PktDataView(const Rect& rect, const Stylist& stylist, State& state,
                         const InspectScreen::Bookmarks& bookmarks) :
    View {rect, "Packet data", DecorationStyle::BORDERS, stylist},
    _state {&state},
    _stateObserverGuard {state, *this},
    _bookmarks {&bookmarks}
{
    this->_setTitle();
}

void PktDataView::_resized()
{
    this->_setDataXAndRowSize();

    if (_state->hasActivePktState()) {
        this->_setPrevCurNextOffsetInPktBits();
        this->_setBaseAndEndOffsetInPktBitsFromOffset(_curOffsetInPktBits);
    }

    this->_redrawContent();
}

void PktDataView::_stateChanged(const Message msg)
{
    if (msg == Message::ACTIVE_DS_FILE_CHANGED ||
            msg == Message::ACTIVE_PKT_CHANGED) {
        if (_state->hasActivePktState()) {
            this->_setDataXAndRowSize();
            this->_setPrevCurNextOffsetInPktBits();
            this->_setBaseAndEndOffsetInPktBitsFromOffset(_curOffsetInPktBits);
        }

        this->_redrawContent();
    } else if (msg == Message::CUR_OFFSET_IN_PKT_CHANGED) {
        this->_updateSel();
    }
}

void PktDataView::_setBaseAndEndOffsetInPktBitsFromOffset(const Index offsetInPktBits)
{
    assert(_state->hasActivePktState());
    assert(offsetInPktBits < _state->activePktState().pkt().indexEntry().effectiveTotalLen());

    const auto rowOffsetInPktBits = offsetInPktBits - (offsetInPktBits % _rowSize.bits());

    if (rowOffsetInPktBits < this->_halfPageLen()) {
        _baseOffsetInPktBits = 0;
    } else {
        _baseOffsetInPktBits = rowOffsetInPktBits - this->_halfPageLen().bits();
    }

    this->_setEndOffsetInPktBitsFromBaseOffset();
}

void PktDataView::_setPrevCurNextOffsetInPktBits()
{
    if (!_state->hasActivePktState()) {
        return;
    }

    // current
    _curOffsetInPktBits = _state->curOffsetInPktBits();

    const auto curPktRegion = _state->curPktRegion();
    auto& pkt = _state->activePktState().pkt();

    assert(curPktRegion);

    // previous
    const auto prevPktRegion = pkt.previousRegion(*curPktRegion);

    if (prevPktRegion) {
        _prevOffsetInPktBits = prevPktRegion->segment().offsetInPktBits();
    } else {
        _prevOffsetInPktBits = boost::none;
    }

    // next
    if (*curPktRegion->segment().endOffsetInPktBits() < pkt.indexEntry().effectiveTotalLen()) {
        _nextOffsetInPktBits = *curPktRegion->segment().endOffsetInPktBits();
    } else {
        _nextOffsetInPktBits = boost::none;
    }
}

bool PktDataView::_isCharSel(const _Char& ch) const
{
    auto selPktRegionIt = ch.pktRegionIt(_curOffsetInPktBits);

    if (selPktRegionIt != ch.pktRegions.end()) {
        return true;
    }

    if (_prevOffsetInPktBits) {
        selPktRegionIt = ch.pktRegionIt(*_prevOffsetInPktBits);

        if (selPktRegionIt != ch.pktRegions.end()) {
            return true;
        }
    }

    if (_nextOffsetInPktBits) {
        selPktRegionIt = ch.pktRegionIt(*_nextOffsetInPktBits);

        if (selPktRegionIt != ch.pktRegions.end()) {
            return true;
        }
    }

    return false;
}

void PktDataView::_updateSel()
{
    assert(_state->hasActivePktState());

    if (_state->curOffsetInPktBits() < _baseOffsetInPktBits ||
            _state->curOffsetInPktBits() >= _endOffsetInPktBits || _chars.empty()) {
        // outside the current character: reset base offset
        this->_setBaseAndEndOffsetInPktBitsFromOffset(_state->curOffsetInPktBits());
        this->_setPrevCurNextOffsetInPktBits();
        this->_redrawContent();
        return;
    }

    // "erase" currently selected characters
    // TODO: do not search linearly
    for (const auto& ch : _chars) {
        if (this->_isCharSel(ch)) {
            this->_drawUnselChar(ch);
        }
    }

    if (_isAsciiVisible) {
        for (const auto& ch : _asciiChars) {
            if (this->_isCharSel(ch)) {
                this->_drawUnselChar(ch);
            }
        }
    }

    // draw new selected characters
    // TODO: do not search linearly
    this->_setPrevCurNextOffsetInPktBits();

    for (const auto& ch : _chars) {
        if (this->_isCharSel(ch)) {
            this->_drawChar(ch);
        }
    }

    if (_isAsciiVisible) {
        for (const auto& ch : _asciiChars) {
            if (this->_isCharSel(ch)) {
                this->_drawChar(ch);
            }
        }
    }

    // update offset
    this->_drawOffsets();
}

void PktDataView::_setDataXAndRowSize()
{
    if (!_state->hasActivePktState()) {
        return;
    }

    std::array<char, 32> buf;
    const Size div = _isOffsetInBytes ? 8 : 1;
    auto totalLenBits = _state->activePktState().pktIndexEntry().effectiveTotalLen().bits();

    if (!_isOffsetInPkt) {
        totalLenBits += _state->activePktState().pktIndexEntry().offsetInDsFileBits();
    }

    const auto maxOffset = static_cast<unsigned long long>(totalLenBits) / div;

    if (_isOffsetInHex) {
        std::sprintf(buf.data(), "%llx", maxOffset);
    } else {
        std::strcpy(buf.data(), utils::sepNumber(maxOffset).c_str());
    }

    const auto offsetWidth = std::strlen(buf.data());

    Size bytesPerRow = 1;

    while (true) {
        // bytes + spaces between them
        auto dataWidth = bytesPerRow * (this->_charsPerByte() + 1) - 1;

        if (_isAsciiVisible) {
            // ASCII characters + space before them
            dataWidth += bytesPerRow + 1;
        }

        if (dataWidth + offsetWidth + 1 > this->contentRect().w) {
            if (_isRowSizePowerOfTwo) {
                bytesPerRow /= 2;
            } else {
                --bytesPerRow;
            }

            break;
        }

        if (_isRowSizePowerOfTwo) {
            bytesPerRow *= 2;
        } else {
            ++bytesPerRow;
        }
    }

    if (bytesPerRow > _state->activePktState().pktIndexEntry().effectiveTotalLen().bytes()) {
        bytesPerRow = _state->activePktState().pktIndexEntry().effectiveTotalLen().bytes();
    }

    _dataX = offsetWidth + 1;
    _asciiCharsX = _dataX + bytesPerRow * (this->_charsPerByte() + 1);
    _rowSize = DataLen::fromBytes(bytesPerRow);
}

void PktDataView::_drawSeparators() const
{
    if (!_state->hasActivePktState()) {
        return;
    }

    const auto endOffsetInPktBits = _endOffsetInPktBits + _rowSize.bits() - 1;
    const auto yEnd = (endOffsetInPktBits - _baseOffsetInPktBits) / _rowSize.bits();

    this->_stylist().stdDim(*this);

    for (Index y = 0; y < yEnd; ++y) {
        this->_putChar({_dataX - 1, y}, ACS_VLINE);
    }

    if (!_isAsciiVisible) {
        return;
    }

    for (Index y = 0; y < yEnd; ++y) {
        this->_putChar({_asciiCharsX - 1, y}, ACS_VLINE);
    }
}

void PktDataView::_drawOffsets() const
{
    if (!_state->hasActivePktState()) {
        return;
    }

    assert(_dataX > 0 && _rowSize > 0);

    const auto& curRegionSegment = _state->curPktRegion()->segment();
    const auto& pktTotalLen = _state->activePktState().pktIndexEntry().effectiveTotalLen();
    const auto maxOffsetWidth = _dataX - 1;
    auto offsetInPktBits = _baseOffsetInPktBits;
    const Size div = _isOffsetInBytes ? 8 : 1;

    for (Index y = 0; y < this->contentRect().h; ++y) {
        if (offsetInPktBits >= pktTotalLen) {
            // clear
            for (Index x = 0; x < _dataX - 1; ++x) {
                this->_putChar({x, y}, ' ');
            }
        } else {
            std::array<char, 32> buf;
            Index dispOffsetInPktBits = offsetInPktBits;

            if (!_isOffsetInPkt) {
                dispOffsetInPktBits += _state->activePktState().pktIndexEntry().offsetInDsFileBits();
            }

            dispOffsetInPktBits /= div;

            if (_isOffsetInHex) {
                std::sprintf(buf.data(), "%llx", dispOffsetInPktBits);
            } else {
                std::strcpy(buf.data(), utils::sepNumber(dispOffsetInPktBits, ',').c_str());
            }

            const auto offsetWidth = std::strlen(buf.data());

            this->_moveCursor({0, y});

            const auto curRegionBaseOffsetBits = curRegionSegment.offsetInPktBits() -
                                                 curRegionSegment.offsetInPktBits() % _rowSize.bits();

            assert(curRegionSegment.endOffsetInPktBits());

            const auto curRegionBaseEndOffsetBits = (*curRegionSegment.endOffsetInPktBits() - 1) -
                                                    (*curRegionSegment.endOffsetInPktBits() - 1) % _rowSize.bits();

            assert(curRegionBaseEndOffsetBits >= curRegionBaseOffsetBits);

            const auto sel = offsetInPktBits >= curRegionBaseOffsetBits &&
                             offsetInPktBits <= curRegionBaseEndOffsetBits;

            this->_stylist().pktDataViewOffset(*this, sel);

            for (Index x = 0; x < maxOffsetWidth - offsetWidth; ++x) {
                this->_appendChar(' ');
            }

            this->_print("%s", buf.data());
            offsetInPktBits += _rowSize.bits();
        }
    }
}

void PktDataView::_setTitle()
{
    std::string title = "Packet data (offsets: ";

    if (_isOffsetInHex) {
        title += "hex";
    } else {
        title += "dec";
    }

    title += ", ";

    if (_isOffsetInBytes) {
        title += "bytes";
    } else {
        title += "bits";
    }

    title += ", in ";

    if (_isOffsetInPkt) {
        title += "packet";
    } else {
        title += "DSF";
    }

    title += ")";
    this->_title(title);
    this->_decorate();
}

void PktDataView::_setCustomStyle(const _Char& ch) const
{
    if (ch.pktRegions.size() != 1) {
        if (ch.isPrintable) {
            this->_stylist().std(*this);
        } else {
            this->_stylist().stdDim(*this);
        }

        return;
    }

    const auto it = _bookmarks->find(_state->activeDsFileStateIndex());
    const InspectScreen::PktBookmarks *bookmarks = nullptr;

    if (it != _bookmarks->end()) {
        const auto& dsFileBookmarks = it->second;
        const auto pIt = dsFileBookmarks.find(_state->activeDsFileState().activePktStateIndex());

        if (pIt != dsFileBookmarks.end()) {
            bookmarks = &pIt->second;
        }
    }

    auto bookmarkExists = false;
    const auto& singlePktRegion = *ch.pktRegions.front();

    if (bookmarks) {
        for (Index id = 0; id < bookmarks->size(); ++id) {
            if (bookmarks->at(id) &&
                    *(bookmarks->at(id)) == singlePktRegion.segment().offsetInPktBits()) {
                this->_stylist().pktDataViewBookmark(*this, id);
                bookmarkExists = true;
                break;
            }
        }
    }

    if (!bookmarkExists) {
        if (ch.isErFirst && _isErFirstPktRegionEmphasized) {
            this->_stylist().pktDataViewErFirstPktRegion(*this);
        } else if (const auto region = dynamic_cast<const ContentPktRegion *>(&singlePktRegion)) {
            if (ch.isPrintable) {
                this->_stylist().std(*this);
            } else {
                this->_stylist().stdDim(*this);
            }
        } else if (const auto region = dynamic_cast<const PaddingPktRegion *>(&singlePktRegion)) {
            this->_stylist().pktDataViewPadding(*this);
        } else if (const auto region = dynamic_cast<const ErrorPktRegion *>(&singlePktRegion)) {
            this->_stylist().error(*this);
        } else {
            std::abort();
        }
    }
}

void PktDataView::_drawUnselChar(const _Char& ch) const
{
    this->_setCustomStyle(ch);
    this->_putChar(ch.pt, ch.val);
}

void PktDataView::_drawChar(const _Char& ch) const
{
    const auto selPktRegionIt = ch.pktRegionIt(_curOffsetInPktBits);

    if (selPktRegionIt != ch.pktRegions.end()) {
        if (ch.pktRegions.size() == 1) {
            this->_stylist().pktDataViewSel(*this, Stylist::PktDataViewSelType::CUR);
        } else {
            this->_stylist().pktDataViewAuxSel(*this, Stylist::PktDataViewSelType::CUR);
        }

        this->_putChar(ch.pt, ch.val);
        return;
    }

    if (ch.pktRegions.size() != 1) {
        this->_drawUnselChar(ch);
        return;
    }

    const auto& singlePktRegion = *ch.pktRegions.front();

    if (_isPrevNextVisible) {
        if (_prevOffsetInPktBits &&
                *_prevOffsetInPktBits == singlePktRegion.segment().offsetInPktBits()) {
            this->_stylist().pktDataViewSel(*this,
                                                     Stylist::PktDataViewSelType::PREV);
            this->_putChar(ch.pt, ch.val);
            return;
        }

        if (_nextOffsetInPktBits &&
                *_nextOffsetInPktBits == singlePktRegion.segment().offsetInPktBits()) {
            this->_stylist().pktDataViewSel(*this,
                                                     Stylist::PktDataViewSelType::NEXT);
            this->_putChar(ch.pt, ch.val);
            return;
        }
    }

    this->_drawUnselChar(ch);
}

void PktDataView::_drawAllNumericChars() const
{
    for (const auto& ch : _chars) {
        this->_drawChar(ch);
    }
}

void PktDataView::_drawAllAsciiChars() const
{
    if (!_isAsciiVisible) {
        return;
    }

    for (const auto& ch : _asciiChars) {
        this->_drawChar(ch);
    }
}

void PktDataView::_redrawContent()
{
    this->_clearContent();

    if (!_state->hasActivePktState()) {
        return;
    }

    this->_drawOffsets();
    this->_drawSeparators();
    this->_setNumericCharsAndAsciiChars();
    this->_drawAllNumericChars();
    this->_drawAllAsciiChars();
    this->_hasMoreTop(_baseOffsetInPktBits != 0);
    this->_hasMoreBottom(_endOffsetInPktBits <
                         _state->activePktState().pkt().indexEntry().effectiveTotalLen());
}

static inline chtype charFromNibble(const std::uint8_t nibble)
{
    if (nibble < 10) {
        return '0' + nibble;
    } else {
        return 'a' + nibble - 10;
    }
}

void PktDataView::_setHexChars()
{
    /*
     * The strategy here is to create all the nibble characters first,
     * and then iterate the packet regions and find the already-created
     * character to append a packet region (if not already appended).
     */
    const auto data = _state->activePktState().pkt().data(_baseOffsetInPktBits / 8);

    for (auto offsetInPktBits = _baseOffsetInPktBits; offsetInPktBits < _endOffsetInPktBits;
            offsetInPktBits += 8) {
        const auto byte = data[(offsetInPktBits - _baseOffsetInPktBits) / 8];
        const auto y = (offsetInPktBits - _baseOffsetInPktBits) / _rowSize.bits();
        const auto byteIndexInRow = (offsetInPktBits % _rowSize.bits()) / 8;

        // high nibble
        {
            _Char ch;

            ch.pt.y = y;
            ch.pt.x = _dataX + byteIndexInRow * 3;
            ch.val = charFromNibble((byte >> 4) & 0xf);
            _chars.push_back(std::move(ch));
        }

        // low nibble
        {
            _Char ch;

            ch.pt.y = y;
            ch.pt.x = _dataX + byteIndexInRow * 3 + 1;
            ch.val = charFromNibble(byte & 0xf);
            _chars.push_back(std::move(ch));
        }
    }

    const Er *curEr = nullptr;

    for (auto& pktRegion : _pktRegions) {
        const auto bitArray = _state->activePktState().pkt().bitArray(*pktRegion);
        const auto firstBitOffsetInPkt = pktRegion->segment().offsetInPktBits();

        auto isErFirst = false;

        if (pktRegion->scope() && pktRegion->scope()->er() && pktRegion->scope()->er() != curEr) {
            curEr = pktRegion->scope()->er();
            isErFirst = true;
        }

        const auto startOffsetInPktBits = std::max(firstBitOffsetInPkt, _baseOffsetInPktBits);
        const auto endOffsetInPktBits = std::min(firstBitOffsetInPkt +
                                                 pktRegion->segment().len()->bits(),
                                                 _endOffsetInPktBits);

        for (Index bitOffsetInPkt = startOffsetInPktBits;
                bitOffsetInPkt < endOffsetInPktBits; ++bitOffsetInPkt) {
            const auto indexInBitArray = bitOffsetInPkt -
                                         firstBitOffsetInPkt;

            // times two because `_chars` contains nibbles, not bytes
            auto charIndex = ((bitOffsetInPkt / 8) - (_baseOffsetInPktBits / 8)) * 2;
            const auto bitLoc = bitArray.bitLoc(indexInBitArray);

            if (bitLoc.bitIndexInByte() < 4) {
                // low nibble
                ++charIndex;
            }

            assert(charIndex < _chars.size());

            // get existing character (created above)
            auto& ch = _chars[charIndex];

            ch.isErFirst = isErFirst;

            if (ch.pktRegions.empty() || ch.pktRegions.back() != pktRegion.get()) {
                // associate character to packet region
                ch.pktRegions.push_back(pktRegion.get());
            }
        }
    }
}

void PktDataView::_setAsciiChars()
{
    /*
     * Similar strategy to what we're doing in _setHexChars(), only here
     * the characters represent whole bytes, not nibbles.
     */
    assert((_baseOffsetInPktBits & 7) == 0);

    const auto& pkt = _state->activePktState().pkt();

    for (auto offsetInPktBits = _baseOffsetInPktBits; offsetInPktBits < _endOffsetInPktBits;
            offsetInPktBits += 8) {
        _Char ch;

        ch.pt.y = (offsetInPktBits - _baseOffsetInPktBits) / _rowSize.bits();
        ch.pt.x = _asciiCharsX + (offsetInPktBits % _rowSize.bits()) / 8;

        const auto val = static_cast<chtype>(*pkt.data(offsetInPktBits / 8));

        if (!std::isprint(val)) {
            ch.isPrintable = false;
            ch.val = ACS_BULLET;
        } else {
            ch.val = val;
        }

        _asciiChars.push_back(std::move(ch));
    }

    const Er *curEr = nullptr;

    for (const auto& pktRegion : _pktRegions) {
        const auto firstBitOffsetInPkt = pktRegion->segment().offsetInPktBits();

        bool isErFirst = false;

        if (pktRegion->scope() && pktRegion->scope()->er() && pktRegion->scope()->er() != curEr) {
            curEr = pktRegion->scope()->er();
            isErFirst = true;
        }

        const auto startOffsetInPktBits = std::max(firstBitOffsetInPkt, _baseOffsetInPktBits);
        const auto endOffsetInPktBits = std::min(firstBitOffsetInPkt +
                                                 pktRegion->segment().len()->bits(),
                                                 _endOffsetInPktBits);

        for (Index bitOffsetInPkt = startOffsetInPktBits; bitOffsetInPkt < endOffsetInPktBits;
                ++bitOffsetInPkt) {
            const auto charIndex = ((bitOffsetInPkt / 8) - (_baseOffsetInPktBits / 8));

            assert(charIndex < _asciiChars.size());

            // get existing character (created above)
            auto& ch = _asciiChars[charIndex];

            ch.isErFirst = isErFirst;

            if (ch.pktRegions.empty() || ch.pktRegions.back() != pktRegion.get()) {
                // associate character to packet region
                ch.pktRegions.push_back(pktRegion.get());
            }
        }
    }
}

void PktDataView::_setBinaryChars()
{
    const Er *curEr = nullptr;

    for (const auto& pktRegion : _pktRegions) {
        const auto bitArray = _state->activePktState().pkt().bitArray(*pktRegion);
        const auto firstBitOffsetInPkt = pktRegion->segment().offsetInPktBits();
        bool isErFirst = false;

        if (pktRegion->scope() && pktRegion->scope()->er() && pktRegion->scope()->er() != curEr) {
            curEr = pktRegion->scope()->er();
            isErFirst = true;
        }

        const auto startOffsetInPktBits = std::max(firstBitOffsetInPkt, _baseOffsetInPktBits);
        const auto endOffsetInPktBits = std::min(firstBitOffsetInPkt +
                                                 pktRegion->segment().len()->bits(),
                                                 _endOffsetInPktBits);

        for (Index bitOffsetInPkt = startOffsetInPktBits; bitOffsetInPkt < endOffsetInPktBits;
                ++bitOffsetInPkt) {
            const auto indexInBitArray = bitOffsetInPkt - firstBitOffsetInPkt;
            const auto bitLoc = bitArray.bitLoc(indexInBitArray);
            _Char ch;

            ch.isErFirst = isErFirst;
            ch.val = '0' + bitArray[bitLoc];
            ch.pt.y = (bitOffsetInPkt - _baseOffsetInPktBits) / _rowSize.bits();

            const auto byteIndex = (bitOffsetInPkt % _rowSize.bits()) / 8;

            /*
             * 28000 00101101 11010010 11101010 00010010
             *       ^ _dataX [6]      ^ + byteIndex * 9 [+ 2 * 9]
             *                              ^ + 7 - bitLoc.bitIndexInByte() [+ 7 - 2]
             */
            ch.pt.x = _dataX + byteIndex * 9 + 7 - bitLoc.bitIndexInByte();
            ch.pktRegions.push_back(pktRegion.get());
            _chars.push_back(std::move(ch));
        }
    }
}

void PktDataView::_setNumericCharsAndAsciiChars()
{
    if (_baseOffsetInPktBits == _endOffsetInPktBits) {
        return;
    }

    assert(_baseOffsetInPktBits < _endOffsetInPktBits);
    assert(_state->hasActivePktState());

    // set numeric characters
    std::vector<PktRegion::SPC> pktRegions;
    auto& pkt = _state->activePktState().pkt();

    /*
     * We'll go one packet region before to detect an initial event
     * record change.
     */
    const auto& basePktRegion = pkt.regionAtOffsetInPktBits(_baseOffsetInPktBits);
    auto startingPktRegion = pkt.previousRegion(basePktRegion);

    if (!startingPktRegion) {
        startingPktRegion = &basePktRegion;
    }

    _pktRegions.clear();
    pkt.appendRegions(_pktRegions, startingPktRegion->segment().offsetInPktBits(),
                      _endOffsetInPktBits);
    assert(!_pktRegions.empty());
    _chars.clear();

    if (_isDataInHex) {
        this->_setHexChars();
    } else {
        this->_setBinaryChars();
    }

    // set ASCII chars
    _asciiChars.clear();
    this->_setAsciiChars();
}

void PktDataView::pageDown()
{
    if (!_state->hasActivePktState()) {
        return;
    }

    const auto effectiveTotalLenBits = _state->activePktState().pkt().indexEntry().effectiveTotalLen().bits();

    _baseOffsetInPktBits += this->_halfPageLen().bits();

    if (_baseOffsetInPktBits >= effectiveTotalLenBits) {
        const auto endRowOffsetInPktBits = effectiveTotalLenBits -
                                           (effectiveTotalLenBits % _rowSize.bits());

        if (endRowOffsetInPktBits == 0) {
            _baseOffsetInPktBits = 0;
        } else {
            _baseOffsetInPktBits = endRowOffsetInPktBits - _rowSize.bits();
        }
    }

    this->_setEndOffsetInPktBitsFromBaseOffset();
    this->_redrawContent();
}

void PktDataView::pageUp()
{
    if (!_state->hasActivePktState()) {
        return;
    }

    const auto halfPageLen = this->_halfPageLen();

    if (_baseOffsetInPktBits < halfPageLen.bits()) {
        _baseOffsetInPktBits = 0;
    } else {
        _baseOffsetInPktBits -= halfPageLen.bits();
    }

    this->_setEndOffsetInPktBitsFromBaseOffset();
    this->_redrawContent();
}

void PktDataView::isAsciiVisible(const bool isVisible)
{
    _isAsciiVisible = isVisible;
    this->_setDataXAndRowSize();

    if (_state->hasActivePktState()) {
        this->_setBaseAndEndOffsetInPktBitsFromOffset(_curOffsetInPktBits);
    }

    this->_redrawContent();
}

void PktDataView::isPrevNextVisible(const bool isVisible)
{
    _isPrevNextVisible = isVisible;
    this->_redrawContent();
}

void PktDataView::isErFirstPktRegionEmphasized(const bool isEmphasized)
{
    _isErFirstPktRegionEmphasized = isEmphasized;
    this->_redrawContent();
}

void PktDataView::isDataInHex(const bool isDataInHex)
{
    _isDataInHex = isDataInHex;
    this->_setDataXAndRowSize();

    if (_state->hasActivePktState()) {
        this->_setBaseAndEndOffsetInPktBitsFromOffset(_curOffsetInPktBits);
    }

    this->_redrawContent();
}

void PktDataView::centerSel()
{
    if (!_state->hasActivePktState()) {
        return;
    }

    this->_setBaseAndEndOffsetInPktBitsFromOffset(_curOffsetInPktBits);
    this->_redrawContent();
}

void PktDataView::isOffsetInHex(const bool isOffsetInHex)
{
    _isOffsetInHex = isOffsetInHex;
    this->_setDataXAndRowSize();

    if (_state->hasActivePktState()) {
        this->_setBaseAndEndOffsetInPktBitsFromOffset(_curOffsetInPktBits);
    }

    this->_setTitle();
    this->_redrawContent();
}

void PktDataView::isOffsetInBytes(const bool isOffsetInBytes)
{
    _isOffsetInBytes = isOffsetInBytes;
    this->_setDataXAndRowSize();

    if (_state->hasActivePktState()) {
        this->_setBaseAndEndOffsetInPktBitsFromOffset(_curOffsetInPktBits);
    }

    this->_setTitle();
    this->_redrawContent();
}

void PktDataView::isRowSizePowerOfTwo(const bool isPowerOfTwo)
{
    _isRowSizePowerOfTwo = isPowerOfTwo;
    this->_setDataXAndRowSize();

    if (_state->hasActivePktState()) {
        this->_setBaseAndEndOffsetInPktBitsFromOffset(_curOffsetInPktBits);
    }

    this->_redrawContent();
}

void PktDataView::isOffsetInPkt(const bool isOffsetInPkt)
{
    _isOffsetInPkt = isOffsetInPkt;
    this->_setDataXAndRowSize();

    if (_state->hasActivePktState()) {
        this->_setBaseAndEndOffsetInPktBitsFromOffset(_curOffsetInPktBits);
    }

    this->_setTitle();
    this->_redrawContent();
}

} // namespace jacques
