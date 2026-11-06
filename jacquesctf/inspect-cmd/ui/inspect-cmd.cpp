/*
 * SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
 * SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0
 */

#include <iostream>
#include <stdexcept>
#include <optional>
#include <memory>
#include <signal.h>
#include <unistd.h>
#include <notcurses/notcurses.h>

#include "inspect-cmd.hpp"
#include "key.hpp"
#include "term.hpp"
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

void startInteractive(const InspectCfg& cfg)
{
    auto stylist = std::make_unique<const Stylist>();

    auto msgView = showFullScreenMessage("Opening data stream files…", *stylist);

    Screen *curScreen = nullptr;
    PktCheckpointsBuildProgressUpdater updater {*stylist};
    auto appState = std::make_unique<InspectCmdState>(cfg.paths(), updater);

    if (appState->dsFileStates().empty()) {
        throw CmdError {"All data stream files to inspect are empty."};
    }

    auto screenRect = term::size();

    --screenRect.h;

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
    msgView = showFullScreenMessage("Building caches…", *stylist);

    // status
    auto statusView = std::make_unique<StatusView>(Rect {{0, screenRect.h}, screenRect.w, 1},
                                                   *stylist, *appState);

    std::unique_ptr<SimpleMsgView> quitMsgView;

    /*
     * Renders the whole terminal, from bottom to top: the status view,
     * the current screen, and the quit message, if any.
     */
    const auto renderFrame = [&statusView, &curScreen, &quitMsgView] {
        statusView->render();
        curScreen->render();

        if (quitMsgView) {
            quitMsgView->render();
        }

        term::update();
    };

    // create screens
    const auto inspectScreen = std::make_unique<InspectScreen>(screenRect, cfg, *stylist,
                                                               *appState, renderFrame);
    const auto pktsScreen = std::make_unique<PktsScreen>(screenRect, cfg, *stylist, *appState,
                                                         renderFrame);
    const auto dsfScreen = std::make_unique<DsFilesScreen>(screenRect, cfg, *stylist, *appState);
    const auto helpScreen = std::make_unique<HelpScreen>(screenRect, cfg, *stylist, *appState);
    const auto dtsScreen = std::make_unique<DtsScreen>(screenRect, cfg, *stylist, *appState,
                                                       renderFrame);
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
    msgView = showFullScreenMessage("Selecting initial packet…", *stylist);

    if (appState->activeDsFileState().dsFile().pktCount() > 0) {
        appState->gotoPkt(0);
    }

    // remove the last message
    msgView.reset();

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

    Screen *prevScreen = nullptr;
    auto done = false;
    auto termIsTooSmall = false;

    // `q` or Escape: quit the current screen or ask to quit the program
    const auto quitCurScreen = [&] {
        if (curScreen == inspectScreen.get()) {
            curScreen->isVisible(false);
            statusView->isVisible(false);
            quitMsgView = showFullScreenMessage("Press `Y` to quit, or any other key to stay.",
                                                *stylist);
        } else if (curScreen == helpScreen.get()) {
            curScreen->isVisible(false);
            curScreen = prevScreen;
            curScreen->isVisible(true);
        } else {
            curScreen->isVisible(false);
            curScreen = inspectScreen.get();
            curScreen->isVisible(true);
        }
    };

    while (!done) {
        renderFrame();

        const auto key = term::readKey();

        if (termIsTooSmall && key.special() != SpecialKey::RESIZE &&
                key.special() != SpecialKey::F10 && key.ch() != U'Q') {
            // only accept a resize or an immediate quit
            continue;
        }

        if (quitMsgView) {
            if (key.ch() == U'y' || key.ch() == U'Y') {
                done = true;
                continue;
            } else {
                quitMsgView.reset();
                statusView->isVisible(true);
                curScreen->isVisible(true);
            }

            continue;
        }

        auto passToCurScreen = false;

        if (const auto specialKey = key.special()) {
            switch (*specialKey) {
            case SpecialKey::RESIZE:
            {
                term::repaint();

                auto& stdPlane = term::stdPlane();

                if (!term::sizeOk()) {
                    // hide everything and show an error on the standard plane
                    curScreen->isVisible(false);
                    statusView->isVisible(false);
                    ncplane_erase(&stdPlane);
                    stylist->apply(stdPlane, Stylist::Role::ERROR);

                    const auto termSize = term::size();

                    ncplane_printf_yx(
                        &stdPlane, 0, 0,
                        "Terminal size must be at least 80×16 (currently %llu×%llu).",
                        static_cast<unsigned long long>(termSize.w),
                        static_cast<unsigned long long>(termSize.h)
                    );
                    termIsTooSmall = true;
                    break;
                }

                screenRect = term::size();
                --screenRect.h;
                statusView->moveAndResize(Rect {{0, screenRect.h}, screenRect.w, 1});

                for (const auto screen : screens) {
                    screen->resize(screenRect.w, screenRect.h);
                }

                if (termIsTooSmall) {
                    ncplane_erase(&stdPlane);
                    statusView->isVisible(true);
                    curScreen->isVisible(true);
                    termIsTooSmall = false;
                }

                break;
            }

            case SpecialKey::ESCAPE:
                quitCurScreen();
                break;

            case SpecialKey::F10:
                done = true;
                break;

            default:
                passToCurScreen = true;
                break;
            }
        } else {
            switch (*key.ch()) {
            case 'r':
            case ctrlCh('L'):
                term::repaint();
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
                quitCurScreen();
                break;

            case 'Q':
                done = true;
                break;

            default:
                passToCurScreen = true;
                break;
            }
        }

        if (passToCurScreen) {
            switch (curScreen->handleKey(key)) {
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
    }
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
