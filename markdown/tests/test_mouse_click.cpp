#include "test_helper.hpp"
#include "markdown/viewer.hpp"
#include "markdown/parser.hpp"

#include <ftxui/screen/screen.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/mouse.hpp>

using namespace markdown;

int main() {
    // Test 1: Mouse click on a link fires callback with correct URL
    {
        Viewer viewer(make_cmark_parser());
        std::string clicked_url;
        LinkEvent clicked_event = LinkEvent::Focus;
        bool cb_called = false;
        viewer.on_link_click([&](std::string const& url, LinkEvent ev) {
            clicked_url = url;
            clicked_event = ev;
            cb_called = true;
        });
        viewer.set_content("[click me](https://target.com)");
        auto comp = viewer.component();

        // Render twice: first render builds element tree + reflect boxes,
        // second render fills reflect boxes with layout coordinates.
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(3));
        ftxui::Render(screen, comp->Render());
        ftxui::Render(screen, comp->Render());

        // Find a link box with valid (non-zero) coordinates
        auto const& flat_boxes = viewer.focused_link_box();
        // Instead, just click at a position where the link text should be.
        // The link "click me" starts near column 0-1, row 0.
        // Use coordinates within the rendered area.
        ftxui::Mouse mouse;
        mouse.button = ftxui::Mouse::Left;
        mouse.motion = ftxui::Mouse::Pressed;
        mouse.x = 3; // somewhere over "click me"
        mouse.y = 0;
        comp->OnEvent(ftxui::Event::Mouse("", mouse));

        ASSERT_TRUE(cb_called);
        ASSERT_EQ(clicked_url, "https://target.com");
        ASSERT_EQ(clicked_event, LinkEvent::Press);
    }

    // Test 2: Mouse click outside link area does not fire callback
    {
        Viewer viewer(make_cmark_parser());
        bool cb_called = false;
        viewer.on_link_click([&](std::string const&, LinkEvent) {
            cb_called = true;
        });
        viewer.set_content("plain text [link](https://x.com)");
        auto comp = viewer.component();

        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(3));
        ftxui::Render(screen, comp->Render());
        ftxui::Render(screen, comp->Render());

        // Click far to the right where there's no link
        ftxui::Mouse mouse;
        mouse.button = ftxui::Mouse::Left;
        mouse.motion = ftxui::Mouse::Pressed;
        mouse.x = 70;
        mouse.y = 0;
        comp->OnEvent(ftxui::Event::Mouse("", mouse));

        ASSERT_TRUE(!cb_called);
    }

    // Test 3: Mouse click on second of two links hits the right URL
    {
        Viewer viewer(make_cmark_parser());
        std::string clicked_url;
        viewer.on_link_click([&](std::string const& url, LinkEvent) {
            clicked_url = url;
        });
        viewer.set_content(
            "[first](https://first.com) and [second](https://second.com)");
        auto comp = viewer.component();

        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(3));
        ftxui::Render(screen, comp->Render());
        ftxui::Render(screen, comp->Render());

        // "first" is 5 chars (0-4), " and " is 5 chars (5-9),
        // "second" starts at col 10
        ftxui::Mouse mouse;
        mouse.button = ftxui::Mouse::Left;
        mouse.motion = ftxui::Mouse::Pressed;
        mouse.x = 12;
        mouse.y = 0;
        comp->OnEvent(ftxui::Event::Mouse("", mouse));

        ASSERT_EQ(clicked_url, "https://second.com");
    }

    return 0;
}
