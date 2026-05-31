# 2026-05-31 — Typed Configuration Library Design

## Architecture

The library provides a single `Configuration` class template. Each section type is a user-defined struct passed as a template parameter. The class deserializes JSON sections into typed structs. Read-only — no persistence.

Data flow:
1. `Configuration<Cache, Client>` reads `configuration.json` → `nlohmann::json` tree
2. For each section `S`, looks up `json[S::section_name()]` and calls `S::from_json(section, json_section)`

## Components & Interfaces

### Configuration class

```cpp
template<typename... Sections>
class Configuration {
public:
    explicit Configuration(std::string_view config_path);
    explicit Configuration(nlohmann::json json_data);

    template<typename S>
    const S& get() const;

private:
    nlohmann::json m_json;
    std::tuple<Sections...> m_sections;
};
```

### Section struct requirements

Each section type `S` must provide:

```cpp
struct Cache {
    bool enabled = true;
    std::string path = "./cache.db";

    static constexpr std::string_view section_name() { return "cache"; }
    static void from_json(Cache& self, nlohmann::json const& j);
};
```

- `section_name()` — returns the JSON key for this section
- `from_json()` — deserializes a JSON object into the struct, validates values

Custom types within sections can use either nested JSON objects or flat key-value pairs — `from_json` handles the mapping.

## Data Flow

- **Load from file**: JSON file → `nlohmann::json` → per-section `from_json()` calls → typed struct instances
- **Load from memory**: `nlohmann::json` → per-section `from_json()` calls → typed struct instances
- **Get**: Returns const reference to the stored section struct

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
- Constructor from nlohmann::json works correctly

## File Structure

```
include/configuration.hpp    — Configuration class template
tests/
    configuration_test.cpp   — library functionality tests
```

The library is header-only (fully templated). No separate `.cpp` file.
