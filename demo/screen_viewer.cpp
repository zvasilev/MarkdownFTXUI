#include "screens.hpp"
#include "common.hpp"

#include <algorithm>

#include <ftxui/component/event.hpp>
#include <ftxui/component/mouse.hpp>

#include "markdown/parser.hpp"
#include "markdown/viewer.hpp"

namespace {

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
    "```cpp\nstruct Config {\n"
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

} // namespace

ftxui::Component make_viewer_screen(
    int& current_screen, int& theme_index,
    std::vector<std::string>& theme_names) {

    auto viewer = std::make_shared<markdown::Viewer>(
        markdown::make_cmark_parser());
    viewer->set_content(viewer_content);
    viewer->show_scrollbar(true);

    auto link_url = std::make_shared<std::string>();
    auto scroll = std::make_shared<float>(0.0f);

    viewer->on_link_click(
        [link_url](std::string const& url, markdown::LinkEvent) {
            *link_url = url;
        });
    auto viewer_comp = viewer->component();

    auto with_keys = ftxui::CatchEvent(viewer_comp,
        [=, &theme_index](ftxui::Event ev) {
            // Tab/Shift+Tab: auto-activate viewer for link cycling
            if (!viewer->active() &&
                (ev == ftxui::Event::Tab ||
                 ev == ftxui::Event::TabReverse)) {
                viewer->set_active(true);
                return false;
            }
            if (ev == ftxui::Event::ArrowLeft) {
                theme_index = (theme_index + 2) % 3;
                return true;
            }
            if (ev == ftxui::Event::ArrowRight) {
                theme_index = (theme_index + 1) % 3;
                return true;
            }
            if (ev.is_mouse()) {
                auto& m = ev.mouse();
                if (m.button == ftxui::Mouse::WheelUp) {
                    *scroll = std::max(0.0f, *scroll - 0.05f);
                    viewer->set_scroll(*scroll);
                    return true;
                }
                if (m.button == ftxui::Mouse::WheelDown) {
                    *scroll = std::min(1.0f, *scroll + 0.05f);
                    viewer->set_scroll(*scroll);
                    return true;
                }
            }
            if (!viewer->active()) {
                if (ev == ftxui::Event::ArrowDown) {
                    *scroll = std::min(1.0f, *scroll + 0.05f);
                    viewer->set_scroll(*scroll);
                    return true;
                }
                if (ev == ftxui::Event::ArrowUp) {
                    *scroll = std::max(0.0f, *scroll - 0.05f);
                    viewer->set_scroll(*scroll);
                    return true;
                }
                if (ev == ftxui::Event::PageDown) {
                    *scroll = std::min(1.0f, *scroll + 0.3f);
                    viewer->set_scroll(*scroll);
                    return true;
                }
                if (ev == ftxui::Event::PageUp) {
                    *scroll = std::max(0.0f, *scroll - 0.3f);
                    viewer->set_scroll(*scroll);
                    return true;
                }
            }
            return false;
        });

    auto screen = ftxui::Renderer(with_keys,
        [=, &theme_index, &theme_names] {
            viewer->set_theme(demo::get_theme(theme_index));

            ftxui::Elements status_parts;
            if (!link_url->empty()) {
                status_parts.push_back(
                    ftxui::text(" " + *link_url + " ") | ftxui::dim
                        | ftxui::underlined);
            }
            status_parts.push_back(ftxui::filler());
            status_parts.push_back(
                ftxui::text(
                    " Up/Down/PgUp/PgDn:scroll  Left/Right:theme  Enter:links  Esc:back")
                    | ftxui::dim);

            return ftxui::vbox({
                ftxui::hbox({
                    ftxui::text("  Theme: ") | ftxui::dim,
                    ftxui::text(theme_names[theme_index]) | ftxui::bold,
                    ftxui::filler(),
                }),
                ftxui::vbox({
                    ftxui::text(" Markdown Viewer ") | ftxui::bold
                        | ftxui::center,
                    ftxui::separator(),
                    viewer_comp->Render(),
                }) | ftxui::border | ftxui::flex,
                ftxui::hbox(std::move(status_parts)),
            });
        });

    return ftxui::CatchEvent(screen,
        [=, &current_screen](ftxui::Event ev) {
            if (ev == ftxui::Event::Escape) {
                if (viewer->active()) return false;
                current_screen = 0;
                return true;
            }
            return false;
        });
}
