add_library(markdown-ui
    src/editor.cpp
    src/viewer.cpp
    src/parser_cmark.cpp
    src/dom_builder.cpp
    src/highlight.cpp
)

target_include_directories(markdown-ui PUBLIC include)

target_compile_definitions(markdown-ui PRIVATE CMARK_GFM_STATIC_DEFINE)

target_include_directories(markdown-ui PRIVATE
    ${cmark-gfm_SOURCE_DIR}/src
    ${cmark-gfm_BINARY_DIR}/src
)

target_link_libraries(markdown-ui
    PUBLIC
        ftxui::screen
        ftxui::dom
        ftxui::component
    PRIVATE
        libcmark-gfm_static
        libcmark-gfm-extensions_static
)

add_subdirectory(tests)
