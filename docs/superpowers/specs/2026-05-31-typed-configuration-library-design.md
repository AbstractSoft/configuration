# 2026-05-31 — Typed Configuration Library Design

## Architecture

The library provides a single `Configuration` class template. Each section type inherits from `Section` base class and uses `read<T>()` to fetch values from its JSON sub-object. Lazy loading — the file is read once on first `get<T>()` call, then cached.

Data flow:
1. `Configuration<Cache, Server>` constructor stores path, defers file read
2. First `get<Cache>()` triggers `load()` which parses JSON and populates all sections
3. Subsequent `get<T>()` calls return cached section references

## Components & Interfaces

### Section base class

```cpp
class Section {
protected:
    nlohmann::json const& m_json;

    template<typename T>
    T read(std::string_view key, T default_value) const {
        return m_json.value(key, default_value);
    }
};
```

### Concrete sections

Each section inherits from `Section`, defines fields with defaults, and implements `load()`:

```cpp
struct Cache : Section {
    bool enabled = true;
    std::string path = "./cache.db";
    int64_t default_ttl_seconds = 300;

    static constexpr std::string_view section_name() { return "cache"; }

    void load(nlohmann::json const& section_json) {
        enabled = read(section_json, "enabled", true);
        path = read(section_json, "path", "./cache.db");
        default_ttl_seconds = read(section_json, "default_ttl_seconds", 300LL);
    }
};
```

### Configuration class

```cpp
template<typename... Sections>
class Configuration {
public:
    explicit Configuration(std::string_view config_path = "./config.json");

    template<typename S>
    const S& get() const;

private:
    std::string m_path;
    nlohmann::json m_json;
    std::tuple<Sections...> m_sections;
    mutable bool m_loaded = false;

    void load() const;

    template<typename S>
    void deserialize_section(nlohmann::json const& json);
};
```

- `m_loaded` ensures lazy load happens exactly once
- `deserialize_section<S>(json)` looks up `json[S::section_name()]` and calls `S::load(json_section)`
- Missing section throws `std::runtime_error`

## Data Flow

- **Constructor**: stores path, defers file read
- **First `get<T>()`**: triggers `load()` which reads file into `m_json`, creates all sections, calls `section.load()` on each
- **Subsequent `get<T>()`**: returns cached reference, no file IO

## Error Handling

| Scenario | Exception |
|---|---|
| Missing section in JSON | `std::runtime_error("Missing required section 'X' in configuration.json")` |
| Invalid field value | `std::runtime_error` with section + attribute name |
| File not found / invalid JSON | `std::runtime_error` |

## Testing

Tests use synthetic JSON created in-memory during execution. No external fixtures.

Test scenarios:
- Load sections from JSON, verify all values match
- Missing section throws runtime_error
- Malformed JSON throws runtime_error
- File not found throws runtime_error
- Lazy load: section not loaded before first `get<T>()` call

## File Structure

```
include/configuration.hpp    — Configuration class template + Section base class
tests/
    configuration_test.cpp   — library functionality tests
```

The library is header-only (fully templated). No separate `.cpp` file.
