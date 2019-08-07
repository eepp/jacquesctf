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
    copy-packets-command.cpp
    create-lttng-index-command.cpp
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
    inspect-command/state/data-stream-file-state.cpp
    inspect-command/state/packet-state.cpp
    inspect-command/state/search-parser.cpp
    inspect-command/state/state.cpp
    inspect-command/ui/inspect-command.cpp
    inspect-command/ui/screens/data-stream-files-screen.cpp
    inspect-command/ui/screens/data-types-screen.cpp
    inspect-command/ui/screens/help-screen.cpp
    inspect-command/ui/screens/inspect-screen.cpp
    inspect-command/ui/screens/packets-screen.cpp
    inspect-command/ui/screens/screen.cpp
    inspect-command/ui/screens/trace-info-screen.cpp
    inspect-command/ui/search-controller.cpp
    inspect-command/ui/stylist.cpp
    inspect-command/ui/views/abstract-data-type-details.cpp
    inspect-command/ui/views/data-stream-file-table-view.cpp
    inspect-command/ui/views/data-stream-type-table-view.cpp
    inspect-command/ui/views/data-type-details.cpp
    inspect-command/ui/views/data-type-explorer-view.cpp
    inspect-command/ui/views/event-record-table-view.cpp
    inspect-command/ui/views/event-record-type-table-view.cpp
    inspect-command/ui/views/help-view.cpp
    inspect-command/ui/views/input-view.cpp
    inspect-command/ui/views/packet-checkpoints-build-progress-view.cpp
    inspect-command/ui/views/packet-data-view.cpp
    inspect-command/ui/views/packet-decoding-error-details-view.cpp
    inspect-command/ui/views/packet-index-build-progress-view.cpp
    inspect-command/ui/views/packet-region-info-view.cpp
    inspect-command/ui/views/packet-table-view.cpp
    inspect-command/ui/views/scroll-view.cpp
    inspect-command/ui/views/search-input-view.cpp
    inspect-command/ui/views/simple-message-view.cpp
    inspect-command/ui/views/status-view.cpp
    inspect-command/ui/views/sub-data-type-explorer-view.cpp
    inspect-command/ui/views/table-view.cpp
    inspect-command/ui/views/text-input-view.cpp
    inspect-command/ui/views/trace-info-view.cpp
    inspect-command/ui/views/view.cpp
    jacques.cpp
    list-packets-command.cpp
    print-metadata-text-command.cpp
    utils.cpp
)
target_include_directories (
    jacquesctf PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/inspect-command/ui
    ${CMAKE_CURRENT_SOURCE_DIR}/inspect-command/ui/views
    ${CMAKE_CURRENT_SOURCE_DIR}/inspect-command/ui/screens
    ${CMAKE_CURRENT_SOURCE_DIR}/inspect-command/state
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
