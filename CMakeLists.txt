cmake_minimum_required(VERSION 3.27)
project(configuration VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_PROJECT_HOMEPAGE_URL "https://github.com/AbstractSoft/configuration")
set(CMAKE_PROJECT_DESCRIPTION "JSON based configuration library")
set(CMAKE_PROJECT_COPYRIGHT "Copyright (C) 2026 Eduard Ghergu, PhD <eduard.ghergu@professional-programmer.com>")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Sanitizer support
option(ENABLE_SANITIZERS "Enable sanitizers for Debug builds" OFF)

if(ENABLE_SANITIZERS AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Enabling sanitizers for Debug build")
    set(SANITIZER_FLAGS "-fsanitize=address,undefined -fno-omit-frame-pointer")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${SANITIZER_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${SANITIZER_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${SANITIZER_FLAGS}")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${SANITIZER_FLAGS}")
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0 -g")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g")
endif()

# Output directory: out/<Debug|Release>/
set(BUILD_TYPE "${CMAKE_BUILD_TYPE}")
if(CMAKE_CONFIGURATION_TYPES)
    if("${CMAKE_CFG_INTDIR}" STREQUAL ".")
        set(BUILD_TYPE "${CMAKE_BUILD_TYPE}")
    else()
        set(BUILD_TYPE "${CMAKE_CFG_INTDIR}")
    endif()
else()
    if("${BUILD_TYPE}" STREQUAL "")
        set(BUILD_TYPE "Debug")
    endif()
endif()
set(OUTPUT_DIR "${CMAKE_BINARY_DIR}/../out/${BUILD_TYPE}")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${OUTPUT_DIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${OUTPUT_DIR}")

# Ensure output directory exists before linking
add_custom_target(create_output_dir ALL
        COMMAND ${CMAKE_COMMAND} -E make_directory "${OUTPUT_DIR}"
)

add_library(${PROJECT_NAME} INTERFACE)
add_dependencies(${PROJECT_NAME} create_output_dir)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_sources(${PROJECT_NAME} INTERFACE
        include/configuration.hpp
        include/field_reflection.hpp
)

target_include_directories(${PROJECT_NAME}
        INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(${PROJECT_NAME} INTERFACE nlohmann_json::nlohmann_json)

# ── Tests ──────────────────────────────────────────────────────────────────

option(BUILD_TESTS "Build unit tests" OFF)

if(BUILD_TESTS)
    enable_testing()

    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.3
    )
    FetchContent_MakeAvailable(nlohmann_json)

    add_executable(configuration_tests
        tests/configuration_test.cpp
    )

    set(GOOGLETEST_INCLUDE_DIR "${googletest_SOURCE_DIR}/googletest/include" "${googletest_SOURCE_DIR}/googlemock/include")
    set(NLOHMANN_JSON_INCLUDE_DIR "${nlohmann_json_SOURCE_DIR}/include")

    target_include_directories(configuration_tests PRIVATE
        ${GOOGLETEST_INCLUDE_DIR}
        ${NLOHMANN_JSON_INCLUDE_DIR}
    )

    target_link_libraries(configuration_tests PRIVATE
        ${PROJECT_NAME}
        GTest::gtest_main
        GTest::gmock_main
    )

    include(GoogleTest)
    gtest_discover_tests(configuration_tests)
endif()
