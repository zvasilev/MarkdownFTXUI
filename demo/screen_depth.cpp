#include "screens.hpp"
#include "common.hpp"

#include <string>

#include <ftxui/component/event.hpp>

#include "markdown/parser.hpp"
#include "markdown/viewer.hpp"

namespace {

// Build markdown with blockquotes nested to the given depth.
std::string make_deep_quotes(int max_depth) {
    std::string doc;

    doc += "# Depth Limit Fallback Demo\n\n";
    doc += "Below is a **deeply nested blockquote chain** simulating a "
           "forwarded email thread. Use **+/-** to change the max indent "
           "level and watch the `\u2502` bars appear/disappear in "
           "real-time.\n\n";
    doc += "---\n\n";

    for (int depth = 1; depth <= max_depth; ++depth) {
        std::string prefix(depth, '>');
        prefix += ' ';

        doc += prefix + "**[Level " + std::to_string(depth) +
               "](https://example.com/" + std::to_string(depth) +
               ")** \u2014 ";
        doc += "*formatted* `inline code` and [a link](https://example.com)\n";
        doc += prefix + "\n";
    }

    doc += "\n---\n\n";
    doc += "*Content is preserved at every level \u2014 only the "
           "`\u2502` indent bars stop at the configured limit.*\n";

    return doc;
}

const std::string deep_content = make_deep_quotes(55);

} // namespace

ftxui::Component make_depth_screen(
    int& current_screen, int& theme_index,
    std::vector<std::string>& theme_names) {

    auto viewer = std::make_shared<markdown::Viewer>(
        markdown::make_cmark_parser());
    viewer->set_content(deep_content);
    viewer->show_scrollbar(true);
    viewer->set_max_quote_depth(10);

    auto viewer_comp = viewer->component();

    auto with_keys = ftxui::CatchEvent(viewer_comp,
        [=, &theme_index](ftxui::Event ev) {
            // Tab: auto-activate for link cycling
            if (!viewer->active() &&
                (ev == ftxui::Event::Tab ||
                 ev == ftxui::Event::TabReverse)) {
                viewer->set_active(true);
                return false;
            }
            if (demo::handle_theme_cycling(theme_index, 3, ev)) return true;
            // +/- to adjust max_quote_depth
            if (ev == ftxui::Event::Character('+') ||
                ev == ftxui::Event::Character('=')) {
                viewer->set_max_quote_depth(
                    std::min(40, viewer->max_quote_depth() + 1));
                return true;
            }
            if (ev == ftxui::Event::Character('-')) {
                viewer->set_max_quote_depth(
                    std::max(1, viewer->max_quote_depth() - 1));
                return true;
            }
            if (demo::handle_inactive_scroll(*viewer, ev, 0.03f)) return true;
            return false;
        });

    auto screen = ftxui::Renderer(with_keys,
        [=, &theme_index, &theme_names] {
            viewer->set_theme(demo::get_theme(theme_index));

            auto depth_str = std::to_string(viewer->max_quote_depth());

            return ftxui::vbox({
                ftxui::hbox({
                    ftxui::text("  Theme: ") | ftxui::dim,
                    ftxui::text(theme_names[theme_index]) | ftxui::bold,
                    ftxui::filler(),
                    ftxui::text("Max indent: ") | ftxui::dim,
                    ftxui::text(depth_str) | ftxui::bold,
                    ftxui::text("  (+/- to change) ") | ftxui::dim,
                }),
                ftxui::vbox({
                    ftxui::text(" Depth Fallback Demo ") | ftxui::bold
                        | ftxui::center,
                    ftxui::separator(),
                    viewer_comp->Render() | ftxui::flex,
                }) | ftxui::border | ftxui::flex,
                ftxui::hbox({
                    ftxui::text(
                        " Scroll:Arrows/PgUp/PgDn/Home/End  Theme:Left/Right  "
                        "+/-:indent  Tab:links  Esc:back") | ftxui::dim,
                }),
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
