# SPDX-FileCopyrightText: 2018 Philippe Proulx <eepp.ca>
# SPDX-License-Identifier: LicenseRef-Coke-Zero-Ware-1.0

# find notcurses (optional)
option (OPT_WITHOUT_INSPECT_CMD "Don't build the `inspect` command")

if (NOT OPT_WITHOUT_INSPECT_CMD)
    find_package (PkgConfig)

    if (PkgConfig_FOUND)
        pkg_check_modules (NOTCURSES_CORE IMPORTED_TARGET notcurses-core>=3.0)
    endif ()

    if (NOTCURSES_CORE_FOUND)
        set (JACQUES_HAS_INSPECT_CMD ON)
        add_compile_definitions (JACQUES_HAS_INSPECT_CMD)
        message (STATUS "Will build the `inspect` command")
    else ()
        message (WARNING "Missing notcurses-core (≥ 3.0) dependency: won't build the `inspect` command.")
    endif ()
endif ()

# find Boost
find_package (
    Boost 1.70 REQUIRED
    COMPONENTS
        program_options
        filesystem
)

# Fetch and build a pinned version of yactfr (static library, linked
# into the `jacquesctf` executable).
#
# Set `FETCHCONTENT_SOURCE_DIR_YACTFR` to the path of a local yactfr
# source tree to use it instead.
include (FetchContent)
FetchContent_Declare (
    yactfr
    GIT_REPOSITORY https://github.com/eepp/yactfr.git
    GIT_TAG 19527dca75852a0942da8364098b734404fb6973
)
FetchContent_MakeAvailable (yactfr)

# Fetch and build a pinned version of {fmt} (12.2.0, static library,
# linked into the `jacquesctf` executable).
#
# Set `FETCHCONTENT_SOURCE_DIR_FMT` to the path of a local {fmt} source
# tree to use it instead.
FetchContent_Declare (
    fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG 1be298e1bd68957e4cd352e1f676f00e07dcfb57
)
FetchContent_MakeAvailable (fmt)

# Jacques CTF program
if (JACQUES_HAS_INSPECT_CMD)
    set (
        JACQUES_INSPECT_CMD_SOURCES
        inspect-cmd/state/inspect-cmd-state.cpp
        inspect-cmd/ui/color.cpp
        inspect-cmd/ui/inspect-cmd.cpp
        inspect-cmd/ui/screens/ds-files-screen.cpp
        inspect-cmd/ui/screens/dts-screen.cpp
        inspect-cmd/ui/screens/help-screen.cpp
        inspect-cmd/ui/screens/inspect-screen.cpp
        inspect-cmd/ui/screens/pkts-screen.cpp
        inspect-cmd/ui/screens/screen.cpp
        inspect-cmd/ui/screens/trace-info-screen.cpp
        inspect-cmd/ui/search-ctrl.cpp
        inspect-cmd/ui/stylist.cpp
        inspect-cmd/ui/term.cpp
        inspect-cmd/ui/views/abstract-dt-details.cpp
        inspect-cmd/ui/views/ds-file-table-view.cpp
        inspect-cmd/ui/views/dst-table-view.cpp
        inspect-cmd/ui/views/dt-details-from-dt.cpp
        inspect-cmd/ui/views/dt-details.cpp
        inspect-cmd/ui/views/dt-explorer-view.cpp
        inspect-cmd/ui/views/er-table-view.cpp
        inspect-cmd/ui/views/ert-table-view.cpp
        inspect-cmd/ui/views/help-view.cpp
        inspect-cmd/ui/views/mapping-or-flag-details.cpp
        inspect-cmd/ui/views/pkt-data-view.cpp
        inspect-cmd/ui/views/pkt-decoding-error-details-view.cpp
        inspect-cmd/ui/views/pkt-region-info-view.cpp
        inspect-cmd/ui/views/pkt-table-view.cpp
        inspect-cmd/ui/views/progress-view.cpp
        inspect-cmd/ui/views/scroll-view.cpp
        inspect-cmd/ui/views/search-input-view.cpp
        inspect-cmd/ui/views/simple-msg-view.cpp
        inspect-cmd/ui/views/status-view.cpp
        inspect-cmd/ui/views/sub-dt-explorer-view.cpp
        inspect-cmd/ui/views/table-view.cpp
        inspect-cmd/ui/views/trace-info-view.cpp
        inspect-cmd/ui/views/view.cpp
    )
endif ()

if (JACQUES_HAS_INSPECT_CMD)
    set (
        JACQUES_INSPECT_COMMON_SOURCES
        inspect-common/app-state.cpp
        inspect-common/ds-file-state.cpp
        inspect-common/pkt-state.cpp
        inspect-common/search-query.cpp
        inspect-common/table-nav.cpp
    )
endif ()

add_executable (
    jacquesctf
    ${JACQUES_INSPECT_CMD_SOURCES}
    ${JACQUES_INSPECT_COMMON_SOURCES}
    cfg.cpp
    copy-pkts-cmd.cpp
    create-lttng-index-cmd.cpp
    data/content-pkt-region.cpp
    data/data-len.cpp
    data/ds-file.cpp
    data/dt-path.cpp
    data/duration.cpp
    data/er.cpp
    data/error-pkt-region.cpp
    data/mem-mapped-file.cpp
    data/metadata.cpp
    data/padding-pkt-region.cpp
    data/pkt-checkpoints-build-listener.cpp
    data/pkt-checkpoints.cpp
    data/pkt-index-entry.cpp
    data/pkt-region-visitor.cpp
    data/pkt-region.cpp
    data/pkt-segment.cpp
    data/pkt.cpp
    data/scope.cpp
    data/trace.cpp
    data/ts.cpp
    jacques.cpp
    list-pkts-cmd.cpp
    print-metadata-text-cmd.cpp
    unicode.cpp
    utils.cpp
)
target_include_directories (
    jacquesctf PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${Boost_INCLUDE_DIRS}
)
target_link_libraries (
    jacquesctf
    Boost::program_options
    Boost::filesystem
    yactfr::yactfr
    fmt::fmt
)
if (JACQUES_HAS_INSPECT_CMD)
    target_link_libraries (jacquesctf PkgConfig::NOTCURSES_CORE)
endif ()
target_compile_definitions (
    jacquesctf PRIVATE
    "-DJACQUES_VERSION=\"${PROJECT_VERSION}\""
)
install (
    TARGETS jacquesctf
    RUNTIME DESTINATION bin
)
