# 2026-05-31 — Typed Configuration Library Design

## Architecture

The library provides a single `Configuration` class template. Each section type is a user-defined struct passed as a template parameter. The class deserializes JSON sections into typed structs and serializes them back on save.

Data flow:
1. `Configuration<Cache, Client>` reads `configuration.json` → `nlohmann::json` tree
2. For each section `S`, looks up `json[S::section_name()]` and calls `S::from_json(section, json_section)`
3. `save()` calls `S::to_json()` on each section, writes full tree to disk
4. `save("cache.enabled", true)` splits path on `.`, updates the JSON sub-object in-place, writes to disk

## Components & Interfaces

### Configuration class

```cpp
template<typename... Sections>
class Configuration {
public:
    explicit Configuration(std::string_view config_path);

    template<typename S>
    const S& get() const;

    void set(std::string_view path, std::string value);
    void set(std::string_view path, int value);
    void set(std::string_view path, float value);
    void set(std::string_view path, bool value);

    void save();
    void save(std::string_view path);
    void save(std::string_view path, std::string value);
    void save(std::string_view path, int value);
    void save(std::string_view path, float value);
    void save(std::string_view path, bool value);

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
    void to_json(nlohmann::json& j) const;
};
```

- `section_name()` — returns the JSON key for this section
- `from_json()` — deserializes a JSON object into the struct, validates values
- `to_json()` — serializes the struct back to JSON

Custom types within sections can use either nested JSON objects or flat key-value pairs — `from_json`/`to_json` handle the mapping.

## Data Flow

- **Load**: JSON file → `nlohmann::json` → per-section `from_json()` calls → typed struct instances
- **Get**: Returns const reference to the stored section struct
- **Set**: Splits path on `.`, navigates to the section's JSON sub-object, updates the leaf key
- **Save (full)**: Per-section `to_json()` → write full JSON tree to file
- **Save (single)**: Update JSON in-place → write full JSON tree to file

## Error Handling

| Scenario | Exception |
|---|---|
| Missing section in JSON | `std::runtime_error("Missing required section 'X' in configuration.json")` |
| Invalid field value | `std::runtime_error` with section + attribute name |
| File not found / invalid JSON | `std::runtime_error` |
| Unknown path in set/save | `std::invalid_argument` |

## Testing

Tests use synthetic JSON created in-memory during execution. No external fixtures.

Test scenarios:
- Round-trip: load → get → save → reload produces identical values
- Single-attribute set/save: modify one key, verify only that key changed
- Path resolution: nested paths like `cache.ttl_policy.max_size` traverse sub-objects correctly
- Error cases: missing section, nonexistent path, corrupt JSON

## File Structure

```
include/configuration/
    configuration.hpp    — Configuration class template + Section trait
tests/
    configuration_test.cpp  — library functionality tests
```

The library is header-only (fully templated). No separate `.cpp` file.
