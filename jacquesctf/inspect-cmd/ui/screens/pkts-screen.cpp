/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#include <iostream>
#include <functional>
#include <utility>
#include <signal.h>
#include <unistd.h>

#include "../key.hpp"
#include "../term.hpp"
#include "cfg.hpp"
#include "../views/pkt-table-view.hpp"
#include "../views/search-input-view.hpp"
#include "pkts-screen.hpp"
#include "../stylist.hpp"
#include "data/pkt-checkpoints-build-listener.hpp"
#include "../views/pkt-checkpoints-build-progress-view.hpp"
#include "utils.hpp"

namespace jacques {

PktsScreen::PktsScreen(const Rect& rect, const InspectCfg& cfg, const Stylist& stylist,
                       InspectCmdState& appState, SearchCtrl::RenderFrameFunc renderFrameFunc) :
    Screen {rect, cfg, stylist, appState},
    _ptView {std::make_unique<PktTableView>(rect, stylist, appState)},
    _searchCtrl {*this, stylist, std::move(renderFrameFunc)},
    _tsFmtModeWheel {
        TsFmtMode::LONG,
        TsFmtMode::NS_FROM_ORIGIN,
        TsFmtMode::CYCLES,
    },
    _dataLenFmtModeWheel {
        utils::LenFmtMode::FULL_FLOOR_WITH_EXTRA_BITS,
        utils::LenFmtMode::BYTES_FLOOR_WITH_EXTRA_BITS,
        utils::LenFmtMode::BITS,
    }
{
    this->_addView(*_ptView);
    this->_addView(_searchCtrl.view());
    _ptView->focus();
}

void PktsScreen::_resized()
{
    _ptView->moveAndResize(this->rect());
    _searchCtrl.parentScreenResized(*this);
}

namespace {

class AnalyzeAllPktsProgressUpdater final :
    public PktCheckpointsBuildListener
{
public:
    explicit AnalyzeAllPktsProgressUpdater(const Stylist& stylist) :
        _view {
            std::make_unique<PktCheckpointsBuildProgressView>(
                Rect {{4, 4}, term::size().w - 8, 12},
                stylist
            )
        }
    {
        _view->focus();
    }

private:
    void _startBuild(const DsFile&, const PktIndexEntry& pktIndexEntry) override
    {
        ++_pktCount;

        if (pktIndexEntry.effectiveTotalLen() >= 2_MiB || _pktCount % 49 == 1) {
            _canUpdate = true;
        } else {
            _canUpdate = false;
            return;
        }

        _view->pktIndexEntry(pktIndexEntry);
        this->_render();
        _count = 0;
    }

    void _update(const Er& er) override
    {
        if (!_canUpdate) {
            return;
        }

        if (_count++ % 41 != 0) {
            return;
        }

        _view->er(er);
        this->_render();
    }

    // renders the progress view on top of the others
    void _render()
    {
        _view->render();
        term::update();
    }

private:
    bool _canUpdate = false;
    Index _pktCount = 0;
    Index _count = 0;
    std::unique_ptr<PktCheckpointsBuildProgressView> _view;
};

} // namespace

KeyHandlingReaction PktsScreen::_handleKey(const Key key)
{
    if (const auto specialKey = key.special()) {
        switch (*specialKey) {
        case SpecialKey::UP:
            _ptView->prev();
            break;

        case SpecialKey::DOWN:
            _ptView->next();
            break;

        case SpecialKey::PAGE_UP:
            _ptView->pageUp();
            break;

        case SpecialKey::PAGE_DOWN:
            _ptView->pageDown();
            break;

        case SpecialKey::END:
            _ptView->selectLast();
            break;

        case SpecialKey::HOME:
            _ptView->selectFirst();
            break;

        case SpecialKey::ENTER:
            if (this->_appState().activeDsFileState().dsFile().pktCount() == 0) {
                break;
            }

            this->_appState().gotoPkt(_ptView->selPktIndex());
            return KeyHandlingReaction::RETURN_TO_INSPECT;

        case SpecialKey::F3:
            this->_appState().gotoPrevDsFile();
            break;

        case SpecialKey::F4:
            this->_appState().gotoNextDsFile();
            break;

        case SpecialKey::F5:
            this->_appState().gotoPrevPkt();
            break;

        case SpecialKey::F6:
            this->_appState().gotoNextPkt();
            break;

        default:
            break;
        }
    } else {
        const auto ch = *key.ch();

        switch (ch) {
        case 'c':
            _ptView->centerSelRow();
            break;

        case 't':
            _tsFmtModeWheel.next();
            _ptView->tsFmtMode(_tsFmtModeWheel.curVal());
            break;

        case 's':
            _dataLenFmtModeWheel.next();
            _ptView->dataLenFmtMode(_dataLenFmtModeWheel.curVal());
            break;

        case '/':
        case 'g':
        case 'N':
        case 'k':
        case ':':
        case '$':
        case '*':
        case 'P':
        {
            auto query = _searchCtrl.start(std::invoke([ch]() -> std::string {
                switch (ch) {
                case 'N':
                case '*':
                    return "*";
                    break;

                case 'k':
                    return "**";
                    break;

                case ':':
                    return ":";
                    break;

                case 'P':
                    return "#";
                    break;

                case '$':
                    return "$";
                    break;

                default:
                    return {};
                }
            }));

            if (!query) {
                // canceled or invalid
                break;
            }

            this->_appState().search(*query);
            _lastQuery = std::move(query);
            break;
        }

        case 'n':
            if (!_lastQuery) {
                break;
            }

            this->_appState().search(*_lastQuery);
            break;

        case 'a':
        {
            AnalyzeAllPktsProgressUpdater updater {this->_stylist()};

            this->_appState().activeDsFileState().analyzeAllPkts(&updater);

            // the packet index entries now have more information
            _ptView->pktIndexEntriesChanged();
            break;
        }

        default:
            break;
        }
    }

    return KeyHandlingReaction::CONTINUE;
}

} // namespace jacques
