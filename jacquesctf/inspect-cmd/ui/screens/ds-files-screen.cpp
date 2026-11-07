/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#include "ds-files-screen.hpp"

namespace jacques {

DsFilesScreen::DsFilesScreen(const Rect& rect, const Stylist& stylist, InspectCmdState& appState,
                             ScreenHost& host) :
    Screen {rect, stylist, appState, host},
    _view {rect, stylist, appState}
{
    _view.selectFirst();
    this->_addMainView(_view);
}

void DsFilesScreen::_handleKey(const Key key)
{
    if (this->_handleDsFileKey(key) || this->_handleFmtModeKey(key)) {
        return;
    }

    if (key.special() != SpecialKey::ENTER) {
        return;
    }

    this->_appState().gotoDsFile(_view.selDsFileIndex());

    const auto pktCount = this->_appState().activeDsFileState().dsFile().pktCount();

    if (pktCount == 1) {
        this->_host().switchToInspectScreen();
    } else if (pktCount > 1) {
        this->_host().switchToPktsScreen();
    }
}

} // namespace jacques
