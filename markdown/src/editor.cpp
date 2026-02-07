#include "markdown/editor.hpp"

namespace markdown {

Editor::Editor() = default;

ftxui::Component Editor::component() {
    return ftxui::Input(&content_);
}

std::string const& Editor::content() const {
    return content_;
}

void Editor::set_content(std::string text) {
    content_ = std::move(text);
}

} // namespace markdown
