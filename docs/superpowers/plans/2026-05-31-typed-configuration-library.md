# Typed Configuration Library Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a header-only C++23 typed configuration library with lazy loading. Sections inherit from a `Section` base class that provides `read<T>()`. Configuration owns all sections and caches the JSON file.

**Architecture:** `Configuration<Sections...>` reads the JSON file once on first `get<T>()` call (lazy load), then stores all section instances in a tuple. Each section inherits from `Section` which holds a JSON reference and provides `read<T>(key, default)`. Sections define fields with defaults and implement `load(json_subobject)` which calls `read()` for each field.

**Tech Stack:** C++23, CMake 3.27, nlohmann/json (via FetchContent), GoogleTest (via FetchContent), CLion/AppleClang on macOS.

---

### Task 1: Implement the library with Section base class and lazy loading

**Files:**
- Modify: `include/configuration.hpp` — Section base class + Configuration with lazy load
- Modify: `tests/configuration_test.cpp` — tests using Section inheritance pattern

- [ ] **Step 1: Rewrite include/configuration.hpp**

Replace the entire content of `include/configuration.hpp` with:

```cpp
#ifndef CONFIGURATION_HPP
#define CONFIGURATION_HPP

#include <nlohmann/json.hpp>
#include <fstream>
#include <string>
#include <string_view>
#include <tuple>

namespace configuration {

class Section {
protected:
    nlohmann::json const& m_json;

    template<typename T>
    T read(std::string_view key, T default_value) const {
        return m_json.value(key, default_value);
    }
};

template<typename... Sections>
class Configuration {
public:
    explicit Configuration(std::string_view config_path = "./config.json")
        : m_config_path(config_path) {}

    template<typename S>
    const S& get() const {
        if (!m_loaded) {
            load();
        }
        return std::get<S>(m_sections);
    }

private:
    std::string m_config_path;
    nlohmann::json m_json;
    std::tuple<Sections...> m_sections;
    mutable bool m_loaded = false;

    void load() const {
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
        m_loaded = true;
    }

    template<typename S>
    void deserialize_section(nlohmann::json& json) {
        auto& section_ref = std::get<S>(m_sections);
        auto it = json.find(S::section_name());
        if (it == json.end()) {
            throw std::runtime_error("Missing required section '" + std::string(S::section_name()) + "' in configuration.json");
        }
        section_ref.load(*it);
    }

    void deserialize_all() {
        int dummy[] = {0, (deserialize_section<Sections>(m_json), 0)...};
        (void)dummy;
    }
};

} // namespace configuration

#endif // CONFIGURATION_HPP
```

- [ ] **Step 2: Rewrite tests/configuration_test.cpp**

Replace the entire content of `tests/configuration_test.cpp` with:

```cpp
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>
#include "configuration.hpp"

namespace fs = std::filesystem;

// Test section structs inheriting from Section base class
struct Cache : configuration::Section {
    bool enabled = true;
    std::string path = "./cache.db";
    int64_t default_ttl_seconds = 300;

    static constexpr std::string_view section_name() { return "cache"; }

    void load(nlohmann::json const& json) {
        enabled = read(json, "enabled", true);
        path = read(json, "path", "./cache.db");
        default_ttl_seconds = read(json, "default_ttl_seconds", 300LL);
    }
};

struct Server : configuration::Section {
    int port = 8080;
    int thread_pool_size = 10;

    static constexpr std::string_view section_name() { return "server"; }

    void load(nlohmann::json const& json) {
        port = read(json, "port", 8080);
        thread_pool_size = read(json, "thread_pool_size", 10);
    }
};

class ConfigurationTest : public ::testing::Test {
protected:
    void SetUp() {
        test_dir = fs::temp_directory_path() / "config_test_XXXXXX";
        fs::create_directories(test_dir);
        config_path = test_dir / "configuration.json";
    }

    void TearDown() {
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

    configuration::Configuration<Cache, Server> config(config_path.string());

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
    write_json(j);

    EXPECT_THROW((configuration::Configuration<Cache, Server>(config_path.string())), std::runtime_error);
}

TEST_F(ConfigurationTest, MalformedJsonThrows) {
    std::ofstream file(config_path);
    file << "{ invalid json }";
    file.close();

    EXPECT_THROW((configuration::Configuration<Cache>(config_path.string())), std::runtime_error);
}

TEST_F(ConfigurationTest, MissingFileThrows) {
    fs::path nonexistent = test_dir / "nonexistent.json";
    EXPECT_THROW((configuration::Configuration<Cache>(nonexistent.string())), std::runtime_error);
}

TEST_F(ConfigurationTest, LazyLoad) {
    nlohmann::json j;
    j["cache"] = {{"enabled", false}, {"path", "/tmp/test.db"}, {"default_ttl_seconds", 600}};
    j["server"] = {{"port", 9090}, {"thread_pool_size", 4}};
    write_json(j);

    configuration::Configuration<Cache, Server> config(config_path.string());

    // Verify section defaults before load
    auto& cache = config.get<Cache>();
    EXPECT_TRUE(cache.enabled);  // default value, not loaded yet
    EXPECT_EQ(cache.path, "./cache.db");

    // Now load by accessing server
    auto& server = config.get<Server>();
    EXPECT_EQ(server.port, 9090);

    // Cache should now be loaded too
    EXPECT_FALSE(cache.enabled);
    EXPECT_EQ(cache.path, "/tmp/test.db");
}
```

- [ ] **Step 3: Run CMake configuration**

```bash
rm -rf "/Volumes/Macintosh HD2/projects/utilities/configuration/cmake-build-debug"
cmake -B "/Volumes/Macintosh HD2/projects/utilities/configuration/cmake-build-debug" -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
```

Expected: CMake configures successfully, fetches nlohmann_json and GoogleTest dependencies.

- [ ] **Step 4: Build the project**

```bash
cmake --build "/Volumes/Macintosh HD2/projects/utilities/configuration/cmake-build-debug"
```

Expected: Builds `configuration_tests` executable with no errors.

- [ ] **Step 5: Run tests**

```bash
cd "/Volumes/Macintosh HD2/projects/utilities/configuration/cmake-build-debug" && ctest --output-on-failure
```

Expected: All 5 tests pass.

- [ ] **Step 6: Commit**

```bash
cd "/Volumes/Macintosh HD2/projects/utilities/configuration"
git add include/configuration.hpp tests/configuration_test.cpp
git add -A
git commit -m "refactor: add Section base class with lazy loading"
```

### Task 2: Update docs and regenerate compile_commands.json

**Files:**
- Modify: `AGENTS.md` — update structure and gotchas for new Section architecture
- Modify: `compile_commands.json` — regenerate from CMake setup

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

- `include/configuration.hpp` — header-only Configuration class template + Section base class
- `tests/configuration_test.cpp` — unit tests (synthetic JSON, temp files)

## Clang-tidy

`.clang-tidy` applies only to `src/` files. Checks: cppcoreguidelines-init-variables, llvm-include-order, readability-braces-around-statements, readability-identifier-length (min 3 chars). Default checks are cleared — only listed rules apply.

## Gotchas

- `compile_commands.json` is gitignored — copy from `cmake-build-debug/compile_commands.json` after regenerating CMake.
- The library is header-only — no `src/` directory needed.
- Section structs must inherit from `Section`, define `section_name()` and `load(json)` — the Configuration class uses these to deserialize each section.
- The library is read-only — no persistence methods.
```

- [ ] **Step 2: Regenerate compile_commands.json**

```bash
rm -rf "/Volumes/Macintosh HD2/projects/utilities/configuration/cmake-build-debug"
cmake -B "/Volumes/Macintosh HD2/projects/utilities/configuration/cmake-build-debug" -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
cp "/Volumes/Macintosh HD2/projects/utilities/configuration/cmake-build-debug/compile_commands.json" "/Volumes/Macintosh HD2/projects/utilities/configuration/compile_commands.json"
```

- [ ] **Step 3: Commit**

```bash
cd "/Volumes/Macintosh HD2/projects/utilities/configuration"
git add AGENTS.md compile_commands.json
git commit -m "chore: update docs, regenerate compile_commands"
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
