# configuration — AGENTS.md

Header-only C++23 configuration library with compile-time reflection. CMake, CLion workflow.

## Version

v1.0.1 — `main` branch, pushed to GitHub.

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

15 tests covering primitives, nested keys, typed objects, optional access, nested reflectables, arrays, and error handling.

## Structure

- `include/configuration.hpp` — Configuration class (flat key-value lookup with dot notation, private `convert<T>()` helper)
- `include/field_reflection.hpp` — Reflectable CRTP base + Field descriptor for typed objects + `HasFields` concept
- `tests/configuration_test.cpp` — unit tests (synthetic JSON, temp files)
- `CMakeLists.txt` — CMake 3.27+, FetchContent for nlohmann/json v3.11.3 + GoogleTest v1.14.0
- `.clang-tidy` — minimal check set (defaults cleared)

## Clang-tidy

`.clang-tidy` applies only to `include/` files. Checks: `cppcoreguidelines-init-variables`, `llvm-include-order`, `readability-braces-around-statements`, `readability-identifier-length` (min 2 chars for variables, parameters, loop counters, exceptions, bindings). Default checks are cleared — only listed rules apply.

## Coding Conventions

- Internal identifiers (member variables, template parameters, test fixture classes) use `snake_case`.
- Public API names (classes, concepts, type aliases) use `PascalCase`.

## Gotchas

- `compile_commands.json` is gitignored — copy from `cmake-build-debug/compile_commands.json` after regenerating CMake.
- The library is header-only — no `src/` directory needed.
- Typed structs must inherit `Reflectable<T>` and define `static constexpr auto fields()` returning a tuple of `Field{...}` descriptors.
- Field names must be string literals (static storage duration) — passing a temporary `std::string` will cause a dangling `std::string_view`.
- The library is read-only — no persistence methods.
- `std::filesystem::unique_path()` is not implemented in Apple libc++ — use `std::atomic<int>` counter for test directory uniqueness instead.
- `nlohmann::json::find()` does not accept `std::string_view` directly — construct `std::string` for lookup.
- Use unified brace initialization `{}` — avoid parentheses `()`.
