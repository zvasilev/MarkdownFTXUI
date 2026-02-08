add_executable(markdown-demo
    main.cpp
    screen_editor.cpp
    screen_viewer.cpp
    screen_email.cpp
    screen_newsletter.cpp
)
target_link_libraries(markdown-demo PRIVATE markdown-ui)
target_compile_definitions(markdown-demo PRIVATE
    SNIPPET_FILE="${CMAKE_SOURCE_DIR}/markdown/tests/snippets/email1.md")
