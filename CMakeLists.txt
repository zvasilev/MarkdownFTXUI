cmake_minimum_required(VERSION 3.25)

# Allow older cmake_minimum_required in fetched dependencies (cmark-gfm)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "")

project(markdown-ui-project LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

# --- FTXUI ---
FetchContent_Declare(
    ftxui
    GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI.git
    GIT_TAG        v6.1.9
    GIT_SHALLOW    TRUE
)

# --- cmark-gfm ---
FetchContent_Declare(
    cmark-gfm
    GIT_REPOSITORY https://github.com/github/cmark-gfm.git
    GIT_TAG        0.29.0.gfm.13
    GIT_SHALLOW    TRUE
)

# Disable cmark-gfm tests and shared libs
set(CMARK_TESTS OFF CACHE BOOL "" FORCE)
set(CMARK_SHARED OFF CACHE BOOL "" FORCE)
set(CMARK_STATIC ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(ftxui cmark-gfm)

option(MARKDOWN_UI_BUILD_TESTS "Build tests" ON)
option(MARKDOWN_UI_BUILD_DEMO  "Build demo"  ON)

add_subdirectory(markdown)

if(MARKDOWN_UI_BUILD_TESTS)
    enable_testing()
    add_subdirectory(markdown/tests)
endif()

if(MARKDOWN_UI_BUILD_DEMO)
    add_subdirectory(demo)
endif()
