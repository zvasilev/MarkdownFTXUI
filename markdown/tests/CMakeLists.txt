add_executable(test_trivial test_trivial.cpp)
target_link_libraries(test_trivial PRIVATE markdown-ui)
add_test(NAME test_trivial COMMAND test_trivial)

add_executable(test_parser test_parser.cpp)
target_link_libraries(test_parser PRIVATE markdown-ui)
add_test(NAME test_parser COMMAND test_parser)

add_executable(test_dom_builder test_dom_builder.cpp)
target_link_libraries(test_dom_builder PRIVATE markdown-ui)
add_test(NAME test_dom_builder COMMAND test_dom_builder)
