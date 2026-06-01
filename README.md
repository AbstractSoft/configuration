# configuration

A header-only C++23 configuration library with compile-time reflection.

## Features

- **Lazy loading** — JSON file parsed on first access, then cached in memory.
- **Dot-separated keys** — access nested JSON values with `config.get<int>("server.port")`.
- **Typed objects via reflection** — define structs with `Reflectable<T>` and `fields()` to automatically map JSON to C++ types.
- **Partial JSON support** — missing fields keep their C++ default values instead of throwing.
- **Three query APIs** — `get<T>()` with default, `try_get<T>()` returning `std::optional<T>`, and `has()` to check key existence.
- **Rich error messages** — field-level errors include the field name for easy debugging.
- **Header-only** — no build step required, just include the headers.

## Requirements

- C++23 compiler (Apple Clang 15+, GCC 12+, Clang 15+)
- [nlohmann/json](https://github.com/nlohmann/json) v3.11+ (fetched automatically via CMake FetchContent)

## Usage

### Primitive types

```cpp
#include "configuration.hpp"

configuration::Configuration config("config.json");

bool enabled = config.get<bool>("enabled", false);
int port     = config.get<int>("port", 8080);
double timeout = config.get<double>("timeout");

// Optional access
if (auto path = config.try_get<std::string>("cache.path")) {
    std::cout << *path << '\n';
}

// Key existence check
if (config.has("server.host")) { ... }
```

### Typed structs with reflection

```cpp
#include "field_reflection.hpp"

struct Server : Reflectable<Server>
{
    std::string host = "127.0.0.1";
    int port = 8080;

    static constexpr auto fields()
    {
        return std::tuple{
            Field{"host", &Server::host},
            Field{"port", &Server::port}
        };
    }
};

// JSON: { "server": { "host": "0.0.0.0" } }
auto server = config.get<Server>("server");
// server.host == "0.0.0.0", server.port == 8080 (C++ default preserved)
```

### Nested reflectable types

```cpp
struct Application : Reflectable<Application>
{
    Server server;
    Cache cache;

    static constexpr auto fields()
    {
        return std::tuple{
            Field{"server", &Application::server},
            Field{"cache", &Application::cache}
        };
    }
};

auto app = config.get<Application>("app");
```

## JSON format

```json
{
    "enabled": true,
    "port": 8080,
    "server": {
        "host": "0.0.0.0",
        "port": 3000
    },
    "cache": {
        "enabled": false,
        "path": "/var/cache/app"
    }
}
```

## Build

### CMake

```bash
cmake -B cmake-build-debug -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
cmake --build cmake-build-debug
cd cmake-build-debug && ctest --output-on-failure
```

### Sanitizers

```bash
cmake -B cmake-build-debug -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON -DENABLE_SANITIZERS=ON
cmake --build cmake-build-debug
```

## API Reference

| Method | Description |
|---|---|
| `get<T>(key, default_value)` | Returns value at key, or `default_value` if absent. |
| `try_get<T>(key)` | Returns `std::optional<T>` — `nullopt` if key is absent. |
| `has(key)` | Returns `true` if key exists (leaf or intermediate node). |

All methods throw `std::runtime_error` if the key exists but cannot be converted to type `T`.

## License

Copyright (C) 2026 Eduard Ghergu, PhD
