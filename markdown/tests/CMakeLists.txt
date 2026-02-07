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
