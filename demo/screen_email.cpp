#include "screens.hpp"
#include "common.hpp"

#include <memory>

#include <ftxui/component/event.hpp>

#include "markdown/parser.hpp"
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

} // namespace

ftxui::Component make_email_screen(
    int& current_screen, int& theme_index,
    std::vector<std::string>& theme_names) {

    auto viewer = std::make_shared<markdown::Viewer>(
        markdown::make_cmark_parser());
    viewer->set_content(email_body);
    viewer->set_embed(true);  // caller handles framing for combined scroll

    // Register external focusable items (headers in the Tab ring)
    viewer->add_focusable("From", "Alice <alice@example.com>");
    viewer->add_focusable("To", "team@example.com");
    viewer->add_focusable("Subject", "Sprint Review Notes - Week 42");
    viewer->add_focusable("Date", "Fri, 7 Feb 2026 15:30:00 +0200");

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

            // Wrap headers + body in a single scrollable frame.
            // Use direct_scroll (not yframe) for linear scroll offset.
            auto combined = ftxui::vbox(std::move(header_rows))
                | ftxui::vscroll_indicator;
            if (!viewer->is_link_focused()) {
                combined = demo::direct_scroll(std::move(combined),
                                               viewer->scroll());
            } else {
                // Link focused: yframe scrolls to the ftxui::focus element
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

    // Theme cycling + Esc → menu
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
