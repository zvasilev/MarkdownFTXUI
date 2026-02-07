#include "markdown/viewer.hpp"

#include <algorithm>

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

// Selectable wrapper with built-in scroll for the viewer.
// When selected: ArrowUp/Down scroll, Esc deselects, other keys unhandled.
// When not selected: all keyboard unhandled → container navigates.
namespace {
class SelectableViewer : public ftxui::ComponentBase {
    bool& _selected;
    float& _scroll_ratio;
public:
    SelectableViewer(ftxui::Component child, bool& selected, float& scroll)
        : _selected(selected), _scroll_ratio(scroll) {
        Add(std::move(child));
    }

    bool Focusable() const override { return true; }

    bool OnEvent(ftxui::Event event) override {
        if (event.is_mouse()) {
            if (event.mouse().button == ftxui::Mouse::Left &&
                event.mouse().motion == ftxui::Mouse::Pressed) {
                _selected = true;
                TakeFocus();
            }
            return ComponentBase::OnEvent(event);
        }
        if (_selected) {
            if (event == ftxui::Event::Escape) {
                _selected = false;
                return true;
            }
            // Arrow up/down scroll the viewer
            if (event == ftxui::Event::ArrowUp) {
                _scroll_ratio = std::max(0.0f, _scroll_ratio - 0.05f);
                return true;
            }
            if (event == ftxui::Event::ArrowDown) {
                _scroll_ratio = std::min(1.0f, _scroll_ratio + 0.05f);
                return true;
            }
            if (event == ftxui::Event::Tab || event == ftxui::Event::TabReverse) {
                return true; // consume Tab while selected
            }
            // Everything else (including left/right) → unhandled → navigate
            return false;
        }
        if (event == ftxui::Event::Return) {
            _selected = true;
            return true;
        }
        return false;
    }
};
} // namespace

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

    auto inner = ftxui::CatchEvent(renderer, [this](ftxui::Event event) {
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

    // Wrap with selectable + scroll behavior
    _component = std::make_shared<SelectableViewer>(
        inner, _selected, _scroll_ratio);
    return _component;
}

} // namespace markdown
