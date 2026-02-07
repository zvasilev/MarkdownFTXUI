add_library(markdown-ui
    src/editor.cpp
    src/viewer.cpp
    src/parser_cmark.cpp
    src/dom_builder.cpp
    src/highlight.cpp
)

target_include_directories(markdown-ui PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

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

# --- Install targets ---
include(GNUInstallDirs)

install(TARGETS markdown-ui
    EXPORT markdown-ui-targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY include/markdown
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT markdown-ui-targets
    FILE markdown-ui-config.cmake
    NAMESPACE markdown::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/markdown-ui
)
