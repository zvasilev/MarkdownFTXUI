add_executable(test_trivial test_trivial.cpp)
target_link_libraries(test_trivial PRIVATE markdown-ui)
add_test(NAME test_trivial COMMAND test_trivial)

add_executable(test_parser test_parser.cpp)
target_link_libraries(test_parser PRIVATE markdown-ui)
add_test(NAME test_parser COMMAND test_parser)

add_executable(test_dom_builder test_dom_builder.cpp)
target_link_libraries(test_dom_builder PRIVATE markdown-ui)
add_test(NAME test_dom_builder COMMAND test_dom_builder)

add_executable(test_headings test_headings.cpp)
target_link_libraries(test_headings PRIVATE markdown-ui)
add_test(NAME test_headings COMMAND test_headings)

add_executable(test_bold test_bold.cpp)
target_link_libraries(test_bold PRIVATE markdown-ui)
add_test(NAME test_bold COMMAND test_bold)

add_executable(test_italic test_italic.cpp)
target_link_libraries(test_italic PRIVATE markdown-ui)
add_test(NAME test_italic COMMAND test_italic)

add_executable(test_bold_italic test_bold_italic.cpp)
target_link_libraries(test_bold_italic PRIVATE markdown-ui)
add_test(NAME test_bold_italic COMMAND test_bold_italic)

add_executable(test_links test_links.cpp)
target_link_libraries(test_links PRIVATE markdown-ui)
add_test(NAME test_links COMMAND test_links)

add_executable(test_lists test_lists.cpp)
target_link_libraries(test_lists PRIVATE markdown-ui)
add_test(NAME test_lists COMMAND test_lists)

add_executable(test_code test_code.cpp)
target_link_libraries(test_code PRIVATE markdown-ui)
add_test(NAME test_code COMMAND test_code)

add_executable(test_quotes test_quotes.cpp)
target_link_libraries(test_quotes PRIVATE markdown-ui)
add_test(NAME test_quotes COMMAND test_quotes)

add_executable(test_highlight test_highlight.cpp)
target_link_libraries(test_highlight PRIVATE markdown-ui)
add_test(NAME test_highlight COMMAND test_highlight)

add_executable(test_mixed test_mixed.cpp)
target_link_libraries(test_mixed PRIVATE markdown-ui)
add_test(NAME test_mixed COMMAND test_mixed)

add_executable(test_edge_cases test_edge_cases.cpp)
target_link_libraries(test_edge_cases PRIVATE markdown-ui)
add_test(NAME test_edge_cases COMMAND test_edge_cases)

add_executable(test_unicode test_unicode.cpp)
target_link_libraries(test_unicode PRIVATE markdown-ui)
add_test(NAME test_unicode COMMAND test_unicode)

add_executable(test_text_utils test_text_utils.cpp)
target_link_libraries(test_text_utils PRIVATE markdown-ui)
add_test(NAME test_text_utils COMMAND test_text_utils)

add_executable(test_highlight_cursor test_highlight_cursor.cpp)
target_link_libraries(test_highlight_cursor PRIVATE markdown-ui)
add_test(NAME test_highlight_cursor COMMAND test_highlight_cursor)

add_executable(test_editor test_editor.cpp)
target_link_libraries(test_editor PRIVATE markdown-ui)
add_test(NAME test_editor COMMAND test_editor)

add_executable(test_viewer test_viewer.cpp)
target_link_libraries(test_viewer PRIVATE markdown-ui)
add_test(NAME test_viewer COMMAND test_viewer)

add_executable(test_theme test_theme.cpp)
target_link_libraries(test_theme PRIVATE markdown-ui)
add_test(NAME test_theme COMMAND test_theme)

add_executable(test_dom_focus test_dom_focus.cpp)
target_link_libraries(test_dom_focus PRIVATE markdown-ui)
add_test(NAME test_dom_focus COMMAND test_dom_focus)


add_executable(test_scroll_frame test_scroll_frame.cpp)
target_link_libraries(test_scroll_frame PRIVATE markdown-ui)
add_test(NAME test_scroll_frame COMMAND test_scroll_frame)

add_executable(test_viewer_scroll test_viewer_scroll.cpp)
target_link_libraries(test_viewer_scroll PRIVATE markdown-ui)
add_test(NAME test_viewer_scroll COMMAND test_viewer_scroll)

add_executable(test_editor_scroll test_editor_scroll.cpp)
target_link_libraries(test_editor_scroll PRIVATE markdown-ui)
add_test(NAME test_editor_scroll COMMAND test_editor_scroll)

add_executable(test_snippets test_snippets.cpp)
target_link_libraries(test_snippets PRIVATE markdown-ui)
target_compile_definitions(test_snippets PRIVATE
    SNIPPETS_DIR="${CMAKE_CURRENT_SOURCE_DIR}/snippets")
add_test(NAME test_snippets COMMAND test_snippets)

add_executable(test_stress test_stress.cpp)
target_link_libraries(test_stress PRIVATE markdown-ui)
add_test(NAME test_stress COMMAND test_stress)

add_executable(test_perf_editor test_perf_editor.cpp)
target_link_libraries(test_perf_editor PRIVATE markdown-ui)
add_test(NAME test_perf_editor COMMAND test_perf_editor)

add_executable(test_perf_viewer test_perf_viewer.cpp)
target_link_libraries(test_perf_viewer PRIVATE markdown-ui)
add_test(NAME test_perf_viewer COMMAND test_perf_viewer)

add_executable(test_perf_parse_build test_perf_parse_build.cpp)
target_link_libraries(test_perf_parse_build PRIVATE markdown-ui)
add_test(NAME test_perf_parse_build COMMAND test_perf_parse_build)

add_executable(test_perf_unicode test_perf_unicode.cpp)
target_link_libraries(test_perf_unicode PRIVATE markdown-ui)
add_test(NAME test_perf_unicode COMMAND test_perf_unicode)

add_executable(test_perf_scroll test_perf_scroll.cpp)
target_link_libraries(test_perf_scroll PRIVATE markdown-ui)
add_test(NAME test_perf_scroll COMMAND test_perf_scroll)

add_executable(test_mega_paragraph test_mega_paragraph.cpp)
target_link_libraries(test_mega_paragraph PRIVATE markdown-ui)
add_test(NAME test_mega_paragraph COMMAND test_mega_paragraph)

add_executable(test_tab_exit test_tab_exit.cpp)
target_link_libraries(test_tab_exit PRIVATE markdown-ui)
add_test(NAME test_tab_exit COMMAND test_tab_exit)

add_executable(test_image test_image.cpp)
target_link_libraries(test_image PRIVATE markdown-ui)
add_test(NAME test_image COMMAND test_image)

add_executable(test_thematic_break test_thematic_break.cpp)
target_link_libraries(test_thematic_break PRIVATE markdown-ui)
add_test(NAME test_thematic_break COMMAND test_thematic_break)

add_executable(test_hard_break test_hard_break.cpp)
target_link_libraries(test_hard_break PRIVATE markdown-ui)
add_test(NAME test_hard_break COMMAND test_hard_break)

add_executable(test_mouse_click test_mouse_click.cpp)
target_link_libraries(test_mouse_click PRIVATE markdown-ui)
add_test(NAME test_mouse_click COMMAND test_mouse_click)
