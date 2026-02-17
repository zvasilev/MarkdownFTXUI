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

    // Test 14: Scroll keys work when activated via enter_focus
    {
        // Build a viewer with a link + long content so scroll_info is valid
        Viewer viewer(make_cmark_parser());
        std::string content = "[link](https://a.com)\n\n";
        for (int i = 0; i < 50; ++i)
            content += "Line " + std::to_string(i) + "\n\n";
        viewer.set_content(content);
        auto comp = viewer.component();
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(60),
                                            ftxui::Dimension::Fixed(10));
        ftxui::Render(screen, comp->Render());

        // scroll_info populated from direct_scroll during first render
        auto const& si = viewer.scroll_info();
        ASSERT_TRUE(si.viewport_height > 0);
        ASSERT_TRUE(si.content_height > si.viewport_height);
        float step = static_cast<float>(si.viewport_height)
                   / static_cast<float>(si.content_height);

        ASSERT_TRUE(viewer.enter_focus(+1));
        ASSERT_TRUE(viewer.active());
        comp->OnEvent(ftxui::Event::PageDown);
        ASSERT_TRUE(approx(viewer.scroll(), step));
        comp->OnEvent(ftxui::Event::End);
        ASSERT_TRUE(approx(viewer.scroll(), 1.0f));
        comp->OnEvent(ftxui::Event::Home);
        ASSERT_TRUE(approx(viewer.scroll(), 0.0f));
    }

    // Test 15: Arrow scroll works after enter_focus with re-renders
    {
        Viewer viewer(make_cmark_parser());
        std::string content = "[link1](https://a.com)\n\n";
        for (int i = 0; i < 50; ++i)
            content += "Line " + std::to_string(i) + "\n\n";
        content += "[link2](https://b.com)\n";
        viewer.set_content(content);
        auto comp = viewer.component();
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(60),
                                            ftxui::Dimension::Fixed(10));
        ftxui::Render(screen, comp->Render());

        // Focus first link
        ASSERT_TRUE(viewer.enter_focus(+1));
        ASSERT_TRUE(viewer.active());
        ASSERT_TRUE(approx(viewer.scroll(), 0.0f));

        // Re-render (auto-scroll may run), then ArrowDown
        ftxui::Render(screen, comp->Render());
        comp->OnEvent(ftxui::Event::ArrowDown);
        float after_first = viewer.scroll();
        ASSERT_TRUE(after_first > 0.0f);

        // Re-render, then another ArrowDown — scroll should increase
        ftxui::Render(screen, comp->Render());
        comp->OnEvent(ftxui::Event::ArrowDown);
        float after_second = viewer.scroll();
        ASSERT_TRUE(after_second > after_first);
    }

    // Test 16: ArrowUp scrolls back after ArrowDown with link focused
    {
        Viewer viewer(make_cmark_parser());
        std::string content = "[link](https://a.com)\n\n";
        for (int i = 0; i < 50; ++i)
            content += "Line " + std::to_string(i) + "\n\n";
        viewer.set_content(content);
        auto comp = viewer.component();
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(60),
                                            ftxui::Dimension::Fixed(10));
        ftxui::Render(screen, comp->Render());

        viewer.enter_focus(+1);
        ftxui::Render(screen, comp->Render());

        // Scroll down a few times
        comp->OnEvent(ftxui::Event::ArrowDown);
        comp->OnEvent(ftxui::Event::ArrowDown);
        comp->OnEvent(ftxui::Event::ArrowDown);
        float scrolled = viewer.scroll();
        ASSERT_TRUE(scrolled > 0.0f);

        // Scroll back up
        comp->OnEvent(ftxui::Event::ArrowUp);
        ASSERT_TRUE(viewer.scroll() < scrolled);
    }

    // Test 17: Tab to off-screen link auto-scrolls
    {
        Viewer viewer(make_cmark_parser());
        std::string content = "[link1](https://a.com)\n\n";
        for (int i = 0; i < 50; ++i)
            content += "Line " + std::to_string(i) + "\n\n";
        content += "[link2](https://b.com)\n";
        viewer.set_content(content);
        auto comp = viewer.component();
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(60),
                                            ftxui::Dimension::Fixed(10));
        ftxui::Render(screen, comp->Render());

        // Enter focus on link1 (at top, no scroll needed)
        ASSERT_TRUE(viewer.enter_focus(+1));
        ASSERT_EQ(viewer.focused_index(), 0);
        ASSERT_TRUE(approx(viewer.scroll(), 0.0f));

        // Render with link1 focused so boxes are fresh
        ftxui::Render(screen, comp->Render());

        // Tab to link2 (at bottom, off-screen)
        comp->OnEvent(ftxui::Event::Tab);
        ASSERT_EQ(viewer.focused_index(), 1);

        // scroll_to_focus should have moved scroll to show link2
        ASSERT_TRUE(viewer.scroll() > 0.5f);
    }

    // Test 18: Embed mode — Tab auto-scroll and arrows with external elements
    {
        Viewer viewer(make_cmark_parser());
        std::string content = "[link1](https://a.com)\n\n";
        for (int i = 0; i < 50; ++i)
            content += "Line " + std::to_string(i) + "\n\n";
        content += "[link2](https://b.com)\n";
        viewer.set_content(content);
        viewer.set_embed(true);

        ScrollInfo ext_si;
        viewer.set_external_scroll_info(&ext_si);

        auto comp = viewer.component();
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(60),
                                            ftxui::Dimension::Fixed(15));

        // Simulate parent: headers + separator above viewer element
        auto render_combined = [&]() {
            ftxui::Elements headers;
            for (int i = 0; i < 5; ++i)
                headers.push_back(ftxui::text("Header " + std::to_string(i)));
            auto combined = ftxui::vbox({
                ftxui::vbox(std::move(headers)),
                ftxui::separator(),
                comp->Render(),
            });
            return markdown::direct_scroll(
                std::move(combined), viewer.scroll(), &ext_si);
        };

        ftxui::Render(screen, render_combined());
        ASSERT_TRUE(ext_si.viewport_height > 0);
        ASSERT_TRUE(ext_si.content_height > ext_si.viewport_height);

        // Enter focus, render, tab to off-screen link2
        ASSERT_TRUE(viewer.enter_focus(+1));
        ftxui::Render(screen, render_combined());
        comp->OnEvent(ftxui::Event::Tab);
        ASSERT_EQ(viewer.focused_index(), 1);
        ASSERT_TRUE(viewer.scroll() > 0.5f);

        // Arrows work after tab in embed mode
        ftxui::Render(screen, render_combined());
        float before = viewer.scroll();
        comp->OnEvent(ftxui::Event::ArrowUp);
        ASSERT_TRUE(viewer.scroll() < before);
    }

    // Test 19: Embed mode — parent set_scroll works
    {
        Viewer viewer(make_cmark_parser());
        std::string content;
        for (int i = 0; i < 50; ++i)
            content += "Line " + std::to_string(i) + "\n\n";
        viewer.set_content(content);
        viewer.set_embed(true);

        ScrollInfo ext_si;
        viewer.set_external_scroll_info(&ext_si);

        auto comp = viewer.component();
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(60),
                                            ftxui::Dimension::Fixed(15));

        auto render_combined = [&]() {
            ftxui::Elements headers;
            for (int i = 0; i < 5; ++i)
                headers.push_back(ftxui::text("Header " + std::to_string(i)));
            auto combined = ftxui::vbox({
                ftxui::vbox(std::move(headers)),
                ftxui::separator(),
                comp->Render(),
            });
            return markdown::direct_scroll(
                std::move(combined), viewer.scroll(), &ext_si);
        };

        ftxui::Render(screen, render_combined());
        ASSERT_TRUE(!viewer.active());

        // Parent sets scroll directly (simulating parent's arrow handler)
        viewer.set_scroll(0.5f);
        ASSERT_TRUE(approx(viewer.scroll(), 0.5f));
    }

    return 0;
}
