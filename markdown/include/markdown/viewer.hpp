#pragma once

#include <memory>
#include <string_view>

#include <ftxui/component/component.hpp>

#include "markdown/parser.hpp"

namespace markdown {

class Viewer {
public:
    explicit Viewer(std::unique_ptr<MarkdownParser> parser);

    void set_content(std::string_view markdown_text);

    ftxui::Component component();

private:
    std::unique_ptr<MarkdownParser> parser_;
};

} // namespace markdown
