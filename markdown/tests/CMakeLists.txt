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

add_executable(test_external_focusable test_external_focusable.cpp)
target_link_libraries(test_external_focusable PRIVATE markdown-ui)
add_test(NAME test_external_focusable COMMAND test_external_focusable)
