# Copyright (C) 2018 Philippe Proulx <eepp.ca> - All Rights Reserved
#
# Unauthorized copying of this file, via any medium, is strictly
# prohibited. Proprietary and confidential.

cmake_minimum_required (VERSION 3.1.0 FATAL_ERROR)

# project and version
project (jacquesctf VERSION 0.1.0)

# CPack stuff
set (CPACK_SOURCE_GENERATOR TBZ2)
set (CPACK_SOURCE_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${PROJECT_VERSION}")
set (CPACK_SOURCE_IGNORE_FILES "/build/;/.git/;.gitignore;~$;${CPACK_SOURCE_IGNORE_FILES}")
include (CPack)
add_custom_target (dist COMMAND ${CMAKE_MAKE_PROGRAM} package_source)

# configure compiler globally for C++14
set (CMAKE_CXX_EXTENSIONS OFF)
set (CMAKE_CXX_STANDARD 14)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

# Jacques CTF
add_subdirectory (jacquesctf)
