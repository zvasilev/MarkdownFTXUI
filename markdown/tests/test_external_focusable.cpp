#include "test_helper.hpp"
#include "markdown/viewer.hpp"
#include "markdown/parser.hpp"

#include <ftxui/screen/screen.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/component/event.hpp>

using namespace markdown;

int main() {
    // Test 1: add_focusable / externals()
    {
        Viewer viewer(make_cmark_parser());
        ASSERT_EQ(static_cast<int>(viewer.externals().size()), 0);

        viewer.add_focusable("From", "alice@example.com");
        viewer.add_focusable("To", "bob@example.com");
        ASSERT_EQ(static_cast<int>(viewer.externals().size()), 2);
        ASSERT_EQ(viewer.externals()[0].label, "From");
        ASSERT_EQ(viewer.externals()[0].value, "alice@example.com");
        ASSERT_EQ(viewer.externals()[1].label, "To");
        ASSERT_EQ(viewer.externals()[1].value, "bob@example.com");
    }

    // Test 2: clear_focusables()
    {
        Viewer viewer(make_cmark_parser());
        viewer.add_focusable("A", "a");
        viewer.add_focusable("B", "b");
        ASSERT_EQ(static_cast<int>(viewer.externals().size()), 2);
        viewer.clear_focusables();
        ASSERT_EQ(static_cast<int>(viewer.externals().size()), 0);
        ASSERT_EQ(viewer.focused_index(), -1);
    }

    // Test 3: focused_index() defaults to -1
    {
        Viewer viewer(make_cmark_parser());
        ASSERT_EQ(viewer.focused_index(), -1);
    }

    // Test 4: is_external_focused() returns false when nothing focused
    {
        Viewer viewer(make_cmark_parser());
        viewer.add_focusable("X", "x");
        ASSERT_TRUE(!viewer.is_external_focused(0));
        ASSERT_TRUE(!viewer.is_external_focused(-1));
        ASSERT_TRUE(!viewer.is_external_focused(1));
    }

    // Test 5: Tab ring mode — Tab cycles through externals then links
    {
        Viewer viewer(make_cmark_parser());
        viewer.add_focusable("From", "alice@example.com");
        viewer.add_focusable("To", "bob@example.com");

        std::string cb_value;
        LinkEvent cb_event = LinkEvent::Focus;
        viewer.on_link_click([&](std::string const& v, LinkEvent e) {
            cb_value = v;
            cb_event = e;
        });
        viewer.set_content("[link](https://example.com)");
        auto comp = viewer.component();
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(3));
        ftxui::Render(screen, comp->Render());

        // Tab to first external (index 0)
        comp->OnEvent(ftxui::Event::Tab);
        ASSERT_EQ(viewer.focused_index(), 0);
        ASSERT_TRUE(viewer.is_external_focused(0));
        ASSERT_TRUE(!viewer.is_external_focused(1));
        ASSERT_EQ(cb_value, "alice@example.com");
        ASSERT_EQ(cb_event, LinkEvent::Focus);

        // Tab to second external (index 1)
        comp->OnEvent(ftxui::Event::Tab);
        ASSERT_EQ(viewer.focused_index(), 1);
        ASSERT_TRUE(!viewer.is_external_focused(0));
        ASSERT_TRUE(viewer.is_external_focused(1));
        ASSERT_EQ(cb_value, "bob@example.com");

        // Tab to link (index 2)
        comp->OnEvent(ftxui::Event::Tab);
        ASSERT_EQ(viewer.focused_index(), 2);
        ASSERT_TRUE(!viewer.is_external_focused(0));
        ASSERT_TRUE(!viewer.is_external_focused(1));
        ASSERT_EQ(cb_value, "https://example.com");

        // Tab wraps back to first external
        comp->OnEvent(ftxui::Event::Tab);
        ASSERT_EQ(viewer.focused_index(), 0);
        ASSERT_TRUE(viewer.is_external_focused(0));
        ASSERT_EQ(cb_value, "alice@example.com");
    }

    // Test 6: Shift+Tab cycles backward in tab ring
    {
        Viewer viewer(make_cmark_parser());
        viewer.add_focusable("A", "val_a");
        viewer.add_focusable("B", "val_b");

        std::string cb_value;
        viewer.on_link_click([&](std::string const& v, LinkEvent) {
            cb_value = v;
        });
        viewer.set_content("No links");
        auto comp = viewer.component();
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(3));
        ftxui::Render(screen, comp->Render());

        // Shift+Tab from -1 wraps to last external (index 1)
        comp->OnEvent(ftxui::Event::TabReverse);
        ASSERT_EQ(viewer.focused_index(), 1);
        ASSERT_EQ(cb_value, "val_b");

        // Shift+Tab to first external (index 0)
        comp->OnEvent(ftxui::Event::TabReverse);
        ASSERT_EQ(viewer.focused_index(), 0);
        ASSERT_EQ(cb_value, "val_a");
    }

    // Test 7: focused_value() returns correct value
    {
        Viewer viewer(make_cmark_parser());
        viewer.add_focusable("Key", "the_value");

        std::string cb_value;
        viewer.on_link_click([&](std::string const& v, LinkEvent) {
            cb_value = v;
        });
        viewer.set_content("[link](https://url.com)");
        auto comp = viewer.component();
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(3));
        ftxui::Render(screen, comp->Render());

        // Nothing focused
        ASSERT_EQ(viewer.focused_value(), "");

        // Tab to external
        comp->OnEvent(ftxui::Event::Tab);
        ASSERT_EQ(viewer.focused_value(), "the_value");

        // Tab to link
        comp->OnEvent(ftxui::Event::Tab);
        ASSERT_EQ(viewer.focused_value(), "https://url.com");
    }

    // Test 8: Enter fires Press event in tab ring mode
    {
        Viewer viewer(make_cmark_parser());
        viewer.add_focusable("Item", "item_val");

        std::string cb_value;
        LinkEvent cb_event = LinkEvent::Focus;
        viewer.on_link_click([&](std::string const& v, LinkEvent e) {
            cb_value = v;
            cb_event = e;
        });
        viewer.set_content("text");
        auto comp = viewer.component();
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(3));
        ftxui::Render(screen, comp->Render());

        // Tab to external, then Enter to press
        comp->OnEvent(ftxui::Event::Tab);
        ASSERT_EQ(cb_event, LinkEvent::Focus);
        comp->OnEvent(ftxui::Event::Return);
        ASSERT_EQ(cb_event, LinkEvent::Press);
        ASSERT_EQ(cb_value, "item_val");
    }

    // Test 9: Escape resets focus index in tab ring mode
    {
        Viewer viewer(make_cmark_parser());
        viewer.add_focusable("X", "x_val");
        viewer.set_content("text");
        auto comp = viewer.component();
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(3));
        ftxui::Render(screen, comp->Render());

        comp->OnEvent(ftxui::Event::Tab);
        ASSERT_EQ(viewer.focused_index(), 0);
        comp->OnEvent(ftxui::Event::Escape);
        ASSERT_EQ(viewer.focused_index(), -1);
    }

    // Test 10: New getters — is_embed, scrollbar_visible, theme
    {
        Viewer viewer(make_cmark_parser());
        ASSERT_TRUE(!viewer.is_embed());
        ASSERT_TRUE(viewer.scrollbar_visible());

        viewer.set_embed(true);
        ASSERT_TRUE(viewer.is_embed());

        viewer.show_scrollbar(false);
        ASSERT_TRUE(!viewer.scrollbar_visible());

        viewer.set_theme(theme_colorful());
        // Just verify it doesn't crash and returns a reference
        auto const& t = viewer.theme();
        (void)t;
    }

    return 0;
}
