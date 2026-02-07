#pragma once

#include <functional>
#include <memory>
#include <string>
#include <string_view>

#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>

#include "markdown/dom_builder.hpp"
#include "markdown/parser.hpp"

namespace markdown {

class Viewer {
public:
    explicit Viewer(std::unique_ptr<MarkdownParser> parser);

    void set_content(std::string_view markdown_text);
    void set_scroll(float ratio);
    void on_link_click(std::function<void(std::string const&)> callback);

    ftxui::Component component();

private:
    std::unique_ptr<MarkdownParser> parser_;
    DomBuilder builder_;
    std::string content_;
    std::string last_parsed_;
    ftxui::Element cached_element_ = ftxui::text("");
    float scroll_ratio_ = 0.0f;
    std::function<void(std::string const&)> link_callback_;
    ftxui::Component component_;
};

} // namespace markdown
