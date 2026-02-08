#include "screens.hpp"
#include "common.hpp"

#include <fstream>
#include <memory>
#include <sstream>

#include <ftxui/component/event.hpp>

#include "markdown/parser.hpp"
#include "markdown/viewer.hpp"

namespace {

std::string read_snippet() {
#ifdef SNIPPET_FILE
    std::ifstream f(SNIPPET_FILE);
    if (f.is_open()) {
        std::ostringstream ss;
        ss << f.rdbuf();
        return ss.str();
    }
#endif
    return "*(newsletter file not found)*\n";
}

} // namespace

ftxui::Component make_newsletter_screen(
    int& current_screen, int& theme_index,
    std::vector<std::string>& theme_names) {

    auto viewer = std::make_shared<markdown::Viewer>(
        markdown::make_cmark_parser());
    viewer->set_content(read_snippet());
    viewer->set_embed(true);

    viewer->add_focusable("From", "Apple Developer <developer@insideapple.apple.com>");
    viewer->add_focusable("To", "you@example.com");
    viewer->add_focusable("Subject", "Meet with Apple this fall and winter");
    viewer->add_focusable("Date", "Mon, 10 Nov 2025 09:00:00 -0800");

    auto status_text = std::make_shared<std::string>();

    viewer->on_link_click(
        [status_text](std::string const& value, markdown::LinkEvent) {
            *status_text = value;
        });

    auto viewer_comp = viewer->component();

    auto screen = ftxui::Renderer(viewer_comp,
        [=, &theme_index, &theme_names] {
            viewer->set_theme(demo::get_theme(theme_index));

            auto const& ext = viewer->externals();
            ftxui::Elements header_rows;
            for (int i = 0; i < static_cast<int>(ext.size()); ++i) {
                auto content = ftxui::hbox({
                    ftxui::text(ext[i].label + ": ") | ftxui::bold,
                    ftxui::text(ext[i].value),
                });
                if (viewer->is_external_focused(i)) {
                    header_rows.push_back(ftxui::hbox({
                        ftxui::text("["),
                        content,
                        ftxui::text("]"),
                    }));
                } else {
                    header_rows.push_back(ftxui::hbox({
                        ftxui::text(" "),
                        content,
                        ftxui::text(" "),
                    }));
                }
            }
            header_rows.push_back(ftxui::separator());
            header_rows.push_back(viewer_comp->Render());

            auto combined = ftxui::vbox(std::move(header_rows))
                | ftxui::vscroll_indicator;
            if (!viewer->is_link_focused()) {
                combined = markdown::direct_scroll(std::move(combined),
                                               viewer->scroll());
            } else {
                combined = combined | ftxui::yframe;
            }
            combined = combined | ftxui::flex;

            return ftxui::vbox({
                ftxui::hbox({
                    ftxui::text("  Theme: ") | ftxui::dim,
                    ftxui::text(theme_names[theme_index]) | ftxui::bold,
                    ftxui::filler(),
                }),
                combined | ftxui::border,
                ftxui::hbox({
                    status_text->empty()
                        ? ftxui::text("")
                        : ftxui::text(" " + *status_text + " ")
                            | ftxui::dim | ftxui::underlined,
                    ftxui::filler(),
                    ftxui::text(
                        " Tab:cycle  Up/Down:scroll  Left/Right:theme  Esc:back ")
                        | ftxui::dim,
                }),
            });
        });

    return ftxui::CatchEvent(screen,
        [=, &current_screen, &theme_index](ftxui::Event ev) {
            if (ev == ftxui::Event::ArrowLeft) {
                theme_index = (theme_index + 2) % 3;
                return true;
            }
            if (ev == ftxui::Event::ArrowRight) {
                theme_index = (theme_index + 1) % 3;
                return true;
            }
            if (ev == ftxui::Event::Escape) {
                current_screen = 0;
                return true;
            }
            return false;
        });
}
