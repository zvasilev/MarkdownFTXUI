#include "markdown/viewer.hpp"

#include <ftxui/component/event.hpp>
#include <ftxui/component/mouse.hpp>

namespace markdown {

Viewer::Viewer(std::unique_ptr<MarkdownParser> parser)
    : parser_(std::move(parser)) {}

void Viewer::set_content(std::string_view markdown_text) {
    content_.assign(markdown_text.data(), markdown_text.size());
}

void Viewer::set_scroll(float ratio) {
    scroll_ratio_ = ratio;
}

void Viewer::show_scrollbar(bool show) {
    show_scrollbar_ = show;
}

void Viewer::on_link_click(
    std::function<void(std::string const&)> callback) {
    link_callback_ = std::move(callback);
}

ftxui::Component Viewer::component() {
    if (component_) return component_;

    auto renderer = ftxui::Renderer([this] {
        if (content_ != last_parsed_) {
            auto ast = parser_->parse(content_);
            cached_element_ = builder_.build(ast);
            last_parsed_ = content_;
        }
        auto el = cached_element_
            | ftxui::focusPositionRelative(0.0f, scroll_ratio_);
        if (show_scrollbar_) el = el | ftxui::vscroll_indicator;
        return el | ftxui::yframe | ftxui::flex;
    });

    component_ = ftxui::CatchEvent(renderer, [this](ftxui::Event event) {
        if (!event.is_mouse()) return false;
        auto& mouse = event.mouse();
        if (mouse.button != ftxui::Mouse::Left ||
            mouse.motion != ftxui::Mouse::Pressed) {
            return false;
        }
        for (auto const& link : builder_.link_targets()) {
            if (link.box.Contain(mouse.x, mouse.y)) {
                if (link_callback_) {
                    link_callback_(link.url);
                }
                return true;
            }
        }
        return false;
    });

    return component_;
}

} // namespace markdown
