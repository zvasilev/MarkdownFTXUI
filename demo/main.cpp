#include <string>
#include <vector>

#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include "screens.hpp"

int main() {
    auto screen = ftxui::ScreenInteractive::Fullscreen();

    // --- Shared state ---
    int current_screen = 0;
    int menu_selected = 0;
    int theme_index = 0;
    std::vector<std::string> theme_names = {"Default", "Contrast", "Colorful"};

    // =====================================================================
    // Screen 0: Main Menu
    // =====================================================================
    auto theme_toggle = ftxui::Toggle(&theme_names, &theme_index);
    auto menu_entries = std::vector<std::string>{
        "Editor + Viewer",
        "Viewer with Scroll",
        "Email Viewer",
        "Newsletter Viewer",
        "Depth Fallback",
    };
    auto menu_descriptions = std::vector<std::string>{
        "Side-by-side editing with live preview",
        "Scrollable markdown viewer with scrollbar",
        "Simulated email with combined scroll",
        "Real-world Apple Developer newsletter",
        "Deep nesting: styled vs plain-text fallback",
    };
    auto menu_option = ftxui::MenuOption::Vertical();
    menu_option.on_enter = [&] { current_screen = menu_selected + 1; };
    menu_option.entries_option.transform = [&](ftxui::EntryState s) {
        auto num = std::to_string(s.index + 1) + ". ";
        auto title = ftxui::text(num + s.label) | ftxui::bold;
        auto desc = ftxui::text("   " + menu_descriptions[s.index])
            | ftxui::dim;
        auto el = ftxui::vbox({title, desc});
        if (s.focused) el = el | ftxui::inverted;
        return el;
    };
    auto menu_comp = ftxui::Menu(&menu_entries, &menu_selected, menu_option);
    auto menu_container = ftxui::Container::Vertical({
        theme_toggle, menu_comp});

    auto menu_screen = ftxui::Renderer(menu_container, [&] {
        return ftxui::vbox({
            ftxui::filler(),
            ftxui::vbox({
                ftxui::text("MarkdownFTXUI Demo") | ftxui::bold
                    | ftxui::center,
                ftxui::text(""),
                menu_comp->Render() | ftxui::center,
                ftxui::text(""),
                ftxui::hbox({
                    ftxui::text("Theme: "),
                    theme_toggle->Render(),
                }) | ftxui::center,
                ftxui::text(""),
                ftxui::text("Enter to select") | ftxui::dim | ftxui::center,
            }) | ftxui::center,
            ftxui::filler(),
        });
    });

    // =====================================================================
    // Sub-demo screens (each handles its own Esc → menu)
    // =====================================================================
    auto screen1 = make_editor_screen(current_screen, theme_index,
                                      theme_names);
    auto screen2 = make_viewer_screen(current_screen, theme_index,
                                      theme_names);
    auto screen3 = make_email_screen(current_screen, theme_index,
                                     theme_names);
    auto screen4 = make_newsletter_screen(current_screen, theme_index,
                                          theme_names);
    auto screen5 = make_depth_screen(current_screen, theme_index,
                                     theme_names);

    // =====================================================================
    // Root: Tab switch + menu Esc
    // =====================================================================
    auto tab = ftxui::Container::Tab(
        {menu_screen, screen1, screen2, screen3, screen4, screen5},
        &current_screen);

    auto root = ftxui::CatchEvent(tab, [&](ftxui::Event event) {
        if (event == ftxui::Event::Escape && current_screen == 0) {
            screen.Exit();
            return true;
        }
        return false;
    });

    screen.Loop(root);
    return 0;
}
