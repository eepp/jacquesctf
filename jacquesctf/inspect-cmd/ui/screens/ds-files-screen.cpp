/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#include "../key.hpp"
#include "cfg.hpp"
#include "../views/ds-file-table-view.hpp"
#include "ds-files-screen.hpp"
#include "../stylist.hpp"

namespace jacques {

DsFilesScreen::DsFilesScreen(const Rect& rect, const InspectCfg& cfg, const Stylist& stylist,
                             InspectCmdState& appState, ScreenHost& host) :
    Screen {rect, cfg, stylist, appState, host},
    _view {std::make_unique<DsFileTableView>(rect, stylist, appState)}
{
    _view->selectFirst();
    this->_addMainView(*_view);
}

void DsFilesScreen::_handleKey(const Key key)
{
    if (this->_handleDsFileKey(key) || this->_handleFmtModeKey(key)) {
        return;
    }

    if (key.special() != SpecialKey::ENTER) {
        return;
    }

    this->_appState().gotoDsFile(_view->selDsFileIndex());

    const auto pktCount = this->_appState().activeDsFileState().dsFile().pktCount();

    if (pktCount == 1) {
        this->_host().switchToInspectScreen();
    } else if (pktCount > 1) {
        this->_host().switchToPktsScreen();
    }
}

} // namespace jacques
