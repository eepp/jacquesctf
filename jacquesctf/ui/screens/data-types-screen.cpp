/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include "data-types-screen.hpp"
#include "search-parser.hpp"
#include "content-packet-region.hpp"

namespace jacques {

DataTypesScreen::DataTypesScreen(const Rectangle& rect,
                                 const InspectConfig& cfg,
                                 const Stylist& stylist, State& state) :
    Screen {rect, cfg, stylist, state},
    _searchController {*this, stylist}
{
    const auto viewRects = this->_viewRects();
    _dstTableView = std::make_unique<DataStreamTypeTableView>(std::get<0>(viewRects),
                                                              stylist, state);
    _ertTableView = std::make_unique<EventRecordTypeTableView>(std::get<1>(viewRects),
                                                               stylist, state);
    _dtExplorerView = std::make_unique<DataTypeExplorerView>(std::get<2>(viewRects),
                                                             stylist);
    _focusedView = _dstTableView.get();
    this->_updateViews();
    _focusedView->focus();
}

void DataTypesScreen::_redraw()
{
    _ertTableView->redraw();
    _dstTableView->redraw();
    _dtExplorerView->redraw();
}

void DataTypesScreen::_visibilityChanged()
{
    _dstTableView->isVisible(this->isVisible() && _tablesVisible);
    _ertTableView->isVisible(this->isVisible() && _tablesVisible);
    _dtExplorerView->isVisible(this->isVisible());

    if (this->isVisible()) {
        this->_updateViews();
        _ertTableView->redraw();
        _dstTableView->redraw();
        _dtExplorerView->redraw();
    }
}

std::tuple<Rectangle, Rectangle, Rectangle> DataTypesScreen::_viewRects() const
{
    const auto& rect = this->rect();
    const auto detailsWidth = _tablesVisible ? rect.w - (rect.w / 2) : rect.w;
    const auto detailsX = _tablesVisible ? rect.w / 2 : 0;
    const auto dstTableViewRect = Rectangle {rect.pos, rect.w / 2, 6};
    const auto ertTableViewRect = Rectangle {{rect.pos.x, rect.pos.y + dstTableViewRect.h},
                                             rect.w / 2, rect.h - dstTableViewRect.h};
    const auto dtExplorerViewRect = Rectangle {{detailsX, rect.pos.y},
                                               detailsWidth, rect.h};

    return std::make_tuple(dstTableViewRect, ertTableViewRect,
                           dtExplorerViewRect);
}

void DataTypesScreen::_updateViews()
{
    _dstTableView->traceType(*this->_state().metadata().traceType());
    _ertTableView->dataStreamType(*_dstTableView->dataStreamType());

    if (_focusedView == _dstTableView.get()) {
        _dtExplorerView->dataStreamType(*_dstTableView->dataStreamType());
    } else if (_focusedView == _ertTableView.get()) {
        _dtExplorerView->eventRecordType(*_ertTableView->eventRecordType());
    }
}

void DataTypesScreen::_resized()
{
    const auto viewRects = this->_viewRects();

    _dstTableView->moveAndResize(std::get<0>(viewRects));
    _ertTableView->moveAndResize(std::get<1>(viewRects));
    _dtExplorerView->moveAndResize(std::get<2>(viewRects));
    _searchController.parentScreenResized(*this);
}

void DataTypesScreen::highlightCurrentDataType()
{
    if (!this->_state().hasActivePacketState()) {
        _dtExplorerView->clearHighlight();
    }

    auto& activePacketState = this->_state().activePacketState();

    const auto curDst = activePacketState.packetIndexEntry().dataStreamType();
    const auto curEventRecord = this->_state().currentEventRecord();

    if (curEventRecord && curEventRecord->type()) {
        assert(curDst);

        _dstTableView->selectDataStreamType(curDst->id());
        _ertTableView->dataStreamType(*curDst);
        _ertTableView->selectEventRecordType(curEventRecord->type()->id());
        _dstTableView->centerSelectedRow(true);
        _ertTableView->centerSelectedRow(true);
        _dtExplorerView->eventRecordType(*curEventRecord->type());
        _focusedView->blur();
        _focusedView = _ertTableView.get();
        _focusedView->focus();
    } else if (curDst) {
        _dstTableView->selectDataStreamType(curDst->id());
        _ertTableView->dataStreamType(*curDst);
        _ertTableView->selectFirst();
        _dstTableView->centerSelectedRow(true);
        _ertTableView->centerSelectedRow(true);
        _dtExplorerView->dataStreamType(*curDst);
        _focusedView->blur();
        _focusedView = _dstTableView.get();
        _focusedView->focus();
    }

    _dtExplorerView->clearHighlight();

    const auto curPacketRegion = this->_state().currentPacketRegion();

    if (curPacketRegion) {
        const auto cPacketRegion = dynamic_cast<const ContentPacketRegion *>(curPacketRegion);

        if (cPacketRegion) {
            _dtExplorerView->highlightDataType(cPacketRegion->dataType());
            _focusedView->blur();
            _focusedView = _dtExplorerView.get();
            _focusedView->focus();
        }
    }
}

void DataTypesScreen::clearHighlight()
{
    _dtExplorerView->clearHighlight();
}

KeyHandlingReaction DataTypesScreen::_handleKey(const int key)
{
    switch (key) {
    case KEY_UP:
        if (_focusedView == _ertTableView.get()) {
            _ertTableView->prev();
        } else if (_focusedView == _dstTableView.get()) {
            _dstTableView->prev();
        } else if (_focusedView == _dtExplorerView.get()) {
            _dtExplorerView->prev();
        }

        this->_updateViews();
        break;

    case KEY_DOWN:
        if (_focusedView == _ertTableView.get()) {
            _ertTableView->next();
        } else if (_focusedView == _dstTableView.get()) {
            _dstTableView->next();
        } else if (_focusedView == _dtExplorerView.get()) {
            _dtExplorerView->next();
        }

        this->_updateViews();
        break;

    case KEY_PPAGE:
        if (_focusedView == _ertTableView.get()) {
            _ertTableView->pageUp();
        } else if (_focusedView == _dstTableView.get()) {
            _dstTableView->pageUp();
        } else if (_focusedView == _dtExplorerView.get()) {
            _dtExplorerView->pageUp();
        }

        this->_updateViews();
        break;

    case KEY_NPAGE:
        if (_focusedView == _ertTableView.get()) {
            _ertTableView->pageDown();
        } else if (_focusedView == _dstTableView.get()) {
            _dstTableView->pageDown();
        } else if (_focusedView == _dtExplorerView.get()) {
            _dtExplorerView->pageDown();
        }

        this->_updateViews();
        break;

    case KEY_END:
        if (_focusedView == _ertTableView.get()) {
            _ertTableView->selectLast();
        } else if (_focusedView == _dstTableView.get()) {
            _dstTableView->selectLast();
        }

        this->_updateViews();
        break;

    case KEY_HOME:
        if (_focusedView == _ertTableView.get()) {
            _ertTableView->selectFirst();
        } else if (_focusedView == _dstTableView.get()) {
            _dstTableView->selectFirst();
        }

        this->_updateViews();
        break;

    case 'c':
        if (_focusedView == _ertTableView.get()) {
            _ertTableView->centerSelectedRow();
        } else if (_focusedView == _dstTableView.get()) {
            _dstTableView->centerSelectedRow();
        }

        break;

    case '\n':
    case '\r':
    case '\t':
    {
        if (!_tablesVisible) {
            break;
        }

        // change focus
        auto oldFocusedView = _focusedView;

        if (_focusedView == _ertTableView.get()) {
            _focusedView = _dtExplorerView.get();
        } else if (_focusedView == _dtExplorerView.get()) {
            _focusedView = _dstTableView.get();
        } else if (_focusedView == _dstTableView.get()) {
            if (_ertTableView->isEmpty()) {
                _focusedView = _dtExplorerView.get();
            } else {
                _focusedView = _ertTableView.get();
            }
        }

        this->_updateViews();
        oldFocusedView->blur();
        _focusedView->focus();
        break;
    }

    case KEY_LEFT:
        if (!_tablesVisible) {
            break;
        }

        _focusedView->blur();

        if (_ertTableView->isEmpty()) {
            _focusedView = _dstTableView.get();
        } else {
            _focusedView = _ertTableView.get();
        }

        _focusedView->focus();
        this->_updateViews();
        break;

    case KEY_RIGHT:
        if (!_tablesVisible || _focusedView == _dtExplorerView.get()) {
            break;
        }

        _focusedView->blur();
        _focusedView = _dtExplorerView.get();
        _focusedView->focus();
        this->_updateViews();
        break;

    case '+':
    {
        _tablesVisible = !_tablesVisible;

        const auto viewRects = this->_viewRects();

        _dtExplorerView->moveAndResize(std::get<2>(viewRects));
        _dstTableView->isVisible(this->isVisible() && _tablesVisible);
        _ertTableView->isVisible(this->isVisible() && _tablesVisible);

        if (!_tablesVisible) {
            _focusedView->blur();
            _focusedView = _dtExplorerView.get();
            _focusedView->focus();
        }

        _dstTableView->redraw();
        _ertTableView->redraw();
        _dtExplorerView->redraw();
        break;
    }

    case '/':
    case 'g':
    {
        if (!_tablesVisible || _focusedView != _ertTableView.get()) {
            break;
        }

        auto query = _searchController.start();

        if (!query) {
            // canceled or invalid
            _dstTableView->redraw();
            _ertTableView->redraw();
            _dtExplorerView->redraw();
            break;
        }

        _lastQuery = nullptr;

        if (const auto sQuery = dynamic_cast<const EventRecordTypeNameSearchQuery *>(query.get())) {
            _ertTableView->selectEventRecordType(sQuery->pattern());
            _lastQuery.reset(sQuery);
            query.release();
        } else if (const auto sQuery = dynamic_cast<const EventRecordTypeIdSearchQuery *>(query.get())) {
            _ertTableView->selectEventRecordType(static_cast<yactfr::TypeId>(sQuery->value()));
        }

        this->_updateViews();
        _dstTableView->redraw();
        _ertTableView->centerSelectedRow();
        _ertTableView->redraw();
        _dtExplorerView->redraw();
        break;
    }

    case 'n':
        if (!_tablesVisible || _focusedView != _ertTableView.get() || !_lastQuery) {
            break;
        }

        _ertTableView->selectEventRecordType(_lastQuery->pattern(), true);
        this->_updateViews();
        _ertTableView->centerSelectedRow();
        break;

    default:
        break;
    }

    _ertTableView->refresh();
    _dstTableView->refresh();
    _dtExplorerView->refresh();
    return KeyHandlingReaction::CONTINUE;
}

} // namespace jacques
