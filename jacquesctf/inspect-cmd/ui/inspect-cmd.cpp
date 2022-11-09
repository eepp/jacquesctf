/*
 * Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
 *
 * Unauthorized copying of this file, via any medium, is strictly
 * prohibited. Proprietary and confidential.
 */

#include <iostream>
#include <stdexcept>
#include <curses.h>
#include <signal.h>
#include <unistd.h>
#include <boost/optional.hpp>

#include "inspect-cmd.hpp"
#include "cfg.hpp"
#include "../state/inspect-cmd-state.hpp"
#include "stylist.hpp"
#include "screens/inspect-screen.hpp"
#include "screens/help-screen.hpp"
#include "screens/pkts-screen.hpp"
#include "screens/ds-files-screen.hpp"
#include "screens/dts-screen.hpp"
#include "screens/trace-info-screen.hpp"
#include "views/status-view.hpp"
#include "views/pkt-index-build-progress-view.hpp"
#include "views/pkt-checkpoints-build-progress-view.hpp"
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

auto screenInited = false;

/*
 * Releases the terminal.
 */
void finiScreen()
{
    if (screenInited) {
        endwin();
        screenInited = false;
    }
}

/*
 * Returns true if the terminal size has the minimum dimension.
 */
bool termSizeOk()
{
    return COLS >= 80 && LINES >= 16;
}

/*
 * Initializes and takes control of the terminal.
 */
void initScreen()
{
    initscr();
    screenInited = true;

    if (!has_colors() || !termSizeOk()) {
        finiScreen();

        if (!has_colors()) {
            throw CmdError {"Cannot continue: your terminal doesn't support colors."};
        } else {
            assert(!termSizeOk());

            throw CmdError {"Cannot continue: terminal size must be at least 80×16."};
        }
    }

    // do not print user keys
    noecho();

    // make keys immediately available (disable line buffering)
    cbreak();

    // faster cursor motion and detect return key
    nonl();

    // enable special keys
    keypad(stdscr, TRUE);

    // disable cursor block
    curs_set(0);

    // use colors
    start_color();

    // support user's foreground/background colors
    use_default_colors();
}

void sigHandler(const int signo)
{
    if (signo == SIGINT) {
        finiScreen();
        std::cerr << '\n';
        utils::error() << "Interrupted by user.\n";

        // just exit immediately
        std::exit(0);
    }
}

void registerSignals()
{
    const auto ret = signal(SIGINT, sigHandler);

    assert(ret != SIG_ERR);
    static_cast<void>(ret);
}

void init()
{
    registerSignals();
    initScreen();
}

void buildIndexes(InspectCmdState& appState, const Stylist& stylist)
{
    const auto screenRect = Rect {{0, 0}, static_cast<Size>(COLS), static_cast<Size>(LINES)};
    const auto view = std::make_unique<PktIndexBuildProgressView>(screenRect, stylist);
    auto func = [&view](const PktIndexEntry& entry) {
        view->pktIndexEntry(entry);
        view->refresh();
        doupdate();
    };

    view->focus();
    view->isVisible(true);
    view->refresh(true);

    for (auto& dsfStateUp : appState.dsFileStates()) {
        auto& dsf = dsfStateUp->dsFile();

        view->dsFile(dsf);
        dsf.buildIndex(func, 443);
    }
}

void showFullScreenMessage(const std::string& msg, const Stylist& stylist)
{
    const auto screenRect = Rect {{0, 0}, static_cast<Size>(COLS), static_cast<Size>(LINES)};
    const auto view = std::make_unique<SimpleMsgView>(screenRect, stylist);

    view->msg(msg);
    view->focus();
    view->isVisible(true);
    view->refresh();
    doupdate();
}

class PktCheckpointsBuildProgressUpdater final :
    public PktCheckpointsBuildListener
{
public:
    explicit PktCheckpointsBuildProgressUpdater(const Stylist& stylist, bool& redrawCurScreen) :
        _stylist {&stylist},
        _redrawCurScreen {&redrawCurScreen}
    {
    }

private:
    void _startBuild(const PktIndexEntry& pktIndexEntry) override
    {
        if (pktIndexEntry.effectiveTotalLen() < 2_MiB) {
            // too fast anyway
            return;
        }

        const auto rect = Rect {{4, 4}, static_cast<Size>(COLS) - 8, 13};

        _view = std::make_unique<PktCheckpointsBuildProgressView>(rect, *_stylist);
        _view->focus();
        _view->isVisible(true);
        _view->pktIndexEntry(pktIndexEntry);
        _view->refresh(true);
        doupdate();
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

        _view->er(er);
        _view->refresh();
        doupdate();
    }

    void _endBuild() override
    {
        if (_view) {
            _view->isVisible(false);
            _view = nullptr;
        }

        // redraw current screen
        *_redrawCurScreen = true;
    }

private:
    Index _count = 0;
    std::unique_ptr<PktCheckpointsBuildProgressView> _view;
    const Stylist * const _stylist;
    bool * const _redrawCurScreen;
};

void startInteractive(const InspectCfg& cfg)
{
    auto stylist = std::make_unique<const Stylist>();

    showFullScreenMessage("Opening data stream files...", *stylist);

    Screen *curScreen = nullptr;
    bool redrawCurScreen = false;
    PktCheckpointsBuildProgressUpdater updater {*stylist, redrawCurScreen};
    auto appState = std::make_unique<InspectCmdState>(cfg.paths(), updater);

    if (appState->dsFileStates().empty()) {
        throw CmdError {"All data stream files to inspect are empty."};
    }

    auto screenRect = Rect {{0, 0}, static_cast<Size>(COLS), static_cast<Size>(LINES) - 1};

    /*
     * At this point, the state isn't ready: data stream files have no
     * packet indexes, and there's no active packet built. This is
     * because we want to provide feedback to the user because it could
     * be a long process. Build indexes first.
     */
    buildIndexes(*appState, *stylist);

    /*
     * Show this message because some views created by the screens below
     * can perform some "heavy" caching operations initially.
     */
    showFullScreenMessage("Building caches...", *stylist);

    // status
    auto statusView = std::make_unique<StatusView>(Rect {{0, screenRect.h}, screenRect.w, 1},
                                                   *stylist, *appState);

    // create screens
    const auto inspectScreen = std::make_unique<InspectScreen>(screenRect, cfg, *stylist,
                                                               *appState);
    const auto pktsScreen = std::make_unique<PktsScreen>(screenRect, cfg, *stylist, *appState);
    const auto dsfScreen = std::make_unique<DsFilesScreen>(screenRect, cfg, *stylist, *appState);
    const auto helpScreen = std::make_unique<HelpScreen>(screenRect, cfg, *stylist, *appState);
    const auto dtsScreen = std::make_unique<DtsScreen>(screenRect, cfg, *stylist, *appState);
    const auto traceInfoScreen = std::make_unique<TraceInfoScreen>(screenRect, cfg, *stylist,
                                                                   *appState);
    const std::vector<Screen *> screens {
        inspectScreen.get(),
        pktsScreen.get(),
        dsfScreen.get(),
        helpScreen.get(),
        dtsScreen.get(),
        traceInfoScreen.get(),
    };

    // goto first packet if available: this creates it and shows the progress
    showFullScreenMessage("Selecting initial packet...", *stylist);

    if (appState->activeDsFileState().dsFile().pktCount() > 0) {
        appState->gotoPkt(0);
    }

    // draw status
    statusView->isVisible(true);
    statusView->redraw();

    // initial screen depends on the situation
    if (appState->dsFileStateCount() == 1) {
        if (appState->activeDsFileState().dsFile().pktCount() == 0) {
            curScreen = dsfScreen.get();
        } else if (appState->activeDsFileState().dsFile().pktCount() == 1) {
            curScreen = inspectScreen.get();
        } else {
            curScreen = pktsScreen.get();
        }
    } else {
        curScreen = dsfScreen.get();
    }

    curScreen->isVisible(true);
    doupdate();

    Screen *prevScreen = nullptr;
    auto done = false;
    auto wantsToQuit = false;

    while (!done) {
        const auto ch = getch();
        auto refreshStatus = true;

        if (wantsToQuit) {
            if (ch == 'y' || ch == 'Y') {
                done = true;
                continue;
            } else {
                wantsToQuit = false;
                clear();
                refresh();
                statusView->isVisible(true);
                curScreen->isVisible(true);
                statusView->redraw();
                curScreen->redraw();
                doupdate();
            }

            continue;
        }

        switch (ch) {
        case KEY_RESIZE:
            // it looks like clearing and refreshing `stdscr` is required
            clear();
            refresh();

            if (!termSizeOk()) {
                stylist->error();
                mvprintw(0, 0, "Terminal size must be at least 80×16 (currently %d×%d).", COLS,
                         LINES);
                refreshStatus = false;
                break;
            }

            screenRect = Rect {{0, 0}, static_cast<Size>(COLS), static_cast<Size>(LINES) - 1};
            statusView->moveAndResize(Rect {{0, screenRect.h}, screenRect.w, 1});

            for (const auto screen : screens) {
                screen->resize(screenRect.w, screenRect.h);
            }

            curScreen->redraw();
            statusView->redraw();
            break;

        case 'r':
        case 12:
            clear();
            refresh();
            statusView->redraw();
            curScreen->redraw();
            break;

        case 'p':
            if (curScreen == pktsScreen.get()) {
                break;
            }

            curScreen->isVisible(false);
            curScreen = pktsScreen.get();
            curScreen->isVisible(true);
            break;

        case 'f':
            if (curScreen == dsfScreen.get()) {
                break;
            }

            curScreen->isVisible(false);
            curScreen = dsfScreen.get();
            curScreen->isVisible(true);
            break;

        case 'd':
            if (curScreen == dtsScreen.get()) {
                break;
            }

            curScreen->isVisible(false);

            if (curScreen == inspectScreen.get()) {
                dtsScreen->highlightCurDt();
            } else {
                dtsScreen->clearHighlight();
            }

            curScreen = dtsScreen.get();
            curScreen->isVisible(true);
            break;

        case 'i':
            if (curScreen == traceInfoScreen.get()) {
                break;
            }

            curScreen->isVisible(false);
            curScreen = traceInfoScreen.get();
            curScreen->isVisible(true);
            break;

        case 'h':
        case 'H':
        case '?':
            if (curScreen == helpScreen.get()) {
                break;
            }

            prevScreen = curScreen;
            curScreen->isVisible(false);
            curScreen = helpScreen.get();
            curScreen->isVisible(true);
            break;

        case 'q':
        case 27:
            if (curScreen == inspectScreen.get()) {
                curScreen->isVisible(false);
                statusView->isVisible(false);
                showFullScreenMessage("Press `Y` to quit, or any other key to stay.", *stylist);
                wantsToQuit = true;
            } else if (curScreen == helpScreen.get()) {
                curScreen->isVisible(false);
                curScreen = prevScreen;
                curScreen->isVisible(true);
            } else {
                curScreen->isVisible(false);
                curScreen = inspectScreen.get();
                curScreen->isVisible(true);
            }

            break;

        case KEY_F(10):
        case 'Q':
            done = true;
            break;

        default:
            const auto reaction = curScreen->handleKey(ch);

            switch (reaction) {
            case KeyHandlingReaction::RETURN_TO_INSPECT:
                curScreen->isVisible(false);
                curScreen = inspectScreen.get();
                curScreen->isVisible(true);
                break;

            case KeyHandlingReaction::RETURN_TO_PKTS:
                curScreen->isVisible(false);
                curScreen = pktsScreen.get();
                curScreen->isVisible(true);
                break;

            case KeyHandlingReaction::CONTINUE:
                break;
            }
        }

        if (redrawCurScreen) {
            curScreen->redraw();
            redrawCurScreen = false;
        }

        if (refreshStatus) {
            statusView->refresh();
        }

        doupdate();
    }
}

} // namespace

void inspectCmd(const InspectCfg& cfg)
{
    init();

    /*
     * Initial refresh() because getch() implicitly calls refresh(),
     * which dumps `stdscr` the first time, effectively clearing the
     * screen.
     */
    refresh();

    // release the terminal whatever the outcome
    try {
        startInteractive(cfg);
    } catch (...) {
        finiScreen();
        throw;
    }

    finiScreen();
}

} // namespace jacques
