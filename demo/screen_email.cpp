#include "screens.hpp"
#include "common.hpp"

#include <algorithm>
#include <memory>
#include <string>
#include <vector>

#include <ftxui/component/event.hpp>

#include "markdown/parser.hpp"
#include "markdown/scroll_frame.hpp"
#include "markdown/viewer.hpp"

namespace {

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

struct HeaderField {
    std::string label;
    std::string value;
};

} // namespace

ftxui::Component make_email_screen(
    int& current_screen, int& theme_index,
    std::vector<std::string>& theme_names) {

    auto viewer = std::make_shared<markdown::Viewer>(
        markdown::make_cmark_parser());
    viewer->set_content(email_body);
    viewer->set_embed(true);

    auto scroll_info = std::make_shared<markdown::ScrollInfo>();
    viewer->set_external_scroll_info(scroll_info.get());

    auto headers = std::make_shared<std::vector<HeaderField>>(
        std::vector<HeaderField>{
            {"From", "Alice <alice@example.com>"},
            {"To", "team@example.com"},
            {"Subject", "Sprint Review Notes - Week 42"},
            {"Date", "Fri, 7 Feb 2026 15:30:00 +0200"},
        });
    int num_headers = static_cast<int>(headers->size());

    auto header_focus = std::make_shared<int>(-1);
    auto status_text = std::make_shared<std::string>();

    viewer->on_link_click(
        [status_text](std::string const& value, markdown::LinkEvent) {
            *status_text = value;
        });

    viewer->on_tab_exit(
        [header_focus, headers, status_text, viewer](int direction) {
            if (direction > 0) {
                *header_focus = 0;
            } else {
                *header_focus = static_cast<int>(headers->size()) - 1;
            }
            *status_text = (*headers)[*header_focus].value;
            // Scroll to top so headers are visible (they scroll with body)
            viewer->set_scroll(0.0f);
        });

    auto viewer_comp = viewer->component();

    auto with_keys = ftxui::CatchEvent(viewer_comp,
        [=, &theme_index](ftxui::Event ev) {
            if (ev == viewer->keys().next || ev == viewer->keys().prev) {
                if (viewer->active()) {
                    return false;  // let viewer handle link cycling
                }
                int dir = (ev == viewer->keys().next) ? 1 : -1;
                if (*header_focus < 0) {
                    *header_focus = (dir > 0) ? 0 : num_headers - 1;
                } else {
                    int next = *header_focus + dir;
                    if (next >= num_headers) {
                        *header_focus = -1;
                        if (!viewer->enter_focus(+1)) {
                            *header_focus = 0;  // no links, wrap
                        }
                    } else if (next < 0) {
                        *header_focus = -1;
                        if (!viewer->enter_focus(-1)) {
                            *header_focus = num_headers - 1;
                        }
                    } else {
                        *header_focus = next;
                    }
                }
                if (*header_focus >= 0) {
                    *status_text = (*headers)[*header_focus].value;
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
            // Scroll when viewer is not active (header browsing)
            if (!viewer->active()) {
                constexpr float kStep = 0.05f;
                auto adjust = [&](float delta) {
                    float s = std::clamp(viewer->scroll() + delta, 0.0f, 1.0f);
                    viewer->set_scroll(s);
                    return true;
                };
                if (ev == ftxui::Event::ArrowDown) return adjust(kStep);
                if (ev == ftxui::Event::ArrowUp) return adjust(-kStep);
                if (ev == ftxui::Event::PageDown) return adjust(0.3f);
                if (ev == ftxui::Event::PageUp) return adjust(-0.3f);
                if (ev == ftxui::Event::Home)
                    return adjust(-viewer->scroll());
                if (ev == ftxui::Event::End)
                    return adjust(1.0f - viewer->scroll());
            }
            return false;
        });

    auto screen = ftxui::Renderer(with_keys,
        [=, &theme_index, &theme_names] {
            viewer->set_theme(demo::get_theme(theme_index));

            ftxui::Elements header_rows;
            for (int i = 0; i < num_headers; ++i) {
                auto content = ftxui::hbox({
                    ftxui::text((*headers)[i].label + ": ") | ftxui::bold,
                    ftxui::text((*headers)[i].value),
                });
                if (*header_focus == i) {
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

            // Headers + body in a single scroll frame
            auto combined = ftxui::vbox({
                ftxui::vbox(std::move(header_rows)),
                ftxui::separator(),
                viewer_comp->Render(),
            });
            combined = combined | ftxui::vscroll_indicator;
            combined = markdown::direct_scroll(
                std::move(combined), viewer->scroll(), scroll_info.get());

            return ftxui::vbox({
                ftxui::hbox({
                    ftxui::text("  Theme: ") | ftxui::dim,
                    ftxui::text(theme_names[theme_index]) | ftxui::bold,
                    ftxui::filler(),
                }),
                combined | ftxui::border | ftxui::flex,
                ftxui::hbox({
                    status_text->empty()
                        ? ftxui::text("")
                        : ftxui::text(" " + *status_text + " ")
                            | ftxui::dim | ftxui::underlined,
                    ftxui::filler(),
                    ftxui::text(
                        " Tab:cycle  Scroll:Arrows/PgUp/PgDn/Home/End  Theme:Left/Right  Esc:back")
                        | ftxui::dim,
                }),
            });
        });

    return ftxui::CatchEvent(screen,
        [=, &current_screen](ftxui::Event ev) {
            if (ev == viewer->keys().deactivate) {
                if (viewer->active()) return false;
                current_screen = 0;
                return true;
            }
            return false;
        });
}
