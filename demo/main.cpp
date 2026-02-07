#include <string>

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/node.hpp>

#include "markdown/parser.hpp"
#include "markdown/editor.hpp"
#include "markdown/viewer.hpp"

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
} // namespace

int main() {
    auto screen = ftxui::ScreenInteractive::Fullscreen();

    // --- State ---
    bool show_scrollbar = true;

    // --- Editor ---
    markdown::Editor editor;
    editor.set_content(
        "# Hello Markdown\n\n"
        "> Focus on **important** tasks\n\n"
        "## Section One\n\n"
        "This is **bold**, *italic*, and ***bold italic***.\n\n"
        "### Todo List\n\n"
        "- Write *code*\n"
        "- Review `tests` carefully\n"
        "- Read [docs](https://example.com)\n"
        "- Deploy to **production**\n"
        "- Monitor *metrics* dashboard\n\n"
        "1. First ordered item\n"
        "2. Second ordered item\n"
        "3. Third ordered item\n\n"
        "---\n\n"
        "## Code Examples\n\n"
        "Here is a simple C++ program:\n\n"
        "```\n#include <iostream>\n\nint main() {\n"
        "    std::cout << \"Hello, world!\" << std::endl;\n"
        "    return 0;\n}\n```\n\n"
        "And another snippet:\n\n"
        "```\nfor (int i = 0; i < 10; ++i) {\n"
        "    process(i);\n}\n```\n\n"
        "## Links and Resources\n\n"
        "- Visit the [FTXUI repository](https://github.com/ArthurcBreton/FTXUI)\n"
        "- Read the [CMake docs](https://cmake.org/documentation/)\n"
        "- Check out [cmark-gfm](https://github.com/github/cmark-gfm)\n\n"
        "> **Note:** This viewer supports *word wrapping*, "
        "**nested lists**, `inline code`, and [clickable links](https://example.com). "
        "Try resizing the window to see how paragraphs reflow.\n\n"
        "### Nested Lists\n\n"
        "- Fruits\n"
        "  - Apples\n"
        "  - Bananas\n"
        "- Vegetables\n"
        "  - Carrots\n"
        "  - Peas\n\n"
        "---\n\n"
        "That's all for now. Happy editing!\n\n"
        "## More Resources\n\n"
        "- Check [Markdown Guide](https://www.markdownguide.org) for syntax help\n"
        "- See [FTXUI examples](https://github.com/ArthurSonique/FTXUI/tree/main/examples) for UI patterns\n"
    );
    auto editor_comp = editor.component();

    // --- Viewer ---
    markdown::Viewer viewer(markdown::make_cmark_parser());
    std::string last_link_url;
    viewer.on_link_click([&](std::string const& url, bool pressed) {
        last_link_url = url;
        (void)pressed;
    });
    auto viewer_comp = viewer.component();

    // --- Toolbar ---
    auto cb_option = ftxui::CheckboxOption::Simple();
    cb_option.transform = [](ftxui::EntryState s) {
        auto el = ftxui::text(std::string(s.state ? "[X]" : "[ ]") + " " + s.label);
        if (s.focused) el |= ftxui::inverted;
        return el;
    };
    cb_option.label = "Scrollbar";
    cb_option.checked = &show_scrollbar;
    auto checkbox = ftxui::Checkbox(cb_option);
    auto quit_btn = ftxui::Button("Quit", [&] { screen.Exit(); },
                                  ftxui::ButtonOption::Ascii());
    auto toolbar = ftxui::Container::Horizontal({checkbox, quit_btn});

    // --- Container: FTXUI handles all navigation ---
    auto root = ftxui::Container::Vertical({toolbar, editor_comp, viewer_comp});

    auto component = ftxui::Renderer(root, [&] {
        // Wire state to viewer each frame
        viewer.set_content(editor.content());
        viewer.show_scrollbar(show_scrollbar);

        // Only sync scroll from editor when viewer is not selected
        if (!viewer.selected()) {
            float scroll_ratio = 0.0f;
            if (editor.total_lines() > 1) {
                scroll_ratio = static_cast<float>(editor.cursor_line() - 1) /
                               static_cast<float>(editor.total_lines() - 1);
            }
            viewer.set_scroll(scroll_ratio);
        }

        // Three border states
        auto selected_border = ftxui::borderStyled(
            ftxui::BorderStyle::DOUBLE, ftxui::Color::White);
        auto focused_border = ftxui::borderStyled(ftxui::Color::White);
        auto dim_border = ftxui::borderStyled(ftxui::Color::GrayDark);

        auto tb_border = toolbar->Focused() ? focused_border : dim_border;
        auto ed_border = !editor_comp->Focused() ? dim_border
            : (editor.selected() ? selected_border : focused_border);
        auto vw_border = !viewer_comp->Focused() ? dim_border
            : (viewer.selected() ? selected_border : focused_border);

        // Toolbar
        auto toolbar_el = ftxui::hbox({
            ftxui::text(" "),
            checkbox->Render(),
            ftxui::text("  "),
            quit_btn->Render(),
            ftxui::filler(),
            ftxui::text(" Enter:select  Esc:back  Ctrl+Q:quit ") | ftxui::dim,
        }) | tb_border;

        // Editor pane
        auto editor_pane = zero_min_width(
            ftxui::vbox({
                ftxui::text(" Markdown Editor ") | ftxui::bold | ftxui::center,
                ftxui::separator(),
                editor_comp->Render() | ftxui::flex | ftxui::frame,
            }) | ed_border
        ) | ftxui::flex;

        // Viewer pane
        auto viewer_pane = zero_min_width(
            ftxui::vbox({
                ftxui::text(" Markdown Viewer ") | ftxui::bold | ftxui::center,
                ftxui::separator(),
                viewer_comp->Render(),
            }) | vw_border
        ) | ftxui::flex;

        // Status bar
        auto status_text = " Ln " + std::to_string(editor.cursor_line()) +
                           ", Col " + std::to_string(editor.cursor_col()) +
                           " | " + std::to_string(editor.content().size()) + " chars ";
        ftxui::Elements status_parts;
        if (!last_link_url.empty()) {
            status_parts.push_back(
                ftxui::text(" " + last_link_url + " ") | ftxui::dim | ftxui::underlined);
        }
        status_parts.push_back(ftxui::filler());
        status_parts.push_back(ftxui::text(status_text) | ftxui::dim);
        auto status_bar = ftxui::hbox(std::move(status_parts));

        return ftxui::vbox({
            toolbar_el,
            ftxui::hbox({
                editor_pane,
                viewer_pane,
            }) | ftxui::flex,
            status_bar,
        });
    });

    // ArrowRight/Left → ArrowDown/Up when nothing is selected.
    // Ctrl+Q to quit.
    component = ftxui::CatchEvent(component, [&](ftxui::Event event) {
        if (!editor.selected() && !viewer.selected()) {
            if (event == ftxui::Event::ArrowRight)
                return root->OnEvent(ftxui::Event::ArrowDown);
            if (event == ftxui::Event::ArrowLeft)
                return root->OnEvent(ftxui::Event::ArrowUp);
        }
        if (event == ftxui::Event::Character('q') &&
            event.is_character() == false) {
            screen.Exit();
            return true;
        }
        return false;
    });

    screen.Loop(component);
    return 0;
}
