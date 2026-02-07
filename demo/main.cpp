#include <string>
#include <chrono>
#include <algorithm>
#include <sstream>
#include <vector>

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include "markdown/parser.hpp"
#include "markdown/dom_builder.hpp"
#include "markdown/highlight.hpp"

int main() {
    auto screen = ftxui::ScreenInteractive::Fullscreen();

    std::string document_text = "# Hello Markdown\n\n"
        "> Focus on **important** tasks\n\n"
        "## Section One\n\n"
        "This is **bold**, *italic*, and ***bold italic***.\n\n"
        "### Todo List\n\n"
        "- Write *code*\n"
        "- Review `tests` carefully\n"
        "- Read [docs](https://example.com)\n\n"
        "```\nint main() {\n    return 0;\n}\n```\n";

    auto parser = markdown::make_cmark_parser();
    markdown::DomBuilder builder;

    // Track last content to debounce viewer updates
    std::string last_rendered;
    ftxui::Element viewer_element = ftxui::text("");
    auto last_update = std::chrono::steady_clock::now();

    int cursor_pos = 0;
    ftxui::Box editor_box;

    auto input_option = ftxui::InputOption();
    input_option.multiline = true;
    input_option.cursor_position = &cursor_pos;
    input_option.transform = [&](ftxui::InputState state) {
        if (state.is_placeholder) return state.element;
        auto element = markdown::highlight_markdown_with_cursor(
            document_text, cursor_pos, state.focused, state.hovered);
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

    auto component = ftxui::Renderer(input_with_mouse, [&] {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - last_update);

        if (document_text != last_rendered && elapsed.count() >= 50) {
            auto ast = parser->parse(document_text);
            viewer_element = builder.build(ast);
            last_rendered = document_text;
            last_update = now;
        }

        auto editor_pane = ftxui::vbox({
            ftxui::text(" Markdown Editor ") | ftxui::bold | ftxui::center,
            ftxui::separator(),
            input_with_mouse->Render() | ftxui::flex | ftxui::frame,
        }) | ftxui::border | ftxui::flex;

        auto viewer_pane = ftxui::vbox({
            ftxui::text(" Markdown Viewer ") | ftxui::bold | ftxui::center,
            ftxui::separator(),
            viewer_element | ftxui::flex | ftxui::frame,
        }) | ftxui::border | ftxui::flex;

        return ftxui::hbox({
            editor_pane,
            viewer_pane,
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
