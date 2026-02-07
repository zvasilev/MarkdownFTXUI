#include "markdown/viewer.hpp"

#include <ftxui/component/event.hpp>
#include <ftxui/component/mouse.hpp>

namespace markdown {

Viewer::Viewer(std::unique_ptr<MarkdownParser> parser)
    : _parser(std::move(parser)) {}

void Viewer::set_content(std::string_view markdown_text) {
    _content.assign(markdown_text.data(), markdown_text.size());
}

void Viewer::set_scroll(float ratio) {
    _scroll_ratio = ratio;
}

void Viewer::show_scrollbar(bool show) {
    _show_scrollbar = show;
}

void Viewer::on_link_click(
    std::function<void(std::string const&)> callback) {
    _link_callback = std::move(callback);
}

ftxui::Component Viewer::component() {
    if (_component) return _component;

    auto renderer = ftxui::Renderer([this] {
        if (_content != _last_parsed) {
            auto ast = _parser->parse(_content);
            _cached_element = _builder.build(ast);
            _last_parsed = _content;
        }
        auto el = _cached_element
            | ftxui::focusPositionRelative(0.0f, _scroll_ratio);
        if (_show_scrollbar) el = el | ftxui::vscroll_indicator;
        return el | ftxui::yframe | ftxui::flex;
    });

    _component = ftxui::CatchEvent(renderer, [this](ftxui::Event event) {
        if (!event.is_mouse()) return false;
        auto& mouse = event.mouse();
        if (mouse.button != ftxui::Mouse::Left ||
            mouse.motion != ftxui::Mouse::Pressed) {
            return false;
        }
        for (auto const& link : _builder.link_targets()) {
            if (link.box.Contain(mouse.x, mouse.y)) {
                if (_link_callback) {
                    _link_callback(link.url);
                }
                return true;
            }
        }
        return false;
    });

    return _component;
}

} // namespace markdown
