#include "test_helper.hpp"
#include "markdown/editor.hpp"

#include <ftxui/screen/screen.hpp>
#include <ftxui/dom/elements.hpp>

using namespace markdown;

int main() {
    // Test 1: set_content / content roundtrip
    {
        Editor editor;
        ASSERT_EQ(editor.content(), "");
        editor.set_content("hello world");
        ASSERT_EQ(editor.content(), "hello world");
    }

    // Test 2: Default cursor position is 0
    {
        Editor editor;
        ASSERT_EQ(editor.cursor_position(), 0);
    }

    // Test 3: set_cursor_position / cursor_position
    {
        Editor editor;
        editor.set_content("hello");
        editor.set_cursor_position(3);
        ASSERT_EQ(editor.cursor_position(), 3);
    }

    // Test 4: set_cursor_position clamping
    {
        Editor editor;
        editor.set_content("hi");
        editor.set_cursor_position(100);
        ASSERT_EQ(editor.cursor_position(), 2); // clamped to content.size()
        editor.set_cursor_position(-5);
        ASSERT_EQ(editor.cursor_position(), 0); // clamped to 0
    }

    // Test 5: set_cursor(1,1) — first char, first line
    {
        Editor editor;
        editor.set_content("hello\nworld");
        editor.set_cursor(1, 1);
        ASSERT_EQ(editor.cursor_position(), 0);
    }

    // Test 6: set_cursor(2,1) — first char, second line
    {
        Editor editor;
        editor.set_content("hello\nworld");
        editor.set_cursor(2, 1);
        ASSERT_EQ(editor.cursor_position(), 6); // "hello\n" = 6 bytes
    }

    // Test 7: set_cursor(1,3) — third column on first line
    {
        Editor editor;
        editor.set_content("hello\nworld");
        editor.set_cursor(1, 3);
        ASSERT_EQ(editor.cursor_position(), 2); // col 3 = byte index 2
    }

    // Test 8: set_cursor with UTF-8 content
    {
        Editor editor;
        editor.set_content("caf\xC3\xA9\ntest"); // café\ntest
        // Column 4 on line 1 = start of é at byte 3
        editor.set_cursor(1, 4);
        ASSERT_EQ(editor.cursor_position(), 3);
        // Column 5 on line 1 = past é, byte 5
        editor.set_cursor(1, 5);
        ASSERT_EQ(editor.cursor_position(), 5);
    }

    // Test 9: cursor_line, cursor_col, total_lines after render
    {
        Editor editor;
        editor.set_content("line one\nline two\nline three");
        editor.set_cursor_position(9); // start of "line two"
        auto comp = editor.component();
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(5));
        ftxui::Render(screen, comp->Render());
        ASSERT_EQ(editor.cursor_line(), 2);
        ASSERT_EQ(editor.cursor_col(), 1);
        ASSERT_EQ(editor.total_lines(), 3);
    }

    // Test 10: cursor_col counts UTF-8 characters after render
    {
        Editor editor;
        editor.set_content("caf\xC3\xA9 ok"); // café ok — 7 bytes
        editor.set_cursor_position(5); // byte 5 = space after é
        auto comp = editor.component();
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(3));
        ftxui::Render(screen, comp->Render());
        ASSERT_EQ(editor.cursor_line(), 1);
        ASSERT_EQ(editor.cursor_col(), 5); // 4 chars (c,a,f,é) + 1
    }

    // Test 11: active() defaults to false
    {
        Editor editor;
        ASSERT_TRUE(!editor.active());
    }

    // Test 12: Cursor at end of single-line content
    {
        Editor editor;
        editor.set_content("abc");
        editor.set_cursor_position(3);
        auto comp = editor.component();
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(3));
        ftxui::Render(screen, comp->Render());
        ASSERT_EQ(editor.cursor_line(), 1);
        ASSERT_EQ(editor.cursor_col(), 4); // past 3 chars = col 4
    }

    // Test 13: Cursor at start of third line
    {
        Editor editor;
        editor.set_content("aa\nbb\ncc");
        editor.set_cursor_position(6); // "aa\nbb\n" = 6 bytes
        auto comp = editor.component();
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(5));
        ftxui::Render(screen, comp->Render());
        ASSERT_EQ(editor.cursor_line(), 3);
        ASSERT_EQ(editor.cursor_col(), 1);
        ASSERT_EQ(editor.total_lines(), 3);
    }

    // Test 14: Empty content — cursor at line 1, col 1
    {
        Editor editor;
        editor.set_content("");
        auto comp = editor.component();
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(3));
        ftxui::Render(screen, comp->Render());
        ASSERT_EQ(editor.cursor_line(), 1);
        ASSERT_EQ(editor.cursor_col(), 1);
        ASSERT_EQ(editor.total_lines(), 1);
    }

    // Test 15: set_theme doesn't crash
    {
        Editor editor;
        editor.set_content("# Hello");
        editor.set_theme(theme_colorful());
        auto comp = editor.component();
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(3));
        ftxui::Render(screen, comp->Render());
        auto output = screen.ToString();
        ASSERT_CONTAINS(output, "Hello");
    }

    return 0;
}
