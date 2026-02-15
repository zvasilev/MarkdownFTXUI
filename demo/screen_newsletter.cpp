#include "screens.hpp"
#include "common.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include <ftxui/component/event.hpp>

#include "markdown/parser.hpp"
#include "markdown/viewer.hpp"

namespace {

namespace fs = std::filesystem;

struct SnippetList {
    std::vector<fs::path> files;
    int current = 0;

    void scan(std::string const& dir) {
        files.clear();
        current = 0;
        if (!fs::is_directory(dir)) return;
        for (auto const& entry : fs::directory_iterator(dir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".md") {
                files.push_back(entry.path());
            }
        }
        std::sort(files.begin(), files.end());
    }

    std::string read_current() const {
        if (files.empty()) return "*(no .md files found in snippets directory)*\n";
        std::ifstream f(files[current]);
        if (!f.is_open()) return "*(could not open " + files[current].filename().string() + ")*\n";
        std::ostringstream ss;
        ss << f.rdbuf();
        return ss.str();
    }

    std::string current_name() const {
        if (files.empty()) return "(none)";
        return files[current].filename().string();
    }

    int count() const { return static_cast<int>(files.size()); }

    void next() { if (!files.empty()) current = (current + 1) % count(); }
    void prev() { if (!files.empty()) current = (current + count() - 1) % count(); }
};

} // namespace

ftxui::Component make_newsletter_screen(
    int& current_screen, int& theme_index,
    std::vector<std::string>& theme_names) {

    auto snippets = std::make_shared<SnippetList>();
#ifdef SNIPPET_DIR
    snippets->scan(SNIPPET_DIR);
#endif

    auto viewer = std::make_shared<markdown::Viewer>(
        markdown::make_cmark_parser());
    viewer->set_content(snippets->read_current());
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
                    ftxui::text(snippets->current_name() + " ("
                        + std::to_string(snippets->current + 1) + "/"
                        + std::to_string(snippets->count()) + ")  ") | ftxui::dim,
                }),
                combined | ftxui::border,
                ftxui::hbox({
                    status_text->empty()
                        ? ftxui::text("")
                        : ftxui::text(" " + *status_text + " ")
                            | ftxui::dim | ftxui::underlined,
                    ftxui::filler(),
                    ftxui::text(
                        " n/p:snippet  Tab:cycle  Scroll:Arrows/PgUp/PgDn/Home/End  Theme:Left/Right  Esc:back")
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
            if (ev.is_character() && (ev.character() == "n" || ev.character() == "N")) {
                snippets->next();
                viewer->set_content(snippets->read_current());
                return true;
            }
            if (ev.is_character() && (ev.character() == "p" || ev.character() == "P")) {
                snippets->prev();
                viewer->set_content(snippets->read_current());
                return true;
            }
            return false;
        });
}
