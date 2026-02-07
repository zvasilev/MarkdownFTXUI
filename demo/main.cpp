#include <string>
#include <chrono>

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include "markdown/parser.hpp"
#include "markdown/dom_builder.hpp"

int main() {
    auto screen = ftxui::ScreenInteractive::Fullscreen();

    std::string document_text = "# Hello Markdown\n\n"
        "## Section One\n\n"
        "This is a paragraph of plain text.\n\n"
        "### Subsection\n\n"
        "Another paragraph here.\n";

    auto parser = markdown::make_cmark_parser();
    markdown::DomBuilder builder;

    // Track last content to debounce viewer updates
    std::string last_rendered;
    ftxui::Element viewer_element = ftxui::text("");
    auto last_update = std::chrono::steady_clock::now();

    auto input_option = ftxui::InputOption();
    input_option.multiline = true;
    auto input = ftxui::Input(&document_text, input_option);

    auto component = ftxui::Renderer(input, [&] {
        // Debounce: re-parse if content changed and 50ms elapsed
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
            input->Render() | ftxui::flex | ftxui::frame,
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
