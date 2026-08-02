#   Copyright 2026 Abdulkareem Siddiq. All rights reserved.
#   See LICENSE file
#
#   When building from our CI server, the following options must be set:
#   - aarp_BUILD_TESTS=ON
#   - CI_BUILDID must be set to the gitversion
#   The file project-base.json contains configuration elements
# 
cmake_minimum_required(VERSION 3.31)

# Version detection must precede project() declaration
# Figures out ${CURRENT_PROJECT_VERSION} and brings in CPM module
set(CPM_SOURCE_CACHE "$ENV{HOME}/.cache/.cpmcache" CACHE PATH "CPM source cache")
include(pack/CMakeCommonHelpers.cmake)

# Project metadata
# defaults are in the project-base.json file
project(arrp
        VERSION         "${CURRENT_PROJECT_VERSION}"
        LANGUAGES       CXX
        HOMEPAGE_URL    "%%myurl%%")

# Build options
option(${PROJECT_NAME}_BUILD_TESTS "Build tests for ${PROJECT_NAME}" OFF)
# Decides whether to use a std::recursive_mutex for the std::deque inside the resource_pool
# This is enabled for DEBUG builds and disabled for RELEASE builds.
option(${PROJECT_NAME}_USE_RECURSIVE_MUTEX "Use recursive_mutex in pool ${PROJECT_NAME}" OFF)
option(CI_BUILDID "Build ID for CI builds" "0.0.0")

# Library definition
add_library(${PROJECT_NAME} INTERFACE)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

# Include directories
target_include_directories(${PROJECT_NAME}
    INTERFACE
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

# Compile features and options
target_compile_features(${PROJECT_NAME} INTERFACE cxx_std_${CMAKE_CXX_STANDARD})

# Install natvis file for Visual Studio debugger visualization
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/SiddiqSoft.${PROJECT_NAME}.natvis
        DESTINATION ${CMAKE_INSTALL_DATADIR}/${PROJECT_NAME}
        COMPONENT natvis
        OPTIONAL)

# ____________________________________
# Dependencies
# Here we explicitly declare that we depend on the nlohmann_json package and that
# the client will only need to depend on our file where the nlohmann_json will
# automatically included.
cpmaddpackage("gh:siddiqsoft/RunOnEnd#1.4.5")
target_link_libraries(${PROJECT_NAME} INTERFACE RunOnEnd::RunOnEnd)

# Testing configuration
if(${PROJECT_NAME}_BUILD_TESTS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests" AND IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/tests")
    message(STATUS "${PROJECT_NAME}: Building tests (${PROJECT_NAME}_BUILD_TESTS=ON)")
    enable_testing()
    add_subdirectory(tests)
endif()
