# Building MarkdownFTXUI

## Prerequisites

| Requirement | Minimum Version | Notes |
|-------------|----------------|-------|
| CMake | 3.25 | Build system generator |
| C++ compiler | C++20 support | MSVC 2022, GCC 12+, Clang 15+ |
| Git | any | For FetchContent dependency downloads |

No other dependencies need to be installed manually. FTXUI and cmark-gfm are fetched automatically during the CMake configure step.

## Dependencies (auto-fetched)

The build system uses CMake's `FetchContent` to download and build two libraries:

- **[FTXUI](https://github.com/ArthurSonzogni/FTXUI) v6.1.9** -- Terminal UI framework providing the DOM-based rendering engine, component system, and screen abstraction.
- **[cmark-gfm](https://github.com/github/cmark-gfm) 0.29.0.gfm.13** -- GitHub's fork of the CommonMark reference parser. Used internally by the library; its headers are never exposed to consumers.

Both are fetched as shallow Git clones to minimize download size.

## Configure and Build

### Standard build (all platforms)

```bash
cmake -B build
cmake --build build
```

### Release build (recommended for performance)

```bash
cmake -B build
cmake --build build --config Release
```

> **MSVC note:** Visual Studio uses a multi-configuration generator. The `--config Release` flag is required to select the build configuration. Without it, MSVC defaults to `Debug`.

### Parallel build

```bash
cmake --build build --config Release -j 8
```

Replace `8` with your CPU core count.

## Running Tests

```bash
ctest --test-dir build -C Release
```

For verbose output on failures:

```bash
ctest --test-dir build -C Release --output-on-failure
```

To run a specific test:

```bash
ctest --test-dir build -C Release -R test_parser
```

All 24 test executables are registered with CTest. See [testing.md](testing.md) for the full list.

## Running the Demo

After building:

```bash
# Windows (MSVC)
build\demo\Release\markdown-demo.exe

# Linux / macOS
./build/demo/markdown-demo
```

The demo presents a menu with three screens. See [demos.md](demos.md) for details.

## Installing

To install the library headers and static archive to a prefix directory:

```bash
cmake --install build --prefix /usr/local
```

This installs:
- Headers to `<prefix>/include/markdown/`
- Static library to `<prefix>/lib/`
- CMake config to `<prefix>/lib/cmake/markdown-ui/`

## Using as a Dependency

### Option A: FetchContent (recommended)

Add this to your project's `CMakeLists.txt`:

```cmake
include(FetchContent)

FetchContent_Declare(
    markdown-ui
    GIT_REPOSITORY https://github.com/<your-org>/MarkdownFTXUI.git
    GIT_TAG        main
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(markdown-ui)

target_link_libraries(your_target PRIVATE markdown-ui)
```

This automatically pulls FTXUI and cmark-gfm as transitive dependencies.

### Option B: find_package (installed)

If you have installed the library (see above):

```cmake
find_package(markdown-ui REQUIRED)
target_link_libraries(your_target PRIVATE markdown::markdown-ui)
```

The exported target `markdown::markdown-ui` carries the correct include directories and links to FTXUI's `screen`, `dom`, and `component` modules.

## MSVC-Specific Notes

When building on Windows with Visual Studio:

1. **Multi-config generator**: Always pass `--config Release` (or `Debug`) to `cmake --build` and `-C Release` to `ctest`.

2. **cmark-gfm static library names**: The static targets are `libcmark-gfm_static` and `libcmark-gfm-extensions_static` (not `libcmark-gfm`). This is handled automatically by the library's CMakeLists.

3. **Static define**: The library sets `CMARK_GFM_STATIC_DEFINE` internally. You do **not** need to set this in your own project.

4. **cmark-gfm and modern CMake**: The root CMakeLists sets `CMAKE_POLICY_VERSION_MINIMUM=3.5` to allow cmark-gfm (which specifies an older cmake_minimum_required) to build with modern CMake versions. This is handled automatically.

## Troubleshooting

**Slow first build**: The first `cmake -B build` downloads FTXUI and cmark-gfm from GitHub. Subsequent builds reuse the cached sources in `build/_deps/`.

**CMake policy warnings**: If you see warnings about CMP0077 or similar, they originate from cmark-gfm's older CMakeLists and are harmless. The `CMAKE_POLICY_VERSION_MINIMUM` setting suppresses most of them.

**Link errors with cmark-gfm**: Ensure you are linking against `markdown-ui` (which handles cmark-gfm internally), not against cmark-gfm directly.
