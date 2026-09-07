#   SiddiqSoft CMakeLists - Tests
#   Copyright 2026 Abdulkareem Siddiq. All rights reserved.
#   See LICENSE file
#
cmake_minimum_required(VERSION 3.31)

project(arrp_tests
        VERSION         "${CURRENT_PROJECT_VERSION}"
        LANGUAGES       CXX
        DESCRIPTION     "Test suite for arrp")

if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 23)
endif()

# Fetch GoogleTest and nlohmann_json via CPM
if(NOT TARGET GTest::gtest_main)
    CPMAddPackage("gh:google/googletest#v1.17.0")
endif()

if(NOT TARGET nlohmann_json::nlohmann_json)
    CPMAddPackage("gh:nlohmann/json#v3.12.0")
endif()

file(GLOB_RECURSE TEST_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

add_executable(${PROJECT_NAME} ${TEST_SOURCES})
target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_${CMAKE_CXX_STANDARD})

# MSVC requires /std:c++latest for C++23 features
target_compile_options(${PROJECT_NAME}
    PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/std:c++latest>
)

# Debug and testing mode definitions
target_compile_definitions(${PROJECT_NAME}
    PRIVATE
        $<$<CONFIG:Debug>:DEBUG=1 _DEBUG=1>
        arrp_TESTING_MODE=1
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
        GTest::gtest_main
        nlohmann_json::nlohmann_json
        arrp::arrp
)

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME}
    TEST_PREFIX "${PROJECT_NAME}."
    OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/results"
    OUTPUT_PREFIX "${CMAKE_BUILD_TYPE}_"
    OUTPUT_SUFFIX ".xml"
)
