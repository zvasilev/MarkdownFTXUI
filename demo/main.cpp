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

    // Editor component — owns content, syntax highlighting, cursor, mouse clicks
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
        "That's all for now. Happy editing!\n"
    );
    auto editor_comp = editor.component();

    // Viewer component — owns parser, DomBuilder, caching, link clicks
    markdown::Viewer viewer(markdown::make_cmark_parser());
    std::string last_link_url;
    viewer.on_link_click([&](std::string const& url) {
        last_link_url = url;
    });
    auto viewer_comp = viewer.component();

    // Both editor and viewer in the component tree so both receive events
    auto both = ftxui::Container::Horizontal({editor_comp, viewer_comp});

    auto component = ftxui::Renderer(both, [&] {
        // Feed editor content and scroll position to the viewer
        viewer.set_content(editor.content());

        float scroll_ratio = 0.0f;
        if (editor.total_lines() > 1) {
            scroll_ratio = static_cast<float>(editor.cursor_line() - 1) /
                           static_cast<float>(editor.total_lines() - 1);
        }
        viewer.set_scroll(scroll_ratio);

        auto editor_pane = zero_min_width(
            ftxui::vbox({
                ftxui::text(" Markdown Editor ") | ftxui::bold | ftxui::center,
                ftxui::separator(),
                editor_comp->Render() | ftxui::flex | ftxui::frame,
            }) | ftxui::border) | ftxui::flex;

        auto viewer_pane = zero_min_width(
            ftxui::vbox({
                ftxui::text(" Markdown Viewer ") | ftxui::bold | ftxui::center,
                ftxui::separator(),
                viewer_comp->Render(),
            }) | ftxui::border) | ftxui::flex;

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
            ftxui::hbox({
                editor_pane,
                viewer_pane,
            }) | ftxui::flex,
            status_bar,
        });
    });

    // Catch Ctrl+C and Ctrl+Q to quit
    component = ftxui::CatchEvent(component, [&](ftxui::Event event) {
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
