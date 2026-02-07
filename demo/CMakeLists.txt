add_executable(markdown-demo
    main.cpp
    screen_editor.cpp
    screen_viewer.cpp
    screen_email.cpp
)
target_link_libraries(markdown-demo PRIVATE markdown-ui)
