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
