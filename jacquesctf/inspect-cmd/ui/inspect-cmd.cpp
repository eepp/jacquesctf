/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <stdexcept>
#include <optional>
#include <memory>
#include <utility>
#include <vector>
#include <signal.h>
#include <unistd.h>
#include <notcurses/notcurses.h>

#include "inspect-cmd.hpp"
#include "key.hpp"
#include "term.hpp"
#include "cfg.hpp"
#include "../state/inspect-cmd-state.hpp"
#include "stylist.hpp"
#include "overlay.hpp"
#include "screen-host.hpp"
#include "screens/inspect-screen.hpp"
#include "screens/help-screen.hpp"
#include "screens/pkts-screen.hpp"
#include "screens/ds-files-screen.hpp"
#include "screens/dts-screen.hpp"
#include "screens/trace-info-screen.hpp"
#include "views/status-view.hpp"
#include "views/progress-view.hpp"
#include "views/simple-msg-view.hpp"
#include "utils.hpp"
#include "data/pkt-checkpoints-build-listener.hpp"
#include "cmd-error.hpp"
#include "data/data-len.hpp"
#include "data/pkt-region.hpp"
#include "data/padding-pkt-region.hpp"
#include "data/content-pkt-region.hpp"
#include "data/error-pkt-region.hpp"

namespace jacques {
namespace {

void sigHandler(const int signo)
{
    if (signo == SIGINT) {
        /*
         * When the terminal is initialized, notcurses calls this
         * handler after restoring the terminal.
         */
        std::cerr << '\n';
        utils::error() << "Interrupted by user.\n";

        // just exit immediately
        std::exit(0);
    }
}

/*
 * Registers the signal handlers.
 *
 * Call this before term::init() so that notcurses chains its own
 * handlers to them.
 */
void registerSignals()
{
    [[maybe_unused]] const auto ret = signal(SIGINT, sigHandler);

    assert(ret != SIG_ERR);
}

void init()
{
    registerSignals();
    term::init();
}

void buildIndexes(InspectCmdState& appState, const Stylist& stylist)
{
    const auto screenRect = term::size();
    const auto view = std::make_unique<ProgressView>(screenRect, "Building packet indexes…",
                                                     stylist);
    const DsFile *curDsf = nullptr;
    auto func = [&view, &curDsf](const PktIndexEntry& entry) {
        setPktIndexBuildProgress(*view, *curDsf, &entry);
        view->render();
        term::update();
    };

    view->focus();

    for (auto& dsfStateUp : appState.dsFileStates()) {
        auto& dsf = dsfStateUp->dsFile();

        curDsf = &dsf;
        setPktIndexBuildProgress(*view, dsf, nullptr);
        dsf.buildIndex(func, 443);
    }
}

/*
 * Shows the full screen message `msg`.
 *
 * The message remains on the terminal as long as the returned
 * view exists.
 */
std::unique_ptr<SimpleMsgView> showFullScreenMessage(const std::string_view msg,
                                                     const Stylist& stylist)
{
    const auto screenRect = term::size();
    auto view = std::make_unique<SimpleMsgView>(screenRect, stylist);

    view->msg(msg);
    view->focus();
    view->render();
    term::update();
    return view;
}

class PktCheckpointsBuildProgressUpdater final :
    public PktCheckpointsBuildListener
{
public:
    explicit PktCheckpointsBuildProgressUpdater(const Stylist& stylist) :
        _stylist {&stylist}
    {
    }

private:
    /*
     * Only render the progress view, on top of the current planes: the
     * other views are rendered by the event loop which is running.
     */
    void _startBuild(const DsFile&, const PktIndexEntry& pktIndexEntry) override
    {
        if (pktIndexEntry.effectiveTotalLen() < 2_MiB) {
            // too fast anyway
            return;
        }

        const auto rect = Rect {{4, 4}, term::size().w - 8, 13};

        _view = std::make_unique<ProgressView>(rect, "Creating packet checkpoints…", *_stylist);
        _view->focus();
        _pktIndexEntry = &pktIndexEntry;
        setPktCheckpointsBuildProgress(*_view, pktIndexEntry, nullptr);
        this->_render();
        _count = 0;
    }

    void _update(const Er& er) override
    {
        if (!_view) {
            return;
        }

        if (_count++ % 13 != 0) {
            return;
        }

        setPktCheckpointsBuildProgress(*_view, *_pktIndexEntry, &er);
        this->_render();
    }

    void _endBuild() override
    {
        // the next frame rendering reveals what's below
        _view = nullptr;
    }

    void _render()
    {
        _view->render();
        term::update();
    }

private:
    Index _count = 0;
    std::unique_ptr<ProgressView> _view;
    const PktIndexEntry *_pktIndexEntry = nullptr;
    const Stylist * const _stylist;
};

/*
 * Full screen message which asks the user to confirm that they want to
 * quit the program.
 */
class QuitOverlay final :
    public Overlay
{
public:
    /*
     * `quitFunc()` quits the program.
     */
    explicit QuitOverlay(const Stylist& stylist, std::function<void ()> quitFunc) :
        Overlay {true},
        _view {std::make_unique<SimpleMsgView>(term::size(), stylist)},
        _quitFunc {std::move(quitFunc)}
    {
        _view->msg("Press `Y` to quit, or any other key to stay.");
        _view->focus();
    }

private:
    void _render() override
    {
        const auto termRect = term::size();

        // the terminal could have been resized since the last rendering
        if (termRect.w != _view->rect().w || termRect.h != _view->rect().h) {
            _view->moveAndResize(termRect);
        }

        _view->render();
    }

    bool _handleKey(const Key key) override
    {
        if (key.ch() == U'y' || key.ch() == U'Y') {
            _quitFunc();
        }

        // any other key means to stay
        return false;
    }

    void _visibilityChanged() override
    {
        if (!this->isVisible()) {
            _view->detach();
        }
    }

private:
    std::unique_ptr<SimpleMsgView> _view;
    std::function<void ()> _quitFunc;
};

/*
 * Error message which replaces everything while the terminal is too
 * small (see term::sizeOk()).
 *
 * The message is on the standard plane, which is always at the bottom:
 * being opaque, this overlay makes the main loop hide everything else.
 */
class TermTooSmallOverlay final :
    public Overlay
{
public:
    /*
     * `quitFunc()` quits the program.
     */
    explicit TermTooSmallOverlay(const Stylist& stylist, std::function<void ()> quitFunc) :
        Overlay {true},
        _stylist {&stylist},
        _quitFunc {std::move(quitFunc)}
    {
    }

private:
    void _render() override
    {
        auto& stdPlane = term::stdPlane();
        const auto termSize = term::size();

        ncplane_erase(&stdPlane);
        _stylist->apply(stdPlane, Stylist::Role::ERROR);
        ncplane_printf_yx(&stdPlane, 0, 0,
                          "Terminal size must be at least 80×16 (currently %llu×%llu).",
                          static_cast<unsigned long long>(termSize.w),
                          static_cast<unsigned long long>(termSize.h));
    }

    bool _handleKey(const Key key) override
    {
        /*
         * Only accept an immediate quit: the main loop handles the
         * resize, which closes this overlay, and F10.
         */
        if (key.ch() == U'Q') {
            _quitFunc();
        }

        return true;
    }

    void _visibilityChanged() override
    {
        if (!this->isVisible()) {
            ncplane_erase(&term::stdPlane());
        }
    }

private:
    const Stylist *_stylist;
    std::function<void ()> _quitFunc;
};

/*
 * Interactive user interface which owns the status view, the screens,
 * and the overlays which it opens itself, and which runs the main loop.
 *
 * The main loop is the only place which reads the user keys (see
 * term::readKey()).
 */
class InspectUi final :
    public ScreenHost
{
public:
    explicit InspectUi(const Stylist& stylist, InspectCmdState& appState) :
        _appState {&appState},
        _statusView {
            std::make_unique<StatusView>(InspectUi::_statusViewRect(), stylist, appState)
        },
        _screens {
            InspectScreen {InspectUi::_screenRect(), stylist, appState, *this},
            PktsScreen {InspectUi::_screenRect(), stylist, appState, *this},
            DsFilesScreen {InspectUi::_screenRect(), stylist, appState, *this},
            HelpScreen {InspectUi::_screenRect(), stylist, appState, *this},
            DtsScreen {InspectUi::_screenRect(), stylist, appState, *this},
            TraceInfoScreen {InspectUi::_screenRect(), stylist, appState, *this},
        },
        _quitOverlay {stylist, [this] { _done = true; }},
        _termTooSmallOverlay {stylist, [this] { _done = true; }}
    {
    }

    /*
     * Shows the initial screen and runs the main loop until the
     * user quits.
     */
    void run()
    {
        _curScreen = &this->_initScreen();
        this->_updateVisibility();

        while (!_done) {
            this->_renderFrame();
            this->_handleKey(term::readKey());
        }
    }

private:
    void _switchToInspectScreen() override
    {
        this->_switchTo(_screens.inspect);
    }

    void _switchToPktsScreen() override
    {
        this->_switchTo(_screens.pkts);
    }

    /*
     * Makes `screen` the current screen.
     *
     * Does nothing if it's already the current screen.
     */
    void _switchTo(Screen& screen)
    {
        if (&screen == _curScreen) {
            return;
        }

        if (&screen == &_screens.dts) {
            if (_curScreen == &_screens.inspect) {
                _screens.dts.highlightCurDt();
            } else {
                _screens.dts.clearHighlight();
            }
        } else if (&screen == &_screens.help) {
            _prevScreen = _curScreen;
        }

        _curScreen->isVisible(false);
        _curScreen = &screen;
        this->_updateVisibility();
    }

    /*
     * Screen which the character key `ch` selects, if any.
     */
    Screen *_screenFromKey(const char32_t ch) noexcept
    {
        switch (ch) {
        case U'p':
            return &_screens.pkts;

        case U'f':
            return &_screens.dsFiles;

        case U'd':
            return &_screens.dts;

        case U'i':
            return &_screens.traceInfo;

        case U'h':
        case U'H':
        case U'?':
            return &_screens.help;

        default:
            return nullptr;
        }
    }

    void _openOverlay(Overlay& overlay) override
    {
        assert(!this->_overlayIsOpen(overlay));
        _overlays.push_back(&overlay);
        this->_updateVisibility();
    }

    void _renderFrame() override
    {
        // from bottom to top
        _statusView->render();
        _curScreen->render();

        for (const auto overlay : _overlays) {
            overlay->render();
        }

        term::update();
    }

    void _closeOverlay(Overlay& overlay)
    {
        const auto it = std::find(_overlays.begin(), _overlays.end(), &overlay);

        assert(it != _overlays.end());
        _overlays.erase(it);
        overlay.isVisible(false);
        this->_updateVisibility();
        overlay.closed();
    }

    bool _overlayIsOpen(const Overlay& overlay) const noexcept
    {
        return std::find(_overlays.begin(), _overlays.end(), &overlay) != _overlays.end();
    }

    void _updateVisibility()
    {
        /*
         * Level 0 is the status view and the current screen, and level
         * `i + 1` is the overlay `_overlays[i]`.
         *
         * Everything below the level of the topmost opaque overlay
         * is hidden.
         */
        Index bottomLevel = 0;

        for (Index i = 0; i < _overlays.size(); ++i) {
            if (_overlays[i]->isOpaque()) {
                bottomLevel = i + 1;
            }
        }

        _statusView->isVisible(bottomLevel == 0);
        _curScreen->isVisible(bottomLevel == 0);

        for (Index i = 0; i < _overlays.size(); ++i) {
            _overlays[i]->isVisible(i + 1 >= bottomLevel);
        }
    }

    void _handleKey(const Key key)
    {
        // keys which always apply, even with an open overlay
        if (key.special() == SpecialKey::RESIZE) {
            this->_handleResize();
            return;
        } else if (key.special() == SpecialKey::F10) {
            _done = true;
            return;
        } else if (key.ch() == ctrlCh('L')) {
            term::repaint();
            return;
        }

        // the top overlay, if any, takes all the other keys
        if (!_overlays.empty()) {
            auto& overlay = *_overlays.back();

            if (!overlay.handleKey(key)) {
                this->_closeOverlay(overlay);
            }

            return;
        }

        if (key.special() == SpecialKey::ESCAPE) {
            this->_quitCurScreen();
            return;
        }

        if (const auto ch = key.ch()) {
            if (const auto screen = this->_screenFromKey(*ch)) {
                this->_switchTo(*screen);
                return;
            }

            switch (*ch) {
            case U'r':
                term::repaint();
                return;

            case U'q':
                this->_quitCurScreen();
                return;

            case U'Q':
                _done = true;
                return;

            default:
                break;
            }
        }

        _curScreen->handleKey(key);
    }

    void _handleResize()
    {
        term::repaint();

        if (!term::sizeOk()) {
            if (!this->_overlayIsOpen(_termTooSmallOverlay)) {
                this->_openOverlay(_termTooSmallOverlay);
            }

            return;
        }

        {
            const auto screenRect = InspectUi::_screenRect();

            _statusView->moveAndResize(InspectUi::_statusViewRect());

            for (const auto screen : _screens.all()) {
                screen->resize(screenRect.w, screenRect.h);
            }
        }

        if (this->_overlayIsOpen(_termTooSmallOverlay)) {
            this->_closeOverlay(_termTooSmallOverlay);
        }
    }

    void _quitCurScreen()
    {
        if (_curScreen == &_screens.inspect) {
            this->_openOverlay(_quitOverlay);
        } else if (_curScreen == &_screens.help) {
            this->_switchTo(*_prevScreen);
        } else {
            this->_switchTo(_screens.inspect);
        }
    }

    Screen& _initScreen() noexcept
    {
        if (_appState->dsFileStateCount() > 1) {
            return _screens.dsFiles;
        }

        switch (_appState->activeDsFileState().dsFile().pktCount()) {
        case 0:
            return _screens.dsFiles;

        case 1:
            return _screens.inspect;

        default:
            return _screens.pkts;
        }
    }

    /*
     * Rectangle of the screens: the whole terminal except its last
     * line, which is the status view.
     */
    static Rect _screenRect() noexcept
    {
        auto rect = term::size();

        --rect.h;
        return rect;
    }

    static Rect _statusViewRect() noexcept
    {
        const auto termRect = term::size();

        return Rect {{0, termRect.h - 1}, termRect.w, 1};
    }

private:
    const InspectCmdState *_appState;
    std::unique_ptr<StatusView> _statusView;

    struct
    {
        std::array<Screen *, 6> all() noexcept
        {
            return {&inspect, &pkts, &dsFiles, &help, &dts, &traceInfo};
        }

        InspectScreen inspect;
        PktsScreen pkts;
        DsFilesScreen dsFiles;
        HelpScreen help;
        DtsScreen dts;
        TraceInfoScreen traceInfo;
    } _screens;

    Screen *_curScreen = &_screens.inspect;

    // screen to go back to when quitting the help screen
    Screen *_prevScreen = &_screens.inspect;

    // open overlays, from bottom to top
    std::vector<Overlay *> _overlays;

    QuitOverlay _quitOverlay;
    TermTooSmallOverlay _termTooSmallOverlay;
    bool _done = false;
};

void startInteractive(const InspectCfg& cfg)
{
    auto stylist = std::make_unique<const Stylist>();
    auto msgView = showFullScreenMessage("Opening data stream files…", *stylist);
    PktCheckpointsBuildProgressUpdater updater {*stylist};
    auto appState = std::make_unique<InspectCmdState>(cfg.paths(), updater);

    if (appState->dsFileStates().empty()) {
        throw CmdError {"All data stream files to inspect are empty."};
    }

    /*
     * At this point, the state isn't ready: data stream files have no
     * packet indexes and there's no active packet built. This is
     * because we want to provide feedback to the user because it could
     * be a long process.
     *
     * Build indexes first.
     */
    buildIndexes(*appState, *stylist);

    /*
     * Show this message because some views created by the screens
     * can perform some "heavy" caching operations initially.
     */
    msgView = showFullScreenMessage("Building caches…", *stylist);

    InspectUi ui {*stylist, *appState};

    // goto first packet if available: this creates it and shows the progress
    msgView = showFullScreenMessage("Selecting initial packet…", *stylist);

    if (appState->activeDsFileState().dsFile().pktCount() > 0) {
        appState->gotoPkt(0);
    }

    // remove the last message
    msgView.reset();
    ui.run();
}

} // namespace

void inspectCmd(const InspectCfg& cfg)
{
    init();

    // release the terminal whatever the outcome
    try {
        startInteractive(cfg);
    } catch (...) {
        term::fini();
        throw;
    }

    term::fini();
}

} // namespace jacques
