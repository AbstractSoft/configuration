# configuration — AGENTS.md

Header-only C++23 configuration library with compile-time reflection. CMake, CLion workflow.

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

- `include/configuration.hpp` — Configuration class (flat key-value lookup with dot notation)
- `include/field_reflection.hpp` — Reflectable base class + Field descriptor for typed objects
- `tests/configuration_test.cpp` — unit tests (synthetic JSON, temp files)

## Clang-tidy

`.clang-tidy` applies only to `src/` files. Checks: cppcoreguidelines-init-variables, llvm-include-order, readability-braces-around-statements, readability-identifier-length (min 2 chars). Default checks are cleared — only listed rules apply.

## Gotchas

- `compile_commands.json` is gitignored — copy from `cmake-build-debug/compile_commands.json` after regenerating CMake.
- The library is header-only — no `src/` directory needed.
- Typed structs must inherit `Reflectable<T>` and define `static constexpr auto fields()` returning a tuple of `Field{...}` descriptors.
- Field names must be string literals (static storage duration) — passing a temporary `std::string` will cause a dangling `std::string_view`.
- The library is read-only — no persistence methods.
