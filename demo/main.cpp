#include <string>
#include <algorithm>
#include <sstream>
#include <vector>

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/dom/node.hpp>

#include "markdown/parser.hpp"
#include "markdown/viewer.hpp"
#include "markdown/highlight.hpp"

namespace {
// Reports min_x=0 so that flex distributes hbox space equally,
// while passing through the full assigned width to the child.
// Applied at the pane level (outside frame/border) so that frame
// scrolling still works correctly with the real content width.
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

    std::string document_text = "# Hello Markdown\n\n"
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
        "That's all for now. Happy editing!\n";

    // Viewer component — owns parser, DomBuilder, caching, link clicks
    markdown::Viewer viewer(markdown::make_cmark_parser());
    std::string last_link_url;
    viewer.on_link_click([&](std::string const& url) {
        last_link_url = url;
    });
    auto viewer_comp = viewer.component();

    int cursor_pos = 0;
    ftxui::Box editor_box;

    auto input_option = ftxui::InputOption();
    input_option.multiline = true;
    input_option.cursor_position = &cursor_pos;
    input_option.transform = [&](ftxui::InputState state) {
        if (state.is_placeholder) return state.element;
        auto element = markdown::highlight_markdown_with_cursor(
            document_text, cursor_pos, state.focused, state.hovered, true);
        return element | ftxui::reflect(editor_box);
    };
    auto input = ftxui::Input(&document_text, input_option);

    // Intercept left-press mouse clicks to fix cursor positioning
    // (Input's internal cursor_box_ is lost when transform replaces the element).
    // Non-click mouse events (hover, move) pass through to Input normally.
    auto input_with_mouse = ftxui::CatchEvent(input, [&](ftxui::Event event) {
        if (!event.is_mouse()) return false;

        auto& mouse = event.mouse();

        // Only intercept left-press clicks inside the editor area.
        // Let hover/move/release events pass through to Input for proper state.
        if (mouse.button != ftxui::Mouse::Left ||
            mouse.motion != ftxui::Mouse::Pressed ||
            !editor_box.Contain(mouse.x, mouse.y)) {
            return false;
        }

        input->TakeFocus();

        int click_y = mouse.y - editor_box.y_min;
        int click_x = mouse.x - editor_box.x_min;

        // Split document into lines
        std::vector<std::string> lines;
        std::istringstream iss(document_text);
        std::string line;
        while (std::getline(iss, line)) lines.push_back(line);
        if (lines.empty()) lines.push_back("");

        // Compute gutter width to offset click_x
        int total = static_cast<int>(lines.size());
        int gutter_width = 1;
        while (total >= 10) { ++gutter_width; total /= 10; }
        int gutter_chars = gutter_width + 3; // " N │ "
        click_x -= gutter_chars;
        click_x = std::max(0, click_x);

        click_y = std::clamp(click_y, 0, static_cast<int>(lines.size()) - 1);
        click_x = std::clamp(click_x, 0, static_cast<int>(lines[click_y].size()));

        int pos = 0;
        for (int i = 0; i < click_y; ++i) {
            pos += static_cast<int>(lines[i].size()) + 1;
        }
        pos += click_x;
        cursor_pos = std::min(pos, static_cast<int>(document_text.size()));

        return true;
    });

    // Both editor and viewer in the component tree so both receive events
    auto both = ftxui::Container::Horizontal({input_with_mouse, viewer_comp});

    auto component = ftxui::Renderer(both, [&] {
        // Push content and scroll position to the viewer
        viewer.set_content(document_text);

        // Compute cursor line/column (used by status bar and scroll sync)
        int cur_line = 1;
        int cur_col = 1;
        int total_lines = 1;
        {
            for (char c : document_text) {
                if (c == '\n') ++total_lines;
            }
            int remaining = std::min(cursor_pos,
                                     static_cast<int>(document_text.size()));
            size_t start = 0;
            int line_num = 1;
            while (start < document_text.size()) {
                auto nl = document_text.find('\n', start);
                int line_len = (nl == std::string::npos)
                    ? static_cast<int>(document_text.size() - start)
                    : static_cast<int>(nl - start);
                if (remaining <= line_len) {
                    cur_line = line_num;
                    cur_col = remaining + 1;
                    break;
                }
                remaining -= line_len + 1;
                start = (nl == std::string::npos) ? document_text.size() : nl + 1;
                ++line_num;
                if (start >= document_text.size()) {
                    cur_line = line_num;
                    cur_col = 1;
                }
            }
        }

        // Proportional scroll sync
        float scroll_ratio = 0.0f;
        if (total_lines > 1) {
            scroll_ratio = static_cast<float>(cur_line - 1) /
                           static_cast<float>(total_lines - 1);
        }
        viewer.set_scroll(scroll_ratio);

        auto editor_pane = zero_min_width(
            ftxui::vbox({
                ftxui::text(" Markdown Editor ") | ftxui::bold | ftxui::center,
                ftxui::separator(),
                input_with_mouse->Render() | ftxui::flex | ftxui::frame,
            }) | ftxui::border) | ftxui::flex;

        auto viewer_pane = zero_min_width(
            ftxui::vbox({
                ftxui::text(" Markdown Viewer ") | ftxui::bold | ftxui::center,
                ftxui::separator(),
                viewer_comp->Render(),
            }) | ftxui::border) | ftxui::flex;

        auto status_text = " Ln " + std::to_string(cur_line) +
                           ", Col " + std::to_string(cur_col) +
                           " | " + std::to_string(document_text.size()) + " chars ";
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
