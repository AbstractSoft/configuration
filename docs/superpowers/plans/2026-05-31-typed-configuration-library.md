# Typed Configuration Library Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a header-only C++23 typed configuration library that loads JSON config files into strongly-typed section structs.

**Architecture:** A single `Configuration<Sections...>` class template wraps nlohmann::json. Each section struct provides `section_name()`, `from_json()`, and `to_json()` static methods. The library deserializes JSON sections into typed structs on load, and serializes them back on save. Single-attribute path-based set/save is supported via dot-separated paths like `"cache.enabled"`.

**Tech Stack:** C++23, CMake 3.27, nlohmann/json (via FetchContent), GoogleTest (via FetchContent), CLion/AppleClang on macOS.

---

### Task 1: Update CMakeLists.txt and create project structure

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
#include <utility>

namespace configuration {

template<typename... Sections>
class Configuration {
public:
    explicit Configuration(std::string_view config_path)
        : m_config_path(std::string(config_path)) {
        load();
    }

    template<typename S>
    const S& get() const {
        return std::get<S>(m_sections);
    }

    void set(std::string_view path, std::string value) {
        auto [section_name, attr_key] = split_path(path);
        auto& section_json = get_section_json(section_name);
        section_json[attr_key] = value;
    }

    void set(std::string_view path, int value) {
        auto [section_name, attr_key] = split_path(path);
        auto& section_json = get_section_json(section_name);
        section_json[attr_key] = value;
    }

    void set(std::string_view path, float value) {
        auto [section_name, attr_key] = split_path(path);
        auto& section_json = get_section_json(section_name);
        section_json[attr_key] = value;
    }

    void set(std::string_view path, bool value) {
        auto [section_name, attr_key] = split_path(path);
        auto& section_json = get_section_json(section_name);
        section_json[attr_key] = value;
    }

    void save() {
        m_json.clear();
        serialize_all();
        write_json();
    }

    void save(std::string_view path) {
        auto [section_name, attr_key] = split_path(path);
        auto& section_json = get_section_json(section_name);
        (void)section_json[attr_key]; // trigger existence check
        write_json();
    }

    void save(std::string_view path, std::string value) {
        set(path, value);
        write_json();
    }

    void save(std::string_view path, int value) {
        set(path, value);
        write_json();
    }

    void save(std::string_view path, float value) {
        set(path, value);
        write_json();
    }

    void save(std::string_view path, bool value) {
        set(path, value);
        write_json();
    }

private:
    std::string m_config_path;
    nlohmann::json m_json;
    std::tuple<Sections...> m_sections;

    static std::pair<std::string, std::string> split_path(std::string_view path) {
        auto dot = path.find('.');
        if (dot == std::string_view::npos) {
            throw std::invalid_argument("Path must contain a section name and attribute key separated by '.', got: " + std::string(path));
        }
        return {std::string(path.substr(0, dot)), std::string(path.substr(dot + 1))};
    }

    nlohmann::json& get_section_json(std::string_view section_name) {
        auto it = m_json.find(section_name);
        if (it == m_json.end()) {
            throw std::invalid_argument("Unknown section '" + std::string(section_name) + "' in path");
        }
        return *it;
    }

    void load() {
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

    template<typename S>
    void serialize_section(const std::tuple<Sections...>& sections, nlohmann::json& json) {
        const auto& section_ref = std::get<S>(sections);
        nlohmann::json section_json = nlohmann::json::object();
        section_ref.to_json(section_json);
        json[S::section_name()] = section_json;
    }

    void serialize_all() {
        int dummy[] = {0, (serialize_section<Sections>(m_sections, m_json), 0)...};
        (void)dummy;
    }

    void write_json() {
        std::ofstream file(m_config_path);
        if (!file.is_open()) {
            throw std::runtime_error("Cannot write to configuration file: " + m_config_path);
        }
        file << m_json.dump(4);
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
    void to_json(nlohmann::json& j) const {
        j["enabled"] = enabled;
        j["path"] = path;
        j["default_ttl_seconds"] = default_ttl_seconds;
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
    void to_json(nlohmann::json& j) const {
        j["port"] = port;
        j["thread_pool_size"] = thread_pool_size;
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

TEST_F(ConfigurationTest, RoundTripSaveAndReload) {
    nlohmann::json j;
    j["cache"] = {{"enabled", true}, {"path", "./cache.db"}, {"default_ttl_seconds", 300}};
    j["server"] = {{"port", 8080}, {"thread_pool_size", 10}};
    write_json(j);

    Configuration<Cache, Server> config(config_path.string());

    // Save back
    config.save();

    // Reload and verify
    Configuration<Cache, Server> config2(config_path.string());

    auto& cache = config2.get<Cache>();
    EXPECT_TRUE(cache.enabled);
    EXPECT_EQ(cache.path, "./cache.db");
    EXPECT_EQ(cache.default_ttl_seconds, 300);

    auto& server = config2.get<Server>();
    EXPECT_EQ(server.port, 8080);
    EXPECT_EQ(server.thread_pool_size, 10);
}

TEST_F(ConfigurationTest, SetSingleAttribute) {
    nlohmann::json j;
    j["cache"] = {{"enabled", true}, {"path", "./cache.db"}, {"default_ttl_seconds", 300}};
    j["server"] = {{"port", 8080}, {"thread_pool_size", 10}};
    write_json(j);

    Configuration<Cache, Server> config(config_path.string());

    config.set("cache.enabled", false);
    config.save("cache.enabled");

    Configuration<Cache, Server> config2(config_path.string());
    EXPECT_FALSE(config2.get<Cache>().enabled);
    // Other values unchanged
    EXPECT_EQ(config2.get<Cache>().path, "./cache.db");
    EXPECT_EQ(config2.get<Server>().port, 8080);
}

TEST_F(ConfigurationTest, SaveSingleAttributeWithDifferentTypes) {
    nlohmann::json j;
    j["cache"] = {{"enabled", true}, {"path", "./cache.db"}, {"default_ttl_seconds", 300}};
    j["server"] = {{"port", 8080}, {"thread_pool_size", 10}};
    write_json(j);

    Configuration<Cache, Server> config(config_path.string());

    // Save int
    config.save("server.port", 9090);
    Configuration<Cache, Server> config2(config_path.string());
    EXPECT_EQ(config2.get<Server>().port, 9090);

    // Save bool
    config2.save("cache.enabled", false);
    Configuration<Cache, Server> config3(config_path.string());
    EXPECT_FALSE(config3.get<Cache>().enabled);

    // Save string
    config3.save("cache.path", "/tmp/new.db");
    Configuration<Cache, Server> config4(config_path.string());
    EXPECT_EQ(config4.get<Cache>().path, "/tmp/new.db");
}

TEST_F(ConfigurationTest, MissingSectionThrows) {
    nlohmann::json j;
    j["cache"] = {{"enabled", true}};
    // server section is missing
    write_json(j);

    EXPECT_THROW(Configuration<Cache, Server>(config_path.string()), std::runtime_error);
}

TEST_F(ConfigurationTest, InvalidPathThrows) {
    nlohmann::json j;
    j["cache"] = {{"enabled", true}};
    j["server"] = {{"port", 8080}};
    write_json(j);

    Configuration<Cache, Server> config(config_path.string());
    EXPECT_THROW(config.set("unknown.key", 123), std::invalid_argument);
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
rm -f library.cpp library.h
rm -rf src/ include/
```

Note: The `include/` directory being removed is the old one with `library.h`. The new `include/configuration.hpp` is created in Step 2.

- [ ] **Step 5: Run CMake configuration**

```bash
rm -rf cmake-build-debug && cmake -B cmake-build-debug -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
```

Expected: CMake configures successfully, fetches nlohmann_json and GoogleTest dependencies.

- [ ] **Step 6: Build the project**

```bash
cmake --build cmake-build-debug
```

Expected: Builds `configuration_tests` executable with no errors.

- [ ] **Step 7: Run tests**

```bash
cd cmake-build-debug && ctest --output-on-failure
```

Expected: All 8 tests pass.

- [ ] **Step 8: Commit**

```bash
git add CMakeLists.txt include/configuration.hpp tests/configuration_test.cpp
git rm -f library.cpp library.h 2>/dev/null || true
git add -A
git commit -m "feat: add typed configuration library with section traits"
```

### Task 2: Update AGENTS.md and regenerate compile_commands.json

**Files:**
- Modify: `AGENTS.md` — update structure and gotchas for new library layout
- Modify: `compile_commands.json` — regenerate from new CMake setup
- Modify: `.gitignore` — ensure new files are tracked, old ones ignored

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
- `samples/1/` — cppreference config example (uses nlohmann/json, not part of library)
- `samples/2/` — email backup config example (uses nlohmann/json, not part of library)
- `samples/CMakeLists.txt` — standalone build for samples (thread_pool project, legacy)

## Clang-tidy

`.clang-tidy` applies only to `src/` files. Checks: cppcoreguidelines-init-variables, llvm-include-order, readability-braces-around-statements, readability-identifier-length (min 3 chars). Default checks are cleared — only listed rules apply.

## Gotchas

- `compile_commands.json` is gitignored — copy from `cmake-build-debug/compile_commands.json` after regenerating CMake.
- `samples/CMakeLists.txt` is a copy of the thread_pool project's CMakeLists.txt, not the samples themselves. Samples 1 and 2 are standalone config implementations using nlohmann/json.
- The library is header-only — no `src/` directory needed.
- Section structs must define `section_name()`, `from_json()`, and `to_json()` — the Configuration class uses these to deserialize/serialize each section.
```

- [ ] **Step 2: Regenerate compile_commands.json**

```bash
rm -rf cmake-build-debug && cmake -B cmake-build-debug -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
cp cmake-build-debug/compile_commands.json .
```

- [ ] **Step 3: Remove samples directory** (as per user request: "Samples will be deleted afterwards")

```bash
rm -rf samples/
```

- [ ] **Step 4: Commit**

```bash
git add AGENTS.md compile_commands.json
git rm -rf samples/
git commit -m "chore: update docs, regenerate compile_commands, remove samples"
```

### Task 3: Final verification

- [ ] **Step 1: Run full test suite**

```bash
cd cmake-build-debug && ctest --output-on-failure -V
```

Expected: All tests pass with verbose output.

- [ ] **Step 2: Verify clean build from scratch**

```bash
rm -rf cmake-build-debug out/
cmake -B cmake-build-debug -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
cmake --build cmake-build-debug
cd cmake-build-debug && ctest --output-on-failure
```

Expected: Full build and all tests pass from clean state.

- [ ] **Step 3: Verify clang-tidy runs on header**

```bash
clang-tidy include/configuration.hpp -- -std=c++23 -I$(pwd)/cmake-build-debug/_deps/nlohmann_json-src/include -quiet
```

Expected: No warnings (or only minor style warnings not blocked by `.clang-tidy` HeaderFilterRegex which targets `src/` — header is outside scope).

- [ ] **Step 4: Final commit**

```bash
git add -A
git commit -m "verify: clean build, tests pass, clang-tidy clean"
```
