# Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
#
# Unauthorized copying of this file, via any medium, is strictly
# prohibited. Proprietary and confidential.

# find ncurses
set (CURSES_NEED_NCURSES TRUE)
find_package (Curses REQUIRED)

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

# Jacques CTF program
add_executable (
    jacquesctf
    config.cpp
    jacques.cpp
    state/active-data-stream-file-changed-message.cpp
    state/active-packet-changed-message.cpp
    state/content-data-region.cpp
    state/cur-offset-in-packet-changed-message.cpp
    state/data-region.cpp
    state/data-region-visitor.cpp
    state/data-segment.cpp
    state/data-size.cpp
    state/data-stream-file-state.cpp
    state/data-stream-file.cpp
    state/duration.cpp
    state/error-data-region.cpp
    state/event-record.cpp
    state/memory-mapped-file.cpp
    state/message-visitor.cpp
    state/metadata-store.cpp
    state/metadata.cpp
    state/packet-checkpoints-build-listener.cpp
    state/packet-checkpoints.cpp
    state/packet-index-entry.cpp
    state/packet.cpp
    state/padding-data-region.cpp
    state/scope.cpp
    state/search-parser.cpp
    state/state.cpp
    state/timestamp.cpp
    ui/interactive.cpp
    ui/screens/data-stream-files-screen.cpp
    ui/screens/data-types-screen.cpp
    ui/screens/help-screen.cpp
    ui/screens/inspect-screen.cpp
    ui/screens/packets-screen.cpp
    ui/screens/screen.cpp
    ui/screens/trace-info-screen.cpp
    ui/search-controller.cpp
    ui/stylist.cpp
    ui/views/abstract-data-type-details.cpp
    ui/views/data-region-info-view.cpp
    ui/views/data-stream-file-table-view.cpp
    ui/views/data-stream-type-table-view.cpp
    ui/views/data-type-details.cpp
    ui/views/data-type-explorer-view.cpp
    ui/views/event-record-table-view.cpp
    ui/views/event-record-type-table-view.cpp
    ui/views/help-view.cpp
    ui/views/input-view.cpp
    ui/views/packet-checkpoints-build-progress-view.cpp
    ui/views/packet-decoding-error-details-view.cpp
    ui/views/packet-index-build-progress-view.cpp
    ui/views/packet-table-view.cpp
    ui/views/scroll-view.cpp
    ui/views/search-input-view.cpp
    ui/views/simple-message-view.cpp
    ui/views/status-view.cpp
    ui/views/table-view.cpp
    ui/views/text-input-view.cpp
    ui/views/trace-info-view.cpp
    ui/views/view.cpp
    utils.cpp
)
target_include_directories (
    jacquesctf PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/ui/views
    ${CMAKE_CURRENT_SOURCE_DIR}/ui/screens
    ${CMAKE_CURRENT_SOURCE_DIR}/state
    ${CMAKE_SOURCE_DIR}/logging
    ${CURSES_INCLUDE_DIR}
    ${Boost_INCLUDE_DIRS}
    ${YACTFR_INCLUDE_DIR}
)
target_link_libraries (
    jacquesctf
    ${CURSES_LIBRARIES}
    Boost::boost
    Boost::program_options
    Boost::filesystem
    ${YACTFR_LIB}
)
target_compile_definitions (
    jacquesctf PRIVATE
    "-DJACQUES_VERSION=\"${PROJECT_VERSION}\""
)
