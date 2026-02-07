#include "markdown/viewer.hpp"

#include <algorithm>
#include <iterator>

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
    std::function<void(std::string const&, bool)> callback) {
    _link_callback = std::move(callback);
}

// Selectable wrapper with built-in scroll and link navigation for the viewer.
// When selected: Tab/Shift+Tab cycle links, Enter presses focused link,
// ArrowUp/Down scroll, Esc deselects.
// When not selected: all keyboard unhandled -> container navigates.
namespace {
class SelectableViewer : public ftxui::ComponentBase {
    bool& _selected;
    float& _scroll_ratio;
    int& _focused_link;
    DomBuilder& _builder;
    std::function<void(std::string const&, bool)>& _link_callback;
public:
    SelectableViewer(ftxui::Component child, bool& selected, float& scroll,
                     int& focused_link, DomBuilder& builder,
                     std::function<void(std::string const&, bool)>& link_cb)
        : _selected(selected), _scroll_ratio(scroll),
          _focused_link(focused_link), _builder(builder),
          _link_callback(link_cb) {
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
                _focused_link = -1;
                return true;
            }
            // Tab cycles forward through links
            if (event == ftxui::Event::Tab) {
                int count = static_cast<int>(
                    _builder.link_targets().size());
                if (count > 0) {
                    _focused_link = (_focused_link + 1) % count;
                    notify_link_focus();
                }
                return true;
            }
            // Shift+Tab cycles backward through links
            if (event == ftxui::Event::TabReverse) {
                int count = static_cast<int>(
                    _builder.link_targets().size());
                if (count > 0) {
                    _focused_link =
                        (_focused_link - 1 + count) % count;
                    notify_link_focus();
                }
                return true;
            }
            // Enter presses the focused link
            if (event == ftxui::Event::Return) {
                if (_focused_link >= 0) {
                    auto it = _builder.link_targets().begin();
                    std::advance(it, _focused_link);
                    if (_link_callback) {
                        _link_callback(it->url, true);
                    }
                    return true;
                }
                return false;
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
            // Everything else (including left/right) -> unhandled -> navigate
            return false;
        }
        if (event == ftxui::Event::Return) {
            _selected = true;
            return true;
        }
        return false;
    }

private:
    void notify_link_focus() {
        if (_focused_link < 0) return;
        auto it = _builder.link_targets().begin();
        std::advance(it, _focused_link);
        if (_link_callback) {
            _link_callback(it->url, false);
        }
    }
};
} // namespace

ftxui::Component Viewer::component() {
    if (_component) return _component;

    auto renderer = ftxui::Renderer([this] {
        // Parse only when content changes
        if (_content != _last_parsed) {
            _cached_ast = _parser->parse(_content);
            _last_parsed = _content;
        }
        // Rebuild element when content or focused link changes
        if (_content != _last_built ||
            _focused_link != _last_focused_link) {
            _cached_element = _builder.build(_cached_ast, _focused_link);
            _last_built = _content;
            _last_focused_link = _focused_link;
        }
        auto el = _cached_element;
        // When a link is focused, ftxui::focus on it handles scrolling via
        // yframe. Otherwise use the manual scroll ratio.
        if (_focused_link < 0) {
            el = el | ftxui::focusPositionRelative(0.0f, _scroll_ratio);
        }
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
                    _link_callback(link.url, true);
                }
                return true;
            }
        }
        return false;
    });

    // Wrap with selectable + scroll + link navigation behavior
    _component = std::make_shared<SelectableViewer>(
        inner, _selected, _scroll_ratio, _focused_link,
        _builder, _link_callback);
    return _component;
}

} // namespace markdown
