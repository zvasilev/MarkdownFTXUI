#include "screens.hpp"
#include "common.hpp"

#include <ftxui/component/event.hpp>

#include "markdown/editor.hpp"
#include "markdown/parser.hpp"
#include "markdown/viewer.hpp"

namespace {

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

} // namespace

ftxui::Component make_editor_screen(
    int& current_screen, int& theme_index,
    std::vector<std::string>& theme_names) {

    auto editor = std::make_shared<markdown::Editor>();
    editor->set_content(editor_content);
    auto editor_comp = editor->component();

    auto viewer = std::make_shared<markdown::Viewer>(
        markdown::make_cmark_parser());
    auto viewer_comp = viewer->component();

    auto theme_toggle = ftxui::Toggle(&theme_names, &theme_index);
    auto container = ftxui::Container::Vertical({
        theme_toggle, editor_comp, viewer_comp});

    auto screen = ftxui::Renderer(container, [=, &theme_index] {
        auto const& theme = demo::get_theme(theme_index);
        editor->set_theme(theme);
        viewer->set_theme(theme);
        viewer->set_content(editor->content());
        viewer->show_scrollbar(true);

        if (!viewer->active()) {
            float scroll_ratio = 0.0f;
            if (editor->total_lines() > 1) {
                scroll_ratio =
                    static_cast<float>(editor->cursor_line() - 1) /
                    static_cast<float>(editor->total_lines() - 1);
            }
            viewer->set_scroll(scroll_ratio);
        }

        auto active_border = ftxui::borderStyled(
            ftxui::BorderStyle::DOUBLE, ftxui::Color::White);
        auto focused_border = ftxui::borderStyled(ftxui::Color::White);
        auto dim_border = ftxui::borderStyled(ftxui::Color::GrayDark);

        auto ed_border = !editor_comp->Focused() ? dim_border
            : (editor->active() ? active_border : focused_border);
        auto vw_border = !viewer_comp->Focused() ? dim_border
            : (viewer->active() ? active_border : focused_border);

        auto editor_pane = demo::zero_min_width(
            ftxui::vbox({
                ftxui::text(" Editor ") | ftxui::bold | ftxui::center,
                ftxui::separator(),
                editor_comp->Render() | ftxui::flex | ftxui::frame,
            }) | ed_border
        ) | ftxui::flex;

        auto viewer_pane = demo::zero_min_width(
            ftxui::vbox({
                ftxui::text(" Viewer ") | ftxui::bold | ftxui::center,
                ftxui::separator(),
                viewer_comp->Render(),
            }) | vw_border
        ) | ftxui::flex;

        return ftxui::vbox({
            demo::theme_bar(theme_toggle),
            ftxui::hbox({editor_pane, viewer_pane}) | ftxui::flex,
            ftxui::text(" Enter:select  Esc:back ") | ftxui::dim,
        });
    });

    return ftxui::CatchEvent(screen,
        [=, &current_screen](ftxui::Event ev) {
            if (ev == ftxui::Event::Escape) {
                if (editor->active() || viewer->active()) return false;
                current_screen = 0;
                return true;
            }
            return false;
        });
}
