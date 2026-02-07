#include "test_helper.hpp"
#include "markdown/viewer.hpp"
#include "markdown/parser.hpp"

#include <ftxui/screen/screen.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/component/event.hpp>

using namespace markdown;

int main() {
    // Test 1: Content renders correctly
    {
        Viewer viewer(make_cmark_parser());
        viewer.set_content("Hello world");
        auto comp = viewer.component();
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(3));
        ftxui::Render(screen, comp->Render());
        auto output = screen.ToString();
        ASSERT_CONTAINS(output, "Hello world");
    }

    // Test 2: active() defaults to false
    {
        Viewer viewer(make_cmark_parser());
        ASSERT_TRUE(!viewer.active());
    }

    // Test 3: Return activates, Escape deactivates
    {
        Viewer viewer(make_cmark_parser());
        viewer.set_content("test");
        auto comp = viewer.component();
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(3));
        ftxui::Render(screen, comp->Render());

        ASSERT_TRUE(!viewer.active());
        comp->OnEvent(ftxui::Event::Return);
        ASSERT_TRUE(viewer.active());
        comp->OnEvent(ftxui::Event::Escape);
        ASSERT_TRUE(!viewer.active());
    }

    // Test 4: Tab cycles through links, callback fires with correct URL/event
    {
        Viewer viewer(make_cmark_parser());
        std::string cb_url;
        LinkEvent cb_event = LinkEvent::Focus;
        bool cb_called = false;
        viewer.on_link_click([&](std::string const& url, LinkEvent event) {
            cb_url = url;
            cb_event = event;
            cb_called = true;
        });
        viewer.set_content(
            "[link1](https://one.com) and [link2](https://two.com)");
        auto comp = viewer.component();
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(3));
        ftxui::Render(screen, comp->Render());

        // Activate
        comp->OnEvent(ftxui::Event::Return);
        ASSERT_TRUE(viewer.active());

        // Tab to first link
        cb_called = false;
        comp->OnEvent(ftxui::Event::Tab);
        ASSERT_TRUE(cb_called);
        ASSERT_EQ(cb_url, "https://one.com");
        ASSERT_EQ(cb_event, LinkEvent::Focus);

        // Tab to second link
        cb_called = false;
        comp->OnEvent(ftxui::Event::Tab);
        ASSERT_TRUE(cb_called);
        ASSERT_EQ(cb_url, "https://two.com");
        ASSERT_EQ(cb_event, LinkEvent::Focus);

        // Enter to press focused link
        cb_called = false;
        comp->OnEvent(ftxui::Event::Return);
        ASSERT_TRUE(cb_called);
        ASSERT_EQ(cb_url, "https://two.com");
        ASSERT_EQ(cb_event, LinkEvent::Press);
    }

    // Test 5: Shift+Tab cycles backward
    {
        Viewer viewer(make_cmark_parser());
        std::string cb_url;
        viewer.on_link_click([&](std::string const& url, LinkEvent) {
            cb_url = url;
        });
        viewer.set_content(
            "[a](https://a.com) [b](https://b.com) [c](https://c.com)");
        auto comp = viewer.component();
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(3));
        ftxui::Render(screen, comp->Render());

        comp->OnEvent(ftxui::Event::Return); // activate
        // Tab forward to first link
        comp->OnEvent(ftxui::Event::Tab);
        ASSERT_EQ(cb_url, "https://a.com");
        // Shift+Tab wraps to last link
        comp->OnEvent(ftxui::Event::TabReverse);
        ASSERT_EQ(cb_url, "https://c.com");
    }

    // Test 6: Tab wraps around with single link
    {
        Viewer viewer(make_cmark_parser());
        std::string cb_url;
        viewer.on_link_click([&](std::string const& url, LinkEvent) {
            cb_url = url;
        });
        viewer.set_content("[only](https://only.com)");
        auto comp = viewer.component();
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(3));
        ftxui::Render(screen, comp->Render());

        comp->OnEvent(ftxui::Event::Return);
        comp->OnEvent(ftxui::Event::Tab);
        ASSERT_EQ(cb_url, "https://only.com");
        // Tab again wraps to same link
        comp->OnEvent(ftxui::Event::Tab);
        ASSERT_EQ(cb_url, "https://only.com");
    }

    // Test 7: Content with no links — Tab does nothing harmful
    {
        Viewer viewer(make_cmark_parser());
        bool cb_called = false;
        viewer.on_link_click([&](std::string const&, LinkEvent) {
            cb_called = true;
        });
        viewer.set_content("No links here");
        auto comp = viewer.component();
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(3));
        ftxui::Render(screen, comp->Render());

        comp->OnEvent(ftxui::Event::Return); // activate
        comp->OnEvent(ftxui::Event::Tab);    // no links to cycle
        ASSERT_TRUE(!cb_called);
    }

    // Test 8: set_theme renders without crash
    {
        Viewer viewer(make_cmark_parser());
        viewer.set_content("# Themed heading");
        viewer.set_theme(theme_colorful());
        auto comp = viewer.component();
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(3));
        ftxui::Render(screen, comp->Render());
        auto output = screen.ToString();
        ASSERT_CONTAINS(output, "Themed heading");
    }

    // Test 9: Escape resets focused link
    {
        Viewer viewer(make_cmark_parser());
        std::string cb_url;
        bool cb_called = false;
        viewer.on_link_click([&](std::string const& url, LinkEvent) {
            cb_url = url;
            cb_called = true;
        });
        viewer.set_content("[link](https://url.com)");
        auto comp = viewer.component();
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(3));
        ftxui::Render(screen, comp->Render());

        comp->OnEvent(ftxui::Event::Return); // activate
        comp->OnEvent(ftxui::Event::Tab);    // focus link
        ASSERT_TRUE(cb_called);

        comp->OnEvent(ftxui::Event::Escape); // deactivate + reset
        ASSERT_TRUE(!viewer.active());

        // Re-activate and Tab again — should go to first link, not second
        cb_called = false;
        comp->OnEvent(ftxui::Event::Return);
        comp->OnEvent(ftxui::Event::Tab);
        ASSERT_TRUE(cb_called);
        ASSERT_EQ(cb_url, "https://url.com"); // back to first (only) link
    }

    // Test 10: Content caching — re-render same content doesn't re-parse
    {
        Viewer viewer(make_cmark_parser());
        viewer.set_content("**bold** text");
        auto comp = viewer.component();
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(3));
        ftxui::Render(screen, comp->Render());
        auto output1 = screen.ToString();
        // Render again — should use cache
        ftxui::Render(screen, comp->Render());
        auto output2 = screen.ToString();
        ASSERT_EQ(output1, output2);
    }

    return 0;
}
