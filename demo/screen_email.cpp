#include "screens.hpp"
#include "common.hpp"

#include <algorithm>
#include <iterator>

#include <ftxui/component/event.hpp>

#include "markdown/dom_builder.hpp"
#include "markdown/parser.hpp"

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

    auto parser = markdown::make_cmark_parser();
    auto builder = std::make_shared<markdown::DomBuilder>();
    auto ast = std::make_shared<markdown::MarkdownAST>(
        parser->parse(email_body));

    auto scroll = std::make_shared<float>(0.0f);
    auto focused_link = std::make_shared<int>(-1);
    auto link_url = std::make_shared<std::string>();

    auto renderer = ftxui::Renderer(
        ftxui::Make<demo::FocusableBase>(),
        [=, &theme_index] {
            auto const& theme = demo::get_theme(theme_index);
            auto body = builder->build(*ast, *focused_link, theme);

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
                | ftxui::focusPositionRelative(0.0f, *scroll)
                | ftxui::vscroll_indicator
                | ftxui::yframe
                | ftxui::flex;
        });

    auto with_keys = ftxui::CatchEvent(renderer,
        [=, &theme_index](ftxui::Event ev) {
            if (ev == ftxui::Event::Tab) {
                int count = static_cast<int>(
                    builder->link_targets().size());
                if (count > 0) {
                    *focused_link = (*focused_link + 1) % count;
                    auto it = builder->link_targets().begin();
                    std::advance(it, *focused_link);
                    *link_url = it->url;
                }
                return true;
            }
            if (ev == ftxui::Event::TabReverse) {
                int count = static_cast<int>(
                    builder->link_targets().size());
                if (count > 0) {
                    *focused_link =
                        (*focused_link - 1 + count) % count;
                    auto it = builder->link_targets().begin();
                    std::advance(it, *focused_link);
                    *link_url = it->url;
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
                *scroll = std::min(1.0f, *scroll + 0.05f);
                return true;
            }
            if (ev == ftxui::Event::ArrowUp) {
                *scroll = std::max(0.0f, *scroll - 0.05f);
                return true;
            }
            return false;
        });

    auto screen = ftxui::Renderer(with_keys,
        [=, &theme_index, &theme_names] {
            return ftxui::vbox({
                ftxui::hbox({
                    ftxui::text("  Theme: ") | ftxui::dim,
                    ftxui::text(theme_names[theme_index]) | ftxui::bold,
                    ftxui::filler(),
                }),
                with_keys->Render()
                    | ftxui::border | ftxui::flex,
                ftxui::hbox({
                    link_url->empty()
                        ? ftxui::text("")
                        : ftxui::text(" " + *link_url + " ")
                            | ftxui::dim | ftxui::underlined,
                    ftxui::filler(),
                    ftxui::text(
                        " Tab:links  Up/Down:scroll  Left/Right:theme  Esc:back ")
                        | ftxui::dim,
                }),
            });
        });

    return ftxui::CatchEvent(screen,
        [&current_screen](ftxui::Event ev) {
            if (ev == ftxui::Event::Escape) {
                current_screen = 0;
                return true;
            }
            return false;
        });
}
