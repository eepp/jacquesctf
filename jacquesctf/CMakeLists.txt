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

# threading
find_package (Threads REQUIRED)

# Jacques CTF program
add_executable (
    jacquesctf
    config.cpp
    data/content-packet-region.cpp
    data/data-size.cpp
    data/data-stream-file.cpp
    data/duration.cpp
    data/error-packet-region.cpp
    data/event-record.cpp
    data/memory-mapped-file.cpp
    data/metadata.cpp
    data/packet-checkpoints-build-listener.cpp
    data/packet-checkpoints.cpp
    data/packet-index-entry.cpp
    data/packet-region-visitor.cpp
    data/packet-region.cpp
    data/packet-segment.cpp
    data/packet.cpp
    data/padding-packet-region.cpp
    data/scope.cpp
    data/timestamp.cpp
    data/trace.cpp
    jacques.cpp
    list-packets.cpp
    print-metadata-text.cpp
    state/data-stream-file-state.cpp
    state/packet-state.cpp
    state/search-parser.cpp
    state/state.cpp
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
    ui/views/data-stream-file-table-view.cpp
    ui/views/data-stream-type-table-view.cpp
    ui/views/data-type-details.cpp
    ui/views/data-type-explorer-view.cpp
    ui/views/event-record-table-view.cpp
    ui/views/event-record-type-table-view.cpp
    ui/views/help-view.cpp
    ui/views/input-view.cpp
    ui/views/packet-checkpoints-build-progress-view.cpp
    ui/views/packet-data-view.cpp
    ui/views/packet-decoding-error-details-view.cpp
    ui/views/packet-index-build-progress-view.cpp
    ui/views/packet-region-info-view.cpp
    ui/views/packet-table-view.cpp
    ui/views/scroll-view.cpp
    ui/views/search-input-view.cpp
    ui/views/simple-message-view.cpp
    ui/views/status-view.cpp
    ui/views/sub-data-type-explorer-view.cpp
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
    ${CMAKE_CURRENT_SOURCE_DIR}/data
    ${CMAKE_SOURCE_DIR}/logging
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
