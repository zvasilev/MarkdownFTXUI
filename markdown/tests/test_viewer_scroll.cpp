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

    // Viewport-proportional page step (matches ViewerWrap::page_step()).
    float page_step() const {
        auto const& si = viewer.scroll_info();
        if (si.content_height <= si.viewport_height) return 1.0f;
        return static_cast<float>(si.viewport_height)
             / static_cast<float>(si.content_height);
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
    // Test 1: ScrollInfo is populated after render
    {
        ViewerFixture f;
        auto const& si = f.viewer.scroll_info();
        ASSERT_TRUE(si.viewport_height > 0);
        ASSERT_TRUE(si.content_height > si.viewport_height);
    }

    // Test 2: PageDown increases scroll by viewport-proportional step
    {
        ViewerFixture f;
        f.activate();
        float step = f.page_step();
        ASSERT_TRUE(step > 0.0f);
        ASSERT_TRUE(step < 1.0f);
        ASSERT_TRUE(approx(f.viewer.scroll(), 0.0f));
        f.comp->OnEvent(ftxui::Event::PageDown);
        ASSERT_TRUE(approx(f.viewer.scroll(), step));
    }

    // Test 3: PageUp decreases scroll by the same step
    {
        ViewerFixture f;
        f.activate();
        float step = f.page_step();
        f.viewer.set_scroll(0.5f);
        f.comp->OnEvent(ftxui::Event::PageUp);
        ASSERT_TRUE(approx(f.viewer.scroll(), 0.5f - step));
    }

    // Test 4: PageDown clamps to 1.0
    {
        ViewerFixture f;
        f.activate();
        f.viewer.set_scroll(1.0f - f.page_step() * 0.5f);
        f.comp->OnEvent(ftxui::Event::PageDown);
        ASSERT_TRUE(approx(f.viewer.scroll(), 1.0f));
    }

    // Test 5: PageUp clamps to 0.0
    {
        ViewerFixture f;
        f.activate();
        f.viewer.set_scroll(f.page_step() * 0.5f);
        f.comp->OnEvent(ftxui::Event::PageUp);
        ASSERT_TRUE(approx(f.viewer.scroll(), 0.0f));
    }

    // Test 6: WheelDown increases scroll by 0.05
    {
        ViewerFixture f;
        ASSERT_TRUE(approx(f.viewer.scroll(), 0.0f));
        f.comp->OnEvent(f.wheel(ftxui::Mouse::WheelDown));
        ASSERT_TRUE(approx(f.viewer.scroll(), 0.05f));
    }

    // Test 7: WheelUp decreases scroll by 0.05
    {
        ViewerFixture f;
        f.viewer.set_scroll(0.5f);
        f.comp->OnEvent(f.wheel(ftxui::Mouse::WheelUp));
        ASSERT_TRUE(approx(f.viewer.scroll(), 0.45f));
    }

    // Test 8: WheelDown clamps to 1.0
    {
        ViewerFixture f;
        f.viewer.set_scroll(0.98f);
        f.comp->OnEvent(f.wheel(ftxui::Mouse::WheelDown));
        ASSERT_TRUE(approx(f.viewer.scroll(), 1.0f));
    }

    // Test 9: WheelUp clamps to 0.0
    {
        ViewerFixture f;
        f.viewer.set_scroll(0.02f);
        f.comp->OnEvent(f.wheel(ftxui::Mouse::WheelUp));
        ASSERT_TRUE(approx(f.viewer.scroll(), 0.0f));
    }

    // Test 10: Home jumps to top
    {
        ViewerFixture f;
        f.activate();
        f.viewer.set_scroll(0.7f);
        f.comp->OnEvent(ftxui::Event::Home);
        ASSERT_TRUE(approx(f.viewer.scroll(), 0.0f));
    }

    // Test 11: End jumps to bottom
    {
        ViewerFixture f;
        f.activate();
        f.viewer.set_scroll(0.3f);
        f.comp->OnEvent(ftxui::Event::End);
        ASSERT_TRUE(approx(f.viewer.scroll(), 1.0f));
    }

    // Test 12: Home at top stays at 0
    {
        ViewerFixture f;
        f.activate();
        f.comp->OnEvent(ftxui::Event::Home);
        ASSERT_TRUE(approx(f.viewer.scroll(), 0.0f));
    }

    // Test 13: End at bottom stays at 1
    {
        ViewerFixture f;
        f.activate();
        f.viewer.set_scroll(1.0f);
        f.comp->OnEvent(ftxui::Event::End);
        ASSERT_TRUE(approx(f.viewer.scroll(), 1.0f));
    }

    // Test 14: Tab ring mode — PageUp/PageDown + Home/End work with externals
    {
        ViewerFixture f;
        f.viewer.add_focusable("Reply", "reply");
        ftxui::Render(f.screen, f.comp->Render());
        float step = f.page_step();
        f.comp->OnEvent(ftxui::Event::PageDown);
        ASSERT_TRUE(approx(f.viewer.scroll(), step));
        f.comp->OnEvent(ftxui::Event::End);
        ASSERT_TRUE(approx(f.viewer.scroll(), 1.0f));
        f.comp->OnEvent(ftxui::Event::Home);
        ASSERT_TRUE(approx(f.viewer.scroll(), 0.0f));
    }

    return 0;
}
