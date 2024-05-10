# Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
#
# Unauthorized copying of this file, via any medium, is strictly
# prohibited. Proprietary and confidential.

# find ncurses (optional)
option (OPT_WITHOUT_INSPECT_CMD "Don't build the `inspect` command")

if (NOT OPT_WITHOUT_INSPECT_CMD)
    set (CURSES_NEED_NCURSES TRUE)
    find_package (Curses)

    if (CURSES_FOUND)
        set (JACQUES_HAS_INSPECT_CMD ON)
        add_compile_definitions (JACQUES_HAS_INSPECT_CMD)
        message (STATUS "Will build the `inspect` command")
    else ()
        message (WARNING "Missing ncurses dependency: won't build the `inspect` command.")
    endif ()
endif ()

# find Boost
find_package (
    Boost 1.58 REQUIRED
    COMPONENTS
        program_options
        filesystem
)

# find yactfr
find_library (YACTFR_LIB yactfr)

if (NOT YACTFR_LIB)
    message (FATAL_ERROR "Cannot find the yactfr library")
else ()
    message (STATUS "yactfr library: ${YACTFR_LIB}")
endif ()

find_file (YACTFR_INCLUDE_FILE yactfr/yactfr.hpp)

if (NOT YACTFR_INCLUDE_FILE)
    message (FATAL_ERROR "Cannot find the yactfr include directory")
else ()
    get_filename_component (YACTFR_INCLUDE_DIR "${YACTFR_INCLUDE_FILE}" DIRECTORY)
    get_filename_component (YACTFR_INCLUDE_DIR "${YACTFR_INCLUDE_DIR}" DIRECTORY)
    message (STATUS "yactfr include directory: ${YACTFR_INCLUDE_DIR}")
endif ()

# threading
find_package (Threads REQUIRED)

# Jacques CTF program
if (JACQUES_HAS_INSPECT_CMD)
    set (
        JACQUES_INSPECT_CMD_SOURCES
        inspect-cmd/state/inspect-cmd-state.cpp
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
        inspect-cmd/ui/views/abstract-dt-details.cpp
        inspect-cmd/ui/views/ds-file-table-view.cpp
        inspect-cmd/ui/views/dst-table-view.cpp
        inspect-cmd/ui/views/dt-details-from-dt.cpp
        inspect-cmd/ui/views/dt-details.cpp
        inspect-cmd/ui/views/dt-explorer-view.cpp
        inspect-cmd/ui/views/er-table-view.cpp
        inspect-cmd/ui/views/ert-table-view.cpp
        inspect-cmd/ui/views/help-view.cpp
        inspect-cmd/ui/views/input-view.cpp
        inspect-cmd/ui/views/mapping-or-flag-details.cpp
        inspect-cmd/ui/views/pkt-checkpoints-build-progress-view.cpp
        inspect-cmd/ui/views/pkt-data-view.cpp
        inspect-cmd/ui/views/pkt-decoding-error-details-view.cpp
        inspect-cmd/ui/views/pkt-index-build-progress-view.cpp
        inspect-cmd/ui/views/pkt-region-info-view.cpp
        inspect-cmd/ui/views/pkt-table-view.cpp
        inspect-cmd/ui/views/scroll-view.cpp
        inspect-cmd/ui/views/search-input-view.cpp
        inspect-cmd/ui/views/simple-msg-view.cpp
        inspect-cmd/ui/views/status-view.cpp
        inspect-cmd/ui/views/sub-dt-explorer-view.cpp
        inspect-cmd/ui/views/table-view.cpp
        inspect-cmd/ui/views/text-input-view.cpp
        inspect-cmd/ui/views/trace-info-view.cpp
        inspect-cmd/ui/views/view.cpp
    )
endif ()

if (JACQUES_HAS_INSPECT_CMD)
    set (
        JACQUES_INSPECT_COMMON_SOURCES
        inspect-common/app-state.cpp
        inspect-common/common-inspect-table-view.cpp
        inspect-common/ds-file-state.cpp
        inspect-common/pkt-state.cpp
        inspect-common/search-query.cpp
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
    utils.cpp
)
target_include_directories (
    jacquesctf PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CURSES_INCLUDE_DIR}
    ${Boost_INCLUDE_DIRS}
    ${YACTFR_INCLUDE_DIR}
)
target_link_libraries (
    jacquesctf
    ${CURSES_LIBRARIES}
    Boost::program_options
    Boost::filesystem
    Threads::Threads
    ${YACTFR_LIB}
)
target_compile_definitions (
    jacquesctf PRIVATE
    "-DJACQUES_VERSION=\"${PROJECT_VERSION}\""
)
install (
    TARGETS jacquesctf
    RUNTIME DESTINATION bin
)
