# Typed Configuration Library Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a header-only C++23 typed configuration library that loads JSON config files into strongly-typed section structs. Read-only — no persistence.

**Architecture:** A single `Configuration<Sections...>` class template wraps nlohmann::json. Each section struct provides `section_name()` and `from_json()` methods. The library deserializes JSON sections into typed structs on load. Supports construction from file path or from an in-memory `nlohmann::json` object.

**Tech Stack:** C++23, CMake 3.27, nlohmann/json (via FetchContent), GoogleTest (via FetchContent), CLion/AppleClang on macOS.

---

### Task 1: Create library and tests

**Files:**
- Modify: `CMakeLists.txt` — replace existing content with configuration library build + test support
- Create: `include/configuration.hpp` — header-only Configuration class
- Create: `tests/configuration_test.cpp` — unit tests for library functionality

- [ ] **Step 1: Update CMakeLists.txt**

Replace the entire content of `CMakeLists.txt` with:

```cmake
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

    target_link_libraries(configuration_tests PRIVATE
        ${PROJECT_NAME}
        GTest::gtest_main
        GTest::gmock_main
    )

    include(GoogleTest)
    gtest_discover_tests(configuration_tests)
endif()
```

- [ ] **Step 2: Create include/configuration.hpp**

Create the header-only Configuration class:

```cpp
#ifndef CONFIGURATION_HPP
#define CONFIGURATION_HPP

#include <nlohmann/json.hpp>
#include <fstream>
#include <string>
#include <string_view>
#include <tuple>

namespace configuration {

template<typename... Sections>
class Configuration {
public:
    explicit Configuration(std::string_view config_path)
        : m_config_path(std::string(config_path)) {
        load_from_file();
    }

    explicit Configuration(nlohmann::json json_data)
        : m_json(std::move(json_data)) {
        deserialize_all();
    }

    template<typename S>
    const S& get() const {
        return std::get<S>(m_sections);
    }

private:
    std::string m_config_path;
    nlohmann::json m_json;
    std::tuple<Sections...> m_sections;

    void load_from_file() {
        std::ifstream file(m_config_path);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot open configuration file: " + m_config_path);
        }
        try {
            m_json = nlohmann::json::parse(file);
        } catch (const nlohmann::json::parse_error& e) {
            throw std::runtime_error("JSON parse error in '" + m_config_path + "': " + e.what());
        }
        deserialize_all();
    }

    template<typename S>
    void deserialize_section(nlohmann::json& json, std::tuple<Sections...>& sections) {
        auto& section_ref = std::get<S>(sections);
        auto it = json.find(S::section_name());
        if (it == json.end()) {
            throw std::runtime_error("Missing required section '" + std::string(S::section_name()) + "' in configuration.json");
        }
        S::from_json(section_ref, *it);
    }

    void deserialize_all() {
        int dummy[] = {0, (deserialize_section<Sections>(m_json, m_sections), 0)...};
        (void)dummy;
    }
};

} // namespace configuration

#endif // CONFIGURATION_HPP
```

- [ ] **Step 3: Create tests/configuration_test.cpp**

Create the test file:

```cpp
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include "configuration.hpp"

namespace fs = std::filesystem;

// Test section structs
struct Cache {
    bool enabled = true;
    std::string path = "./cache.db";
    int64_t default_ttl_seconds = 300;

    static constexpr std::string_view section_name() { return "cache"; }
    static void from_json(Cache& self, nlohmann::json const& j) {
        self.enabled = j.value("enabled", true);
        self.path = j.value("path", "./cache.db");
        self.default_ttl_seconds = j.value("default_ttl_seconds", 300LL);
    }
};

struct Server {
    int port = 8080;
    int thread_pool_size = 10;

    static constexpr std::string_view section_name() { return "server"; }
    static void from_json(Server& self, nlohmann::json const& j) {
        self.port = j.value("port", 8080);
        self.thread_pool_size = j.value("thread_pool_size", 10);
    }
};

class ConfigurationTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir = fs::temp_directory_path() / "config_test_XXXXXX";
        fs::create_directories(test_dir);
        config_path = test_dir / "configuration.json";
    }

    void TearDown() override {
        if (fs::exists(test_dir)) {
            fs::remove_all(test_dir);
        }
    }

    fs::path test_dir;
    fs::path config_path;

    void write_json(nlohmann::json const& j) {
        std::ofstream file(config_path);
        file << j.dump(4);
    }
};

TEST_F(ConfigurationTest, LoadsSectionsFromJson) {
    nlohmann::json j;
    j["cache"] = {{"enabled", false}, {"path", "/tmp/test.db"}, {"default_ttl_seconds", 600}};
    j["server"] = {{"port", 9090}, {"thread_pool_size", 4}};
    write_json(j);

    Configuration<Cache, Server> config(config_path.string());

    auto& cache = config.get<Cache>();
    EXPECT_FALSE(cache.enabled);
    EXPECT_EQ(cache.path, "/tmp/test.db");
    EXPECT_EQ(cache.default_ttl_seconds, 600);

    auto& server = config.get<Server>();
    EXPECT_EQ(server.port, 9090);
    EXPECT_EQ(server.thread_pool_size, 4);
}

TEST_F(ConfigurationTest, LoadsFromMemoryJson) {
    nlohmann::json j;
    j["cache"] = {{"enabled", false}, {"path", "/tmp/test.db"}, {"default_ttl_seconds", 600}};
    j["server"] = {{"port", 9090}, {"thread_pool_size", 4}};

    Configuration<Cache, Server> config(std::move(j));

    auto& cache = config.get<Cache>();
    EXPECT_FALSE(cache.enabled);
    EXPECT_EQ(cache.path, "/tmp/test.db");
    EXPECT_EQ(cache.default_ttl_seconds, 600);

    auto& server = config.get<Server>();
    EXPECT_EQ(server.port, 9090);
    EXPECT_EQ(server.thread_pool_size, 4);
}

TEST_F(ConfigurationTest, MissingSectionThrows) {
    nlohmann::json j;
    j["cache"] = {{"enabled", true}};
    // server section is missing
    write_json(j);

    EXPECT_THROW(Configuration<Cache, Server>(config_path.string()), std::runtime_error);
}

TEST_F(ConfigurationTest, MalformedJsonThrows) {
    std::ofstream file(config_path);
    file << "{ invalid json }";
    file.close();

    EXPECT_THROW(Configuration<Cache>(config_path.string()), std::runtime_error);
}

TEST_F(ConfigurationTest, MissingFileThrows) {
    fs::path nonexistent = test_dir / "nonexistent.json";
    EXPECT_THROW(Configuration<Cache>(nonexistent.string()), std::runtime_error);
}
```

- [ ] **Step 4: Remove old placeholder files**

Delete the old library placeholder files that are no longer part of the project:

```bash
rm -f "/Volumes/Macintosh HD2/projects/utilities/configuration/library.cpp"
rm -f "/Volumes/Macintosh HD2/projects/utilities/configuration/library.h"
rm -rf "/Volumes/Macintosh HD2/projects/utilities/configuration/src/"
```

- [ ] **Step 5: Run CMake configuration**

```bash
rm -rf "/Volumes/Macintosh HD2/projects/utilities/configuration/cmake-build-debug"
cmake -B "/Volumes/Macintosh HD2/projects/utilities/configuration/cmake-build-debug" -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
```

Expected: CMake configures successfully, fetches nlohmann_json and GoogleTest dependencies.

- [ ] **Step 6: Build the project**

```bash
cmake --build "/Volumes/Macintosh HD2/projects/utilities/configuration/cmake-build-debug"
```

Expected: Builds `configuration_tests` executable with no errors.

- [ ] **Step 7: Run tests**

```bash
cd "/Volumes/Macintosh HD2/projects/utilities/configuration/cmake-build-debug" && ctest --output-on-failure
```

Expected: All 5 tests pass.

- [ ] **Step 8: Commit**

```bash
cd "/Volumes/Macintosh HD2/projects/utilities/configuration"
git add CMakeLists.txt include/configuration.hpp tests/configuration_test.cpp
git rm -f library.cpp library.h 2>/dev/null || true
git add -A
git commit -m "feat: add typed configuration library with section traits"
```

### Task 2: Update docs and clean up

**Files:**
- Modify: `AGENTS.md` — update structure and gotchas for new library layout
- Modify: `compile_commands.json` — regenerate from new CMake setup
- Remove: `samples/` directory

- [ ] **Step 1: Update AGENTS.md**

Replace the entire content of `AGENTS.md` with:

```markdown
# configuration — AGENTS.md

JSON-based C++ configuration library. C++23, CMake, CLion workflow.

## Build

```
cmake -B cmake-build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build cmake-build-debug
```

Output lands in `out/Debug/` or `out/Release/`.

## Sanitizers

Pass `-DENABLE_SANITIZERS=ON` to enable AddressSanitizer + UndefinedBehaviorSanitizer (Debug only):

```
cmake -B cmake-build-debug -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON
cmake --build cmake-build-debug
```

## Tests

Build with tests enabled:

```
cmake -B cmake-build-debug -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
cmake --build cmake-build-debug
cd cmake-build-debug && ctest --output-on-failure
```

## Structure

- `include/configuration.hpp` — header-only Configuration class template
- `tests/configuration_test.cpp` — unit tests (synthetic JSON, temp files)

## Clang-tidy

`.clang-tidy` applies only to `src/` files. Checks: cppcoreguidelines-init-variables, llvm-include-order, readability-braces-around-statements, readability-identifier-length (min 3 chars). Default checks are cleared — only listed rules apply.

## Gotchas

- `compile_commands.json` is gitignored — copy from `cmake-build-debug/compile_commands.json` after regenerating CMake.
- The library is header-only — no `src/` directory needed.
- Section structs must define `section_name()` and `from_json()` — the Configuration class uses these to deserialize each section.
- The library is read-only — no persistence methods.
```

- [ ] **Step 2: Regenerate compile_commands.json**

```bash
rm -rf "/Volumes/Macintosh HD2/projects/utilities/configuration/cmake-build-debug"
cmake -B "/Volumes/Macintosh HD2/projects/utilities/configuration/cmake-build-debug" -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
cp "/Volumes/Macintosh HD2/projects/utilities/configuration/cmake-build-debug/compile_commands.json" "/Volumes/Macintosh HD2/projects/utilities/configuration/compile_commands.json"
```

- [ ] **Step 3: Remove samples directory**

```bash
rm -rf "/Volumes/Macintosh HD2/projects/utilities/configuration/samples/"
```

- [ ] **Step 4: Commit**

```bash
cd "/Volumes/Macintosh HD2/projects/utilities/configuration"
git add AGENTS.md compile_commands.json
git rm -rf samples/
git commit -m "chore: update docs, regenerate compile_commands, remove samples"
```

### Task 3: Final verification

- [ ] **Step 1: Run full test suite**

```bash
cd "/Volumes/Macintosh HD2/projects/utilities/configuration/cmake-build-debug" && ctest --output-on-failure -V
```

Expected: All tests pass with verbose output.

- [ ] **Step 2: Verify clean build from scratch**

```bash
rm -rf "/Volumes/Macintosh HD2/projects/utilities/configuration/cmake-build-debug" "/Volumes/Macintosh HD2/projects/utilities/configuration/out/"
cmake -B "/Volumes/Macintosh HD2/projects/utilities/configuration/cmake-build-debug" -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
cmake --build "/Volumes/Macintosh HD2/projects/utilities/configuration/cmake-build-debug"
cd "/Volumes/Macintosh HD2/projects/utilities/configuration/cmake-build-debug" && ctest --output-on-failure
```

Expected: Full build and all tests pass from clean state.

- [ ] **Step 3: Final commit**

```bash
cd "/Volumes/Macintosh HD2/projects/utilities/configuration"
git add -A
git commit -m "verify: clean build, tests pass"
```
