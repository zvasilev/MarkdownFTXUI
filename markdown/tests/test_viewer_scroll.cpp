#include "test_helper.hpp"
#include "markdown/viewer.hpp"
#include "markdown/parser.hpp"

#include <cmath>

#include <ftxui/component/event.hpp>
#include <ftxui/component/mouse.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>

using namespace markdown;

namespace {

// Helper: create a viewer with long content and render once.
struct ViewerFixture {
    Viewer viewer{make_cmark_parser()};
    ftxui::Component comp;
    ftxui::Screen screen{
        ftxui::Screen::Create(ftxui::Dimension::Fixed(60),
                              ftxui::Dimension::Fixed(10))};

    ViewerFixture() {
        // Long content so scroll ratio is meaningful.
        std::string content;
        for (int i = 0; i < 50; ++i)
            content += "Line " + std::to_string(i) + "\n\n";
        viewer.set_content(content);
        comp = viewer.component();
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

bool approx(float a, float b, float eps = 0.001f) {
    return std::fabs(a - b) < eps;
}

} // namespace

int main() {
    // Test 1: PageDown increases scroll by 0.3
    {
        ViewerFixture f;
        f.activate();
        ASSERT_TRUE(approx(f.viewer.scroll(), 0.0f));
        f.comp->OnEvent(ftxui::Event::PageDown);
        ASSERT_TRUE(approx(f.viewer.scroll(), 0.3f));
    }

    // Test 2: PageUp decreases scroll by 0.3
    {
        ViewerFixture f;
        f.activate();
        f.viewer.set_scroll(0.5f);
        f.comp->OnEvent(ftxui::Event::PageUp);
        ASSERT_TRUE(approx(f.viewer.scroll(), 0.2f));
    }

    // Test 3: PageDown clamps to 1.0
    {
        ViewerFixture f;
        f.activate();
        f.viewer.set_scroll(0.9f);
        f.comp->OnEvent(ftxui::Event::PageDown);
        ASSERT_TRUE(approx(f.viewer.scroll(), 1.0f));
    }

    // Test 4: PageUp clamps to 0.0
    {
        ViewerFixture f;
        f.activate();
        f.viewer.set_scroll(0.1f);
        f.comp->OnEvent(ftxui::Event::PageUp);
        ASSERT_TRUE(approx(f.viewer.scroll(), 0.0f));
    }

    // Test 5: WheelDown increases scroll by 0.05
    {
        ViewerFixture f;
        // Wheel scroll works without activation.
        ASSERT_TRUE(approx(f.viewer.scroll(), 0.0f));
        f.comp->OnEvent(f.wheel(ftxui::Mouse::WheelDown));
        ASSERT_TRUE(approx(f.viewer.scroll(), 0.05f));
    }

    // Test 6: WheelUp decreases scroll by 0.05
    {
        ViewerFixture f;
        f.viewer.set_scroll(0.5f);
        f.comp->OnEvent(f.wheel(ftxui::Mouse::WheelUp));
        ASSERT_TRUE(approx(f.viewer.scroll(), 0.45f));
    }

    // Test 7: WheelDown clamps to 1.0
    {
        ViewerFixture f;
        f.viewer.set_scroll(0.98f);
        f.comp->OnEvent(f.wheel(ftxui::Mouse::WheelDown));
        ASSERT_TRUE(approx(f.viewer.scroll(), 1.0f));
    }

    // Test 8: WheelUp clamps to 0.0
    {
        ViewerFixture f;
        f.viewer.set_scroll(0.02f);
        f.comp->OnEvent(f.wheel(ftxui::Mouse::WheelUp));
        ASSERT_TRUE(approx(f.viewer.scroll(), 0.0f));
    }

    // Test 9: Multiple PageDown + PageUp round-trip
    {
        ViewerFixture f;
        f.activate();
        f.comp->OnEvent(ftxui::Event::PageDown); // 0.3
        f.comp->OnEvent(ftxui::Event::PageDown); // 0.6
        f.comp->OnEvent(ftxui::Event::PageDown); // 0.9
        f.comp->OnEvent(ftxui::Event::PageDown); // 1.0 (clamped)
        ASSERT_TRUE(approx(f.viewer.scroll(), 1.0f));
        f.comp->OnEvent(ftxui::Event::PageUp);   // 0.7
        ASSERT_TRUE(approx(f.viewer.scroll(), 0.7f));
    }

    // Test 10: Tab ring mode — PageUp/PageDown work with externals
    {
        ViewerFixture f;
        f.viewer.add_focusable("Reply", "reply");
        // Re-render to pick up externals.
        ftxui::Render(f.screen, f.comp->Render());
        f.comp->OnEvent(ftxui::Event::PageDown);
        ASSERT_TRUE(approx(f.viewer.scroll(), 0.3f));
        f.comp->OnEvent(ftxui::Event::PageUp);
        ASSERT_TRUE(approx(f.viewer.scroll(), 0.0f));
    }

    return 0;
}
