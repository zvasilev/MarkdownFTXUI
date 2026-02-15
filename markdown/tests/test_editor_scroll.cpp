#include "test_helper.hpp"
#include "markdown/editor.hpp"

#include <ftxui/component/event.hpp>
#include <ftxui/component/mouse.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>

using namespace markdown;

namespace {

struct EditorFixture {
    Editor editor;
    ftxui::Component comp;
    ftxui::Screen screen{
        ftxui::Screen::Create(ftxui::Dimension::Fixed(60),
                              ftxui::Dimension::Fixed(10))};

    EditorFixture() {
        // 50 lines of content
        std::string content;
        for (int i = 1; i <= 50; ++i)
            content += "line " + std::to_string(i) + "\n";
        editor.set_content(content);
        editor.set_cursor(1, 1);
        comp = editor.component();
        ftxui::Render(screen, comp->Render());
    }

    void activate() {
        comp->OnEvent(ftxui::Event::Return);
    }

    ftxui::Event wheel(ftxui::Mouse::Button btn) {
        ftxui::Mouse m;
        m.button = btn;
        m.motion = ftxui::Mouse::Pressed;
        m.x = 5;
        m.y = 5;
        return ftxui::Event::Mouse("", m);
    }
};

} // namespace

int main() {
    // Test 1: PageDown moves cursor down 20 lines
    {
        EditorFixture f;
        f.activate();
        ASSERT_EQ(f.editor.cursor_line(), 1);
        f.comp->OnEvent(ftxui::Event::PageDown);
        ftxui::Render(f.screen, f.comp->Render());
        ASSERT_EQ(f.editor.cursor_line(), 21);
    }

    // Test 2: PageUp moves cursor up 20 lines
    {
        EditorFixture f;
        f.activate();
        f.editor.set_cursor(30, 1);
        f.comp->OnEvent(ftxui::Event::PageUp);
        ftxui::Render(f.screen, f.comp->Render());
        ASSERT_EQ(f.editor.cursor_line(), 10);
    }

    // Test 3: PageDown clamps to last line (51 — trailing \n creates empty line)
    {
        EditorFixture f;
        f.activate();
        f.editor.set_cursor(45, 1);
        f.comp->OnEvent(ftxui::Event::PageDown);
        ftxui::Render(f.screen, f.comp->Render());
        ASSERT_EQ(f.editor.cursor_line(), 51);
    }

    // Test 4: PageUp clamps to first line
    {
        EditorFixture f;
        f.activate();
        f.editor.set_cursor(5, 1);
        f.comp->OnEvent(ftxui::Event::PageUp);
        ftxui::Render(f.screen, f.comp->Render());
        ASSERT_EQ(f.editor.cursor_line(), 1);
    }

    // Test 5: WheelDown moves cursor down 3 lines
    {
        EditorFixture f;
        // Wheel works without activation.
        ASSERT_EQ(f.editor.cursor_line(), 1);
        f.comp->OnEvent(f.wheel(ftxui::Mouse::WheelDown));
        ftxui::Render(f.screen, f.comp->Render());
        ASSERT_EQ(f.editor.cursor_line(), 4);
    }

    // Test 6: WheelUp moves cursor up 3 lines
    {
        EditorFixture f;
        f.editor.set_cursor(10, 1);
        f.comp->OnEvent(f.wheel(ftxui::Mouse::WheelUp));
        ftxui::Render(f.screen, f.comp->Render());
        ASSERT_EQ(f.editor.cursor_line(), 7);
    }

    // Test 7: WheelUp clamps to line 1
    {
        EditorFixture f;
        f.editor.set_cursor(2, 1);
        f.comp->OnEvent(f.wheel(ftxui::Mouse::WheelUp));
        ftxui::Render(f.screen, f.comp->Render());
        ASSERT_EQ(f.editor.cursor_line(), 1);
    }

    // Test 8: WheelDown clamps to last line (51 — trailing \n)
    {
        EditorFixture f;
        f.editor.set_cursor(50, 1);
        f.comp->OnEvent(f.wheel(ftxui::Mouse::WheelDown));
        ftxui::Render(f.screen, f.comp->Render());
        ASSERT_EQ(f.editor.cursor_line(), 51);
    }

    // Test 9: Multiple PageDown + PageUp round-trip
    {
        EditorFixture f;
        f.activate();
        f.comp->OnEvent(ftxui::Event::PageDown); // 1 → 21
        f.comp->OnEvent(ftxui::Event::PageDown); // 21 → 41
        f.comp->OnEvent(ftxui::Event::PageDown); // 41 → 51 (clamped)
        ftxui::Render(f.screen, f.comp->Render());
        ASSERT_EQ(f.editor.cursor_line(), 51);
        f.comp->OnEvent(ftxui::Event::PageUp);   // 51 → 31
        ftxui::Render(f.screen, f.comp->Render());
        ASSERT_EQ(f.editor.cursor_line(), 31);
    }

    // Test 10: move_cursor_lines preserves column
    {
        EditorFixture f;
        f.editor.set_cursor(1, 3); // col 3 on line 1
        f.editor.move_cursor_lines(5);
        ftxui::Render(f.screen, f.comp->Render());
        ASSERT_EQ(f.editor.cursor_line(), 6);
        ASSERT_EQ(f.editor.cursor_col(), 3);
    }

    return 0;
}
