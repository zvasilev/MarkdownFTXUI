#include "markdown/viewer.hpp"

namespace markdown {

Viewer::Viewer(std::unique_ptr<MarkdownParser> parser)
    : parser_(std::move(parser)) {}

void Viewer::set_content(std::string_view /*markdown_text*/) {
    // Stub — will be implemented in later phases
}

ftxui::Component Viewer::component() {
    return ftxui::Renderer([] {
        return ftxui::text("Viewer placeholder");
    });
}

} // namespace markdown
