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

#include "inspect-command.hpp"
#include "config.hpp"
#include "../state/state.hpp"
#include "stylist.hpp"
#include "screens/inspect-screen.hpp"
#include "screens/help-screen.hpp"
#include "screens/packets-screen.hpp"
#include "screens/data-stream-files-screen.hpp"
#include "screens/data-types-screen.hpp"
#include "screens/trace-info-screen.hpp"
#include "views/status-view.hpp"
#include "views/packet-index-build-progress-view.hpp"
#include "views/packet-checkpoints-build-progress-view.hpp"
#include "views/simple-message-view.hpp"
#include "utils.hpp"
#include "data/packet-checkpoints-build-listener.hpp"
#include "command-error.hpp"
#include "data/data-size.hpp"
#include "data/packet-region.hpp"
#include "data/padding-packet-region.hpp"
#include "data/content-packet-region.hpp"
#include "data/error-packet-region.hpp"

namespace jacques {

static bool screenInited = false;

/*
 * Releases the terminal.
 */
static void finiScreen()
{
    if (screenInited) {
        endwin();
        screenInited = false;
    }
}

/*
 * Returns true if the terminal's size has the minimum dimension.
 */
static bool termSizeOk()
{
    return COLS >= 80 && LINES >= 16;
}

/*
 * Initializes and takes control of the terminal.
 */
static void initScreen()
{
    initscr();
    screenInited = true;

    if (!has_colors() || !termSizeOk()) {
        finiScreen();

        if (!has_colors()) {
            throw CommandError {
                "Cannot continue: your terminal does not support colors."
            };
        } else if (!termSizeOk()) {
            throw CommandError {
                "Cannot continue: terminal size must be at least 80x16."
            };
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

static void sigHandler(const int signo)
{
    if (signo == SIGINT) {
        finiScreen();
        std::cerr << '\n';
        utils::error() << "Interrupted by user.\n";

        // just exit immediately
        std::exit(0);
    }
}

static void registerSignals()
{
    auto ret = signal(SIGINT, sigHandler);

    assert(ret != SIG_ERR);
    JACQUES_UNUSED(ret);
}

static void init()
{
    registerSignals();
    initScreen();
}

static void buildIndexes(State& state, const Stylist& stylist)
{
    const auto screenRect = Rectangle {{0, 0}, static_cast<Size>(COLS),
                                       static_cast<Size>(LINES)};
    const auto view = std::make_unique<PacketIndexBuildProgressView>(screenRect,
                                                                     stylist);
    auto func = [&view](const PacketIndexEntry& entry) {
        view->packetIndexEntry(entry);
        view->refresh();
        doupdate();
    };

    view->focus();
    view->isVisible(true);
    view->refresh(true);

    for (auto& dsfStateUp : state.dataStreamFileStates()) {
        auto& dsf = dsfStateUp->dataStreamFile();

        view->dataStreamFile(dsf);
        dsf.buildIndex(func, 443);
    }
}

static void showFullScreenMessage(const std::string& msg,
                                  const Stylist& stylist)
{
    const auto screenRect = Rectangle {{0, 0}, static_cast<Size>(COLS),
                                       static_cast<Size>(LINES)};
    const auto view = std::make_unique<SimpleMessageView>(screenRect, stylist);

    view->message(msg);
    view->focus();
    view->isVisible(true);
    view->refresh();
    doupdate();
}

class PacketCheckpointsBuildProgressUpdater :
    public PacketCheckpointsBuildListener
{
public:
    explicit PacketCheckpointsBuildProgressUpdater(const Stylist& stylist,
                                                   bool& redrawCurScreen) :
        _stylist {&stylist},
        _redrawCurScreen {&redrawCurScreen}
    {
    }

private:
    void _startBuild(const PacketIndexEntry& packetIndexEntry) override
    {
        if (packetIndexEntry.effectiveTotalSize() < 2_MiB) {
            // too fast anyway
            return;
        }

        const auto rect = Rectangle {{4, 4}, static_cast<Size>(COLS) - 8, 13};

        _view = std::make_unique<PacketCheckpointsBuildProgressView>(rect,
                                                                     *_stylist);
        _view->focus();
        _view->isVisible(true);
        _view->packetIndexEntry(packetIndexEntry);
        _view->refresh(true);
        doupdate();
        _count = 0;
    }

    void _update(const EventRecord& eventRecord) override
    {
        if (!_view) {
            return;
        }

        if (_count++ % 13 != 0) {
            return;
        }

        _view->eventRecord(eventRecord);
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
    std::unique_ptr<PacketCheckpointsBuildProgressView> _view;
    const Stylist * const _stylist;
    bool * const _redrawCurScreen;
};

class PrintVisitor :
    public boost::static_visitor<>
{
public:
    template <typename T>
    void operator()(const T& val) const
    {
        std::cout << val;
    }
};

static void startInteractive(const InspectConfig& cfg)
{
    auto stylist = std::make_unique<const Stylist>();

    showFullScreenMessage("Opening data stream files...", *stylist);

    Screen *curScreen = nullptr;
    bool redrawCurScreen = false;
    auto packetCheckpointsBuildProgressUpdater = std::make_shared<PacketCheckpointsBuildProgressUpdater>(*stylist,
                                                                                                         redrawCurScreen);
    auto state = std::make_unique<State>(cfg.paths(),
                                         packetCheckpointsBuildProgressUpdater);

    if (state->dataStreamFileStates().empty()) {
        throw CommandError {"All data stream files to inspect are empty."};
    }

    auto screenRect = Rectangle {{0, 0}, static_cast<Size>(COLS),
                                         static_cast<Size>(LINES) - 1};

    /*
     * At this point, the state is not ready. Data stream files have no
     * packet indexes, and there's no active packet built. This is
     * because we want to provide feedback to the user because it could
     * be a long process. Build indexes first.
     */
    buildIndexes(*state, *stylist);

    /*
     * Show this message because some views created by the screens below
     * can perform some "heavy" caching operations initially.
     */
    showFullScreenMessage("Building caches...", *stylist);

    // status
    auto statusView = std::make_unique<StatusView>(Rectangle {{0, screenRect.h},
                                                              screenRect.w, 1},
                                                   *stylist, *state);

    // create screens
    const auto inspectScreen = std::make_unique<InspectScreen>(screenRect, cfg,
                                                               *stylist,
                                                               *state);
    const auto packetsScreen = std::make_unique<PacketsScreen>(screenRect, cfg,
                                                               *stylist,
                                                               *state);
    const auto dsfScreen = std::make_unique<DataStreamFilesScreen>(screenRect,
                                                                   cfg,
                                                                   *stylist,
                                                                   *state);
    const auto helpScreen = std::make_unique<HelpScreen>(screenRect, cfg,
                                                         *stylist, *state);
    const auto dataTypesScreen = std::make_unique<DataTypesScreen>(screenRect,
                                                                   cfg,
                                                                   *stylist,
                                                                   *state);
    const auto traceInfoScreen = std::make_unique<TraceInfoScreen>(screenRect,
                                                                   cfg,
                                                                   *stylist,
                                                                   *state);
    const std::vector<Screen *> screens {
        inspectScreen.get(),
        packetsScreen.get(),
        dsfScreen.get(),
        helpScreen.get(),
        dataTypesScreen.get(),
        traceInfoScreen.get(),
    };

    // goto first packet if available: this creates it and shows the progress
    showFullScreenMessage("Selecting initial packet...", *stylist);

    if (state->activeDataStreamFileState().dataStreamFile().packetCount() > 0) {
        state->gotoPacket(0);
    }

    // draw status
    statusView->isVisible(true);
    statusView->redraw();

    // initial screen depends on the situation
    if (state->dataStreamFileStateCount() == 1) {
        if (state->activeDataStreamFileState().dataStreamFile().packetCount() == 0) {
            curScreen = dsfScreen.get();
        } else if (state->activeDataStreamFileState().dataStreamFile().packetCount() == 1) {
            curScreen = inspectScreen.get();
        } else {
            curScreen = packetsScreen.get();
        }
    } else {
        curScreen = dsfScreen.get();
    }

    curScreen->isVisible(true);
    doupdate();

    Screen *prevScreen = nullptr;
    bool done = false;
    bool wantsToQuit = false;

    while (!done) {
        const auto ch = getch();
        bool refreshStatus = true;

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
            // it looks like clearing and refreshing stdscr is required
            clear();
            refresh();

            if (!termSizeOk()) {
                stylist->error();
                mvprintw(0, 0, "Terminal size must be at least 80x16 (currently %dx%d).",
                         COLS, LINES);
                refreshStatus = false;
                break;
            }

            screenRect = Rectangle {{0, 0}, static_cast<Size>(COLS),
                                    static_cast<Size>(LINES) - 1};

            statusView->moveAndResize(Rectangle {{0, screenRect.h},
                                                 screenRect.w, 1});

            for (auto screen : screens) {
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
            if (curScreen == packetsScreen.get()) {
                break;
            }

            curScreen->isVisible(false);
            curScreen = packetsScreen.get();
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
            if (curScreen == dataTypesScreen.get()) {
                break;
            }

            curScreen->isVisible(false);

            if (curScreen == inspectScreen.get()) {
                dataTypesScreen->highlightCurrentDataType();
            } else {
                dataTypesScreen->clearHighlight();
            }

            curScreen = dataTypesScreen.get();
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
                showFullScreenMessage("Press Y to quit, or any other key to stay.", *stylist);
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

            case KeyHandlingReaction::RETURN_TO_PACKETS:
                curScreen->isVisible(false);
                curScreen = packetsScreen.get();
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

void inspectCommand(const InspectConfig& cfg)
{
    init();

    /*
     * Initial refresh() because getch() implicitly calls refresh(),
     * which dumps stdscr the first time, effectively clearing the
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
