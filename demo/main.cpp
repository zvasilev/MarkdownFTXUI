#include <iterator>
#include <string>
#include <vector>

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/node.hpp>

#include "markdown/parser.hpp"
#include "markdown/editor.hpp"
#include "markdown/viewer.hpp"
#include "markdown/dom_builder.hpp"
#include "markdown/theme.hpp"

namespace {

// Reports min_x=0 so that flex distributes hbox space equally,
// while passing through the full assigned width to the child.
class ZeroMinWidth : public ftxui::Node {
public:
    explicit ZeroMinWidth(ftxui::Element child)
        : ftxui::Node({std::move(child)}) {}
    void ComputeRequirement() override {
        children_[0]->ComputeRequirement();
        requirement_ = children_[0]->requirement();
        requirement_.min_x = 0;
    }
    void SetBox(ftxui::Box box) override {
        ftxui::Node::SetBox(box);
        children_[0]->SetBox(box);
    }
    void Render(ftxui::Screen& screen) override {
        children_[0]->Render(screen);
    }
};

ftxui::Element zero_min_width(ftxui::Element e) {
    return std::make_shared<ZeroMinWidth>(std::move(e));
}

// Returns the active Theme based on theme_index.
markdown::Theme const& get_theme(int index) {
    if (index == 1) return markdown::theme_high_contrast();
    if (index == 2) return markdown::theme_colorful();
    return markdown::theme_default();
}

// Minimal focusable component for wrapping renderers that need focus.
class FocusableBase : public ftxui::ComponentBase {
public:
    bool Focusable() const override { return true; }
};

// Theme toggle bar (shared across all screens).
ftxui::Element theme_bar(ftxui::Component toggle) {
    return ftxui::hbox({
        ftxui::text("  Theme: "),
        toggle->Render(),
        ftxui::filler(),
    }) | ftxui::dim;
}

// ---------------------------------------------------------------------------
// Content strings
// ---------------------------------------------------------------------------

const std::string editor_content =
    "# Hello Markdown\n\n"
    "> Focus on **important** tasks\n\n"
    "## Section One\n\n"
    "This is **bold**, *italic*, and ***bold italic***.\n\n"
    "### Todo List\n\n"
    "- Write *code*\n"
    "- Review `tests` carefully\n"
    "- Read [docs](https://example.com)\n"
    "- Deploy to **production**\n\n"
    "1. First ordered item\n"
    "2. Second ordered item\n"
    "3. Third ordered item\n\n"
    "---\n\n"
    "## Code Examples\n\n"
    "```\n#include <iostream>\n\nint main() {\n"
    "    std::cout << \"Hello!\" << std::endl;\n"
    "    return 0;\n}\n```\n\n"
    "## Links\n\n"
    "- [FTXUI](https://github.com/ArthurSonzogni/FTXUI)\n"
    "- [cmark-gfm](https://github.com/github/cmark-gfm)\n\n"
    "### Nested Lists\n\n"
    "- Fruits\n"
    "  - Apples\n"
    "  - Bananas\n"
    "- Vegetables\n"
    "  - Carrots\n"
    "  - Peas\n";

const std::string viewer_content =
    "# Markdown Viewer Demo\n\n"
    "This screen demonstrates a **standalone viewer** with scrollbar "
    "and link navigation. Use **Tab** to cycle through links, "
    "**Enter** to activate, and **Arrow keys** to scroll.\n\n"
    "## Text Formatting\n\n"
    "You can write in **bold**, *italic*, ***bold italic***, "
    "and `inline code`. Paragraphs wrap automatically at word "
    "boundaries when the terminal is too narrow.\n\n"
    "> **Tip:** Block quotes can contain *formatted text* and "
    "even `code snippets`.\n\n"
    "## Lists\n\n"
    "### Bullet Lists\n\n"
    "- First item with **bold** text\n"
    "- Second item with *italic* text\n"
    "- Third item with a [link](https://example.com/bullet)\n"
    "  - Nested item A\n"
    "  - Nested item B\n\n"
    "### Ordered Lists\n\n"
    "1. Set up the project\n"
    "2. Write the code\n"
    "3. Run the tests\n"
    "4. Deploy to production\n\n"
    "---\n\n"
    "## Code Block\n\n"
    "```\nstruct Config {\n"
    "    std::string name;\n"
    "    int value = 42;\n"
    "    bool enabled = true;\n};\n```\n\n"
    "## Links and Resources\n\n"
    "- Visit the [FTXUI repository](https://github.com/ArthurSonzogni/FTXUI) "
    "for UI components\n"
    "- Read the [CMake docs](https://cmake.org/documentation/) "
    "for build system help\n"
    "- Check out [cmark-gfm](https://github.com/github/cmark-gfm) "
    "for the Markdown parser\n"
    "- Browse [Markdown Guide](https://www.markdownguide.org) "
    "for syntax reference\n\n"
    "## More Content\n\n"
    "This section exists to make the document long enough to "
    "demonstrate scrolling behavior. The scrollbar on the right "
    "shows your position within the document.\n\n"
    "> Scroll down with **Arrow Down** or use the mouse wheel.\n\n"
    "### Section A\n\n"
    "Lorem ipsum dolor sit amet, consectetur adipiscing elit. "
    "Sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. "
    "Ut enim ad minim veniam, quis nostrud exercitation ullamco.\n\n"
    "### Section B\n\n"
    "Duis aute irure dolor in reprehenderit in voluptate velit esse "
    "cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat "
    "cupidatat non proident.\n\n"
    "---\n\n"
    "*End of document.*\n";

const std::string email_body =
    "# Sprint Review Notes\n\n"
    "Hi team, here are the notes from today's **sprint review**. "
    "Please review and add any *comments* by end of day.\n\n"
    "## Completed\n\n"
    "- Implemented **user authentication** with JWT tokens\n"
    "- Fixed `null pointer` crash in the data pipeline\n"
    "- Added *responsive layout* for mobile views\n"
    "- Migrated database to PostgreSQL 16\n"
    "- Wrote integration tests for the [API gateway](https://api.example.com)\n\n"
    "## In Progress\n\n"
    "- Performance optimization for the search engine\n"
    "- Redesigning the **dashboard** with new charts\n"
    "- Setting up [CI/CD pipeline](https://ci.example.com/builds)\n\n"
    "## Code Change Highlight\n\n"
    "The authentication middleware now validates tokens correctly:\n\n"
    "```\nbool validate_token(std::string const& token) {\n"
    "    auto decoded = jwt::decode(token);\n"
    "    auto verifier = jwt::verify()\n"
    "        .allow_algorithm(jwt::algorithm::hs256{secret})\n"
    "        .with_issuer(\"auth-service\");\n"
    "    verifier.verify(decoded);\n"
    "    return true;\n}\n```\n\n"
    "## Action Items\n\n"
    "1. Review the [PR #142](https://github.com/example/repo/pull/142) "
    "for auth changes\n"
    "2. Update the [deployment docs](https://docs.example.com/deploy) "
    "with new steps\n"
    "3. Schedule **load testing** for next Tuesday\n"
    "4. Create tickets for remaining *tech debt* items\n\n"
    "> **Reminder:** Demo day is next Friday. Please prepare your "
    "presentations by Thursday EOD.\n\n"
    "---\n\n"
    "Thanks,\n\n"
    "**Alice** | Engineering Lead\n";

} // namespace

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
    auto theme_toggle_menu = ftxui::Toggle(&theme_names, &theme_index);
    auto menu_entries = std::vector<std::string>{
        "Editor + Viewer",
        "Viewer with Scroll",
        "Email Viewer",
    };
    auto menu_descriptions = std::vector<std::string>{
        "Side-by-side editing with live preview",
        "Scrollable markdown viewer with scrollbar",
        "Simulated email with combined scroll",
    };
    auto menu_option = ftxui::MenuOption::Vertical();
    menu_option.on_enter = [&] { current_screen = menu_selected + 1; };
    menu_option.entries_option.transform = [&](ftxui::EntryState s) {
        auto num = std::to_string(s.index + 1) + ". ";
        auto title = ftxui::text(num + s.label) | ftxui::bold;
        auto desc = ftxui::text("   " + menu_descriptions[s.index]) | ftxui::dim;
        auto el = ftxui::vbox({title, desc});
        if (s.focused) el = el | ftxui::inverted;
        return el;
    };
    auto menu_comp = ftxui::Menu(&menu_entries, &menu_selected, menu_option);
    auto menu_container = ftxui::Container::Vertical({
        theme_toggle_menu, menu_comp});

    auto menu_screen = ftxui::Renderer(menu_container, [&] {
        return ftxui::vbox({
            ftxui::filler(),
            ftxui::vbox({
                ftxui::text("MarkdownFTXUI Demo") | ftxui::bold | ftxui::center,
                ftxui::text(""),
                menu_comp->Render() | ftxui::center,
                ftxui::text(""),
                ftxui::hbox({
                    ftxui::text("Theme: "),
                    theme_toggle_menu->Render(),
                }) | ftxui::center,
                ftxui::text(""),
                ftxui::text("Enter to select") | ftxui::dim | ftxui::center,
            }) | ftxui::center,
            ftxui::filler(),
        });
    });

    // =====================================================================
    // Screen 1: Editor + Viewer
    // =====================================================================
    markdown::Editor editor;
    editor.set_content(editor_content);
    auto editor_comp = editor.component();

    markdown::Viewer viewer1(markdown::make_cmark_parser());
    auto viewer1_comp = viewer1.component();

    auto theme_toggle_s1 = ftxui::Toggle(&theme_names, &theme_index);
    auto s1_container = ftxui::Container::Vertical({
        theme_toggle_s1, editor_comp, viewer1_comp});

    auto screen1 = ftxui::Renderer(s1_container, [&] {
        auto const& theme = get_theme(theme_index);
        editor.set_theme(theme);
        viewer1.set_theme(theme);
        viewer1.set_content(editor.content());
        viewer1.show_scrollbar(true);

        if (!viewer1.active()) {
            float scroll_ratio = 0.0f;
            if (editor.total_lines() > 1) {
                scroll_ratio = static_cast<float>(editor.cursor_line() - 1) /
                               static_cast<float>(editor.total_lines() - 1);
            }
            viewer1.set_scroll(scroll_ratio);
        }

        auto active_border = ftxui::borderStyled(
            ftxui::BorderStyle::DOUBLE, ftxui::Color::White);
        auto focused_border = ftxui::borderStyled(ftxui::Color::White);
        auto dim_border = ftxui::borderStyled(ftxui::Color::GrayDark);

        auto ed_border = !editor_comp->Focused() ? dim_border
            : (editor.active() ? active_border : focused_border);
        auto vw_border = !viewer1_comp->Focused() ? dim_border
            : (viewer1.active() ? active_border : focused_border);

        auto editor_pane = zero_min_width(
            ftxui::vbox({
                ftxui::text(" Editor ") | ftxui::bold | ftxui::center,
                ftxui::separator(),
                editor_comp->Render() | ftxui::flex | ftxui::frame,
            }) | ed_border
        ) | ftxui::flex;

        auto viewer_pane = zero_min_width(
            ftxui::vbox({
                ftxui::text(" Viewer ") | ftxui::bold | ftxui::center,
                ftxui::separator(),
                viewer1_comp->Render(),
            }) | vw_border
        ) | ftxui::flex;

        return ftxui::vbox({
            theme_bar(theme_toggle_s1),
            ftxui::hbox({editor_pane, viewer_pane}) | ftxui::flex,
            ftxui::text(" Enter:select  Esc:back ") | ftxui::dim,
        });
    });

    // =====================================================================
    // Screen 2: Viewer with Scrollbar
    // =====================================================================
    markdown::Viewer viewer2(markdown::make_cmark_parser());
    viewer2.set_content(viewer_content);
    viewer2.show_scrollbar(true);
    std::string s2_link_url;
    float s2_scroll = 0.0f;
    viewer2.on_link_click([&](std::string const& url, markdown::LinkEvent) {
        s2_link_url = url;
    });
    auto viewer2_comp = viewer2.component();

    // Viewer is the sole component — no theme toggle in the focus chain.
    // Left/Right cycle themes. Up/Down scroll when not in links mode.
    auto s2_with_keys = ftxui::CatchEvent(viewer2_comp, [&](ftxui::Event ev) {
        // Tab/Shift+Tab: auto-activate viewer for link cycling
        if (!viewer2.active() &&
            (ev == ftxui::Event::Tab || ev == ftxui::Event::TabReverse)) {
            viewer2.set_active(true);
            return false; // let ViewerWrap handle the Tab
        }
        // Left/Right cycle theme (always available)
        if (ev == ftxui::Event::ArrowLeft) {
            theme_index = (theme_index + 2) % 3;
            return true;
        }
        if (ev == ftxui::Event::ArrowRight) {
            theme_index = (theme_index + 1) % 3;
            return true;
        }
        // Up/Down scroll when not in links mode
        if (!viewer2.active()) {
            if (ev == ftxui::Event::ArrowDown) {
                s2_scroll = std::min(1.0f, s2_scroll + 0.05f);
                viewer2.set_scroll(s2_scroll);
                return true;
            }
            if (ev == ftxui::Event::ArrowUp) {
                s2_scroll = std::max(0.0f, s2_scroll - 0.05f);
                viewer2.set_scroll(s2_scroll);
                return true;
            }
        }
        return false;
    });

    auto screen2 = ftxui::Renderer(s2_with_keys, [&] {
        viewer2.set_theme(get_theme(theme_index));

        ftxui::Elements status_parts;
        if (!s2_link_url.empty()) {
            status_parts.push_back(
                ftxui::text(" " + s2_link_url + " ") | ftxui::dim
                    | ftxui::underlined);
        }
        status_parts.push_back(ftxui::filler());
        status_parts.push_back(
            ftxui::text(" Up/Down:scroll  Left/Right:theme  Enter:links  Esc:back ")
                | ftxui::dim);

        return ftxui::vbox({
            ftxui::hbox({
                ftxui::text("  Theme: ") | ftxui::dim,
                ftxui::text(theme_names[theme_index]) | ftxui::bold,
                ftxui::filler(),
            }),
            ftxui::vbox({
                ftxui::text(" Markdown Viewer ") | ftxui::bold | ftxui::center,
                ftxui::separator(),
                viewer2_comp->Render(),
            }) | ftxui::border | ftxui::flex,
            ftxui::hbox(std::move(status_parts)),
        });
    });

    // =====================================================================
    // Screen 3: Email Viewer (combined scroll)
    // =====================================================================
    auto email_parser = markdown::make_cmark_parser();
    markdown::DomBuilder email_builder;
    auto email_ast = email_parser->parse(email_body);
    float email_scroll = 0.0f;
    int email_focused_link = -1;
    std::string email_link_url;

    auto email_renderer = ftxui::Renderer(ftxui::Make<FocusableBase>(), [&] {
        auto const& theme = get_theme(theme_index);
        auto body = email_builder.build(email_ast, email_focused_link, theme);

        auto email_el = ftxui::vbox({
            ftxui::hbox({
                ftxui::text("From: ") | ftxui::bold,
                ftxui::text("Alice <alice@example.com>"),
            }),
            ftxui::hbox({
                ftxui::text("To: ") | ftxui::bold,
                ftxui::text("team@example.com"),
            }),
            ftxui::hbox({
                ftxui::text("Subject: ") | ftxui::bold,
                ftxui::text("Sprint Review Notes - Week 42"),
            }),
            ftxui::hbox({
                ftxui::text("Date: ") | ftxui::bold,
                ftxui::text("Fri, 7 Feb 2026 15:30:00 +0200"),
            }),
            ftxui::separator(),
            body,
        });

        return email_el
            | ftxui::focusPositionRelative(0.0f, email_scroll)
            | ftxui::vscroll_indicator
            | ftxui::yframe
            | ftxui::flex;
    });

    // Email is the sole component. Tab cycles links. Up/Down scroll.
    // Left/Right cycle themes.
    auto email_with_keys = ftxui::CatchEvent(email_renderer,
                                              [&](ftxui::Event ev) {
        // Tab cycles forward through links
        if (ev == ftxui::Event::Tab) {
            int count = static_cast<int>(
                email_builder.link_targets().size());
            if (count > 0) {
                email_focused_link = (email_focused_link + 1) % count;
                auto it = email_builder.link_targets().begin();
                std::advance(it, email_focused_link);
                email_link_url = it->url;
            }
            return true;
        }
        // Shift+Tab cycles backward
        if (ev == ftxui::Event::TabReverse) {
            int count = static_cast<int>(
                email_builder.link_targets().size());
            if (count > 0) {
                email_focused_link =
                    (email_focused_link - 1 + count) % count;
                auto it = email_builder.link_targets().begin();
                std::advance(it, email_focused_link);
                email_link_url = it->url;
            }
            return true;
        }
        if (ev == ftxui::Event::ArrowLeft) {
            theme_index = (theme_index + 2) % 3;
            return true;
        }
        if (ev == ftxui::Event::ArrowRight) {
            theme_index = (theme_index + 1) % 3;
            return true;
        }
        if (ev == ftxui::Event::ArrowDown) {
            email_scroll = std::min(1.0f, email_scroll + 0.05f);
            return true;
        }
        if (ev == ftxui::Event::ArrowUp) {
            email_scroll = std::max(0.0f, email_scroll - 0.05f);
            return true;
        }
        return false;
    });

    auto screen3 = ftxui::Renderer(email_with_keys, [&] {
        return ftxui::vbox({
            ftxui::hbox({
                ftxui::text("  Theme: ") | ftxui::dim,
                ftxui::text(theme_names[theme_index]) | ftxui::bold,
                ftxui::filler(),
            }),
            email_with_keys->Render()
                | ftxui::border | ftxui::flex,
            ftxui::hbox({
                email_link_url.empty()
                    ? ftxui::text("")
                    : ftxui::text(" " + email_link_url + " ")
                        | ftxui::dim | ftxui::underlined,
                ftxui::filler(),
                ftxui::text(" Tab:links  Up/Down:scroll  Left/Right:theme  Esc:back ")
                    | ftxui::dim,
            }),
        });
    });

    // =====================================================================
    // Root: Tab switch + Esc handler
    // =====================================================================
    auto tab = ftxui::Container::Tab(
        {menu_screen, screen1, screen2, screen3}, &current_screen);

    auto root = ftxui::CatchEvent(tab, [&](ftxui::Event event) {
        // Esc: return to menu from sub-demos (unless editor/viewer is active)
        if (event == ftxui::Event::Escape) {
            if (current_screen == 0) {
                screen.Exit();
                return true;
            }
            // Screen 1: if editor or viewer is active, let them handle Esc first
            if (current_screen == 1 && (editor.active() || viewer1.active())) {
                return false;
            }
            // Screen 2: if viewer is active, let it handle Esc first
            if (current_screen == 2 && viewer2.active()) {
                return false;
            }
            current_screen = 0;
            return true;
        }
        return false;
    });

    screen.Loop(root);
    return 0;
}
