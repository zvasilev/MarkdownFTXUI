#include "test_helper.hpp"
#include "markdown/viewer.hpp"
#include "markdown/parser.hpp"

#include <ftxui/screen/screen.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/component/event.hpp>

using namespace markdown;

int main() {
    // Test 1: Tab forward past last link fires on_tab_exit(+1) in normal mode
    {
        Viewer viewer(make_cmark_parser());
        viewer.set_content("[a](https://a.com) [b](https://b.com)");
        int exit_dir = 0;
        viewer.on_tab_exit([&](int d) { exit_dir = d; });
        auto comp = viewer.component();
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(3));
        ftxui::Render(screen, comp->Render());

        comp->OnEvent(ftxui::Event::Return); // activate
        ASSERT_TRUE(viewer.active());

        comp->OnEvent(ftxui::Event::Tab); // link 0
        ASSERT_EQ(viewer.focused_index(), 0);
        comp->OnEvent(ftxui::Event::Tab); // link 1
        ASSERT_EQ(viewer.focused_index(), 1);
        comp->OnEvent(ftxui::Event::Tab); // past last -> exit
        ASSERT_EQ(exit_dir, 1);
        ASSERT_EQ(viewer.focused_index(), -1);
        ASSERT_TRUE(!viewer.active());
    }

    // Test 2: Shift+Tab backward past first link fires on_tab_exit(-1)
    {
        Viewer viewer(make_cmark_parser());
        viewer.set_content("[a](https://a.com) [b](https://b.com)");
        int exit_dir = 0;
        viewer.on_tab_exit([&](int d) { exit_dir = d; });
        auto comp = viewer.component();
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(3));
        ftxui::Render(screen, comp->Render());

        comp->OnEvent(ftxui::Event::Return); // activate
        comp->OnEvent(ftxui::Event::Tab);    // link 0
        ASSERT_EQ(viewer.focused_index(), 0);
        comp->OnEvent(ftxui::Event::TabReverse); // past first -> exit
        ASSERT_EQ(exit_dir, -1);
        ASSERT_EQ(viewer.focused_index(), -1);
        ASSERT_TRUE(!viewer.active());
    }

    // Test 3: enter_focus(+1) activates and focuses first link
    {
        Viewer viewer(make_cmark_parser());
        std::string cb_url;
        viewer.on_link_click([&](std::string const& url, LinkEvent) {
            cb_url = url;
        });
        viewer.set_content("[first](https://first.com) [second](https://second.com)");
        auto comp = viewer.component();
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(3));
        ftxui::Render(screen, comp->Render());

        ASSERT_TRUE(!viewer.active());
        bool accepted = viewer.enter_focus(+1);
        ASSERT_TRUE(accepted);
        ASSERT_TRUE(viewer.active());
        ASSERT_EQ(viewer.focused_index(), 0);
        ASSERT_EQ(cb_url, "https://first.com");
    }

    // Test 4: enter_focus(-1) activates and focuses last link
    {
        Viewer viewer(make_cmark_parser());
        std::string cb_url;
        viewer.on_link_click([&](std::string const& url, LinkEvent) {
            cb_url = url;
        });
        viewer.set_content("[first](https://first.com) [second](https://second.com)");
        auto comp = viewer.component();
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(3));
        ftxui::Render(screen, comp->Render());

        bool accepted = viewer.enter_focus(-1);
        ASSERT_TRUE(accepted);
        ASSERT_TRUE(viewer.active());
        ASSERT_EQ(viewer.focused_index(), 1);
        ASSERT_EQ(cb_url, "https://second.com");
    }

    // Test 5: enter_focus returns false when no focusables
    {
        Viewer viewer(make_cmark_parser());
        viewer.set_content("No links here");
        auto comp = viewer.component();
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(3));
        ftxui::Render(screen, comp->Render());

        bool accepted = viewer.enter_focus(+1);
        ASSERT_TRUE(!accepted);
        ASSERT_TRUE(!viewer.active());
    }

    // Test 6: Without callback, Tab still wraps (current behavior preserved)
    {
        Viewer viewer(make_cmark_parser());
        std::string cb_url;
        viewer.on_link_click([&](std::string const& url, LinkEvent) {
            cb_url = url;
        });
        viewer.set_content("[a](https://a.com) [b](https://b.com)");
        auto comp = viewer.component();
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(3));
        ftxui::Render(screen, comp->Render());

        comp->OnEvent(ftxui::Event::Return); // activate
        comp->OnEvent(ftxui::Event::Tab);    // link 0
        ASSERT_EQ(cb_url, "https://a.com");
        comp->OnEvent(ftxui::Event::Tab);    // link 1
        ASSERT_EQ(cb_url, "https://b.com");
        comp->OnEvent(ftxui::Event::Tab);    // wraps to link 0
        ASSERT_EQ(cb_url, "https://a.com");
        ASSERT_EQ(viewer.focused_index(), 0);
        ASSERT_TRUE(viewer.active()); // still active, no exit
    }

    // Test 7: Escape deactivates without calling on_tab_exit
    {
        Viewer viewer(make_cmark_parser());
        viewer.set_content("[link](https://url.com)");
        bool exited = false;
        viewer.on_tab_exit([&](int) { exited = true; });
        auto comp = viewer.component();
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(3));
        ftxui::Render(screen, comp->Render());

        comp->OnEvent(ftxui::Event::Return); // activate
        comp->OnEvent(ftxui::Event::Tab);    // focus link
        ASSERT_EQ(viewer.focused_index(), 0);
        comp->OnEvent(ftxui::Event::Escape); // deactivate
        ASSERT_TRUE(!viewer.active());
        ASSERT_EQ(viewer.focused_index(), -1);
        ASSERT_TRUE(!exited); // on_tab_exit was NOT called
    }

    // Test 8: Full round-trip — enter_focus -> Tab through -> on_tab_exit
    {
        Viewer viewer(make_cmark_parser());
        viewer.set_content("[only](https://only.com)");
        int exit_dir = 0;
        bool exited = false;
        viewer.on_tab_exit([&](int d) { exit_dir = d; exited = true; });
        std::string cb_url;
        viewer.on_link_click([&](std::string const& url, LinkEvent) {
            cb_url = url;
        });
        auto comp = viewer.component();
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(3));
        ftxui::Render(screen, comp->Render());

        // Parent enters focus
        bool accepted = viewer.enter_focus(+1);
        ASSERT_TRUE(accepted);
        ASSERT_EQ(viewer.focused_index(), 0);
        ASSERT_EQ(cb_url, "https://only.com");

        // Tab past the only link -> exit
        comp->OnEvent(ftxui::Event::Tab);
        ASSERT_TRUE(exited);
        ASSERT_EQ(exit_dir, 1);
        ASSERT_TRUE(!viewer.active());
        ASSERT_EQ(viewer.focused_index(), -1);
    }

    // Test 9: Custom keys — activate with 'a', deactivate with 'q'
    {
        Viewer viewer(make_cmark_parser());
        viewer.set_content("[link](https://url.com)");
        ViewerKeys keys;
        keys.activate = ftxui::Event::Character('a');
        keys.deactivate = ftxui::Event::Character('q');
        viewer.set_keys(keys);

        auto comp = viewer.component();
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(3));
        ftxui::Render(screen, comp->Render());

        // Default Return should NOT activate
        comp->OnEvent(ftxui::Event::Return);
        ASSERT_TRUE(!viewer.active());

        // Custom 'a' should activate
        comp->OnEvent(ftxui::Event::Character('a'));
        ASSERT_TRUE(viewer.active());

        // Default Escape should NOT deactivate
        comp->OnEvent(ftxui::Event::Escape);
        ASSERT_TRUE(viewer.active());

        // Custom 'q' should deactivate
        comp->OnEvent(ftxui::Event::Character('q'));
        ASSERT_TRUE(!viewer.active());
    }

    // Test 10: Custom keys — next/prev with 'n'/'p'
    {
        Viewer viewer(make_cmark_parser());
        viewer.set_content("[a](https://a.com) [b](https://b.com)");
        std::string cb_url;
        viewer.on_link_click([&](std::string const& url, LinkEvent) {
            cb_url = url;
        });
        int exit_dir = 0;
        viewer.on_tab_exit([&](int d) { exit_dir = d; });

        ViewerKeys keys;
        keys.next = ftxui::Event::Character('n');
        keys.prev = ftxui::Event::Character('p');
        viewer.set_keys(keys);

        auto comp = viewer.component();
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(3));
        ftxui::Render(screen, comp->Render());

        comp->OnEvent(ftxui::Event::Return); // activate
        ASSERT_TRUE(viewer.active());

        // Default Tab should NOT cycle
        comp->OnEvent(ftxui::Event::Tab);
        ASSERT_EQ(viewer.focused_index(), -1);

        // Custom 'n' should cycle forward
        comp->OnEvent(ftxui::Event::Character('n'));
        ASSERT_EQ(viewer.focused_index(), 0);
        ASSERT_EQ(cb_url, "https://a.com");

        comp->OnEvent(ftxui::Event::Character('n'));
        ASSERT_EQ(viewer.focused_index(), 1);
        ASSERT_EQ(cb_url, "https://b.com");

        // Custom 'n' past last -> on_tab_exit(+1)
        comp->OnEvent(ftxui::Event::Character('n'));
        ASSERT_EQ(exit_dir, 1);
        ASSERT_TRUE(!viewer.active());

        // Re-activate and test 'p' (prev)
        comp->OnEvent(ftxui::Event::Return);
        comp->OnEvent(ftxui::Event::Character('p')); // focus last
        ASSERT_EQ(viewer.focused_index(), 1);
        comp->OnEvent(ftxui::Event::Character('p')); // focus first
        ASSERT_EQ(viewer.focused_index(), 0);
        comp->OnEvent(ftxui::Event::Character('p')); // past first -> exit(-1)
        ASSERT_EQ(exit_dir, -1);
    }

    return 0;
}
