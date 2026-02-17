#include "markdown/viewer.hpp"
#include "markdown/scroll_frame.hpp"

#include <algorithm>
#include <memory>

#include <ftxui/component/event.hpp>
#include <ftxui/component/mouse.hpp>
#include <ftxui/screen/box.hpp>
#include <ftxui/screen/screen.hpp>

namespace markdown {

Viewer::Viewer(std::unique_ptr<MarkdownParser> parser)
    : _parser(std::move(parser)) {}

void Viewer::set_content(std::string_view markdown_text) {
    _content.assign(markdown_text.data(), markdown_text.size());
    ++_content_gen;
}

void Viewer::set_scroll(float ratio) {
    _scroll_ratio = ratio;
}

void Viewer::show_scrollbar(bool show) {
    _show_scrollbar = show;
}

void Viewer::on_link_click(
    std::function<void(std::string const&, LinkEvent)> callback) {
    _link_callback = std::move(callback);
}

void Viewer::on_tab_exit(std::function<void(int direction)> callback) {
    _tab_exit_callback = std::move(callback);
}

bool Viewer::enter_focus(int direction) {
    int total = static_cast<int>(_builder.link_targets().size());
    if (total == 0) return false;
    _active = true;
    _focus_index = (direction > 0) ? 0 : total - 1;
    scroll_to_focus();
    if (_link_callback) {
        _link_callback(focused_value(), LinkEvent::Focus);
    }
    return true;
}

std::string Viewer::focused_value() const {
    if (_focus_index < 0) return {};
    auto const& targets = _builder.link_targets();
    if (_focus_index < static_cast<int>(targets.size()))
        return targets[_focus_index].url;
    return {};
}

bool Viewer::is_link_focused() const {
    return _focus_index >= 0;
}

ftxui::Box Viewer::focused_link_box() const {
    auto const& targets = _builder.link_targets();
    if (_focus_index < 0 || _focus_index >= static_cast<int>(targets.size()))
        return {};
    auto const& boxes = targets[_focus_index].boxes;
    if (boxes.empty()) return {};
    return boxes[0];
}

void Viewer::scroll_to_focus() {
    // Use external scroll info (embed mode) or internal (non-embed)
    auto const& si = _ext_scroll_info ? *_ext_scroll_info : _scroll_info;
    if (_focus_index < 0 || si.viewport_height <= 0) return;
    auto const& targets = _builder.link_targets();
    if (_focus_index >= static_cast<int>(targets.size())) return;
    auto const& boxes = targets[_focus_index].boxes;
    if (boxes.empty()) return;
    auto const& lb = boxes[0];
    int scrollable = si.content_height - si.viewport_height;
    if (scrollable <= 0) return;
    int vp_top = si.viewport_y_min;
    int vp_bot = vp_top + si.viewport_height;
    // If y_max < y_min, the box was clipped by layout (off-screen element)
    if (lb.y_max >= lb.y_min &&
        lb.y_min >= vp_top && lb.y_max <= vp_bot) return; // already visible
    int dy = static_cast<int>(_scroll_ratio * scrollable);
    int content_y = lb.y_min - vp_top + dy;
    int target = content_y - si.viewport_height / 3;
    _scroll_ratio = std::clamp(
        static_cast<float>(target) / static_cast<float>(scrollable),
        0.0f, 1.0f);
}

namespace {

constexpr float kScrollArrowStep = 0.05f;
constexpr float kScrollWheelStep = 0.05f;

// Wrapper with built-in scroll and link navigation for the viewer.
// Enter activates, Esc deactivates. Tab cycles links.
// When on_tab_exit is set, Tab past bounds exits instead of wrapping.
class ViewerWrap : public ftxui::ComponentBase {
    bool& _active;
    float& _scroll_ratio;
    int& _focus_index;
    DomBuilder& _builder;
    std::function<void(std::string const&, LinkEvent)>& _link_callback;
    std::function<void(int)>& _tab_exit_callback;
    ViewerKeys const& _keys;
    ScrollInfo const& _scroll_info;
    std::function<void()> _scroll_to_focus;
public:
    ViewerWrap(ftxui::Component child, bool& active, float& scroll,
               int& focus_index, DomBuilder& builder,
               std::function<void(std::string const&, LinkEvent)>& link_cb,
               std::function<void(int)>& tab_exit_cb,
               ViewerKeys const& keys,
               ScrollInfo const& scroll_info,
               std::function<void()> scroll_to_focus)
        : _active(active), _scroll_ratio(scroll),
          _focus_index(focus_index), _builder(builder),
          _link_callback(link_cb),
          _tab_exit_callback(tab_exit_cb),
          _keys(keys),
          _scroll_info(scroll_info),
          _scroll_to_focus(std::move(scroll_to_focus)) {
        Add(std::move(child));
    }

    bool Focusable() const override { return true; }

    bool OnEvent(ftxui::Event event) override {
        // Mouse: wheel scroll + activate on click
        if (event.is_mouse()) {
            auto& m = event.mouse();
            if (m.button == ftxui::Mouse::WheelUp)
                return scroll_by(-kScrollWheelStep);
            if (m.button == ftxui::Mouse::WheelDown)
                return scroll_by(kScrollWheelStep);
            if (m.button == ftxui::Mouse::Left &&
                m.motion == ftxui::Mouse::Pressed) {
                _active = true;
                TakeFocus();
            }
            return ComponentBase::OnEvent(event);
        }

        if (_active) {
            if (event == _keys.deactivate) {
                _active = false;
                _focus_index = -1;
                return true;
            }
            if (event == _keys.next) {
                cycle_focus(+1);
                return true;
            }
            if (event == _keys.prev) {
                cycle_focus(-1);
                return true;
            }
            if (event == _keys.activate) {
                if (_focus_index >= 0) {
                    notify_focus(LinkEvent::Press);
                    return true;
                }
                return false;
            }
            if (event == ftxui::Event::ArrowUp)
                return scroll_by(-kScrollArrowStep);
            if (event == ftxui::Event::ArrowDown)
                return scroll_by(kScrollArrowStep);
            if (event == ftxui::Event::PageUp)
                return scroll_by(-page_step());
            if (event == ftxui::Event::PageDown)
                return scroll_by(page_step());
            if (event == ftxui::Event::Home)
                return scroll_by(-_scroll_ratio);
            if (event == ftxui::Event::End)
                return scroll_by(1.0f - _scroll_ratio);
            return false;
        }
        if (event == _keys.activate) {
            _active = true;
            return true;
        }
        return false;
    }

private:
    bool scroll_by(float delta) {
        _scroll_ratio = std::clamp(_scroll_ratio + delta, 0.0f, 1.0f);
        return true;
    }

    float page_step() const {
        if (_scroll_info.viewport_height > 0 &&
            _scroll_info.content_height > _scroll_info.viewport_height)
            return static_cast<float>(_scroll_info.viewport_height)
                 / static_cast<float>(_scroll_info.content_height);
        return 0.3f; // fallback for embed mode or before first render
    }

    int link_count() const {
        return static_cast<int>(_builder.link_targets().size());
    }

    void cycle_focus(int direction) {
        int total = link_count();
        if (total == 0) {
            if (_tab_exit_callback) {
                _tab_exit_callback(direction);
            }
            return;
        }
        if (_focus_index < 0) {
            _focus_index = (direction > 0) ? 0 : total - 1;
        } else {
            int next = _focus_index + direction;
            if (_tab_exit_callback && (next < 0 || next >= total)) {
                _focus_index = -1;
                _active = false;
                _tab_exit_callback(direction);
                return;
            }
            _focus_index = (next + total) % total;
        }
        _scroll_to_focus();
        notify_focus(LinkEvent::Focus);
    }

    void notify_focus(LinkEvent event) {
        if (_focus_index < 0) return;
        auto const& targets = _builder.link_targets();
        if (_focus_index >= static_cast<int>(targets.size())) return;
        if (_link_callback) {
            _link_callback(targets[_focus_index].url, event);
        }
    }
};
} // namespace

ftxui::Component Viewer::component() {
    if (_component) return _component;

    auto renderer = ftxui::Renderer([this] {
        // Parse only when content changes
        if (_content_gen != _parsed_gen) {
            _cached_ast = _parser->parse(_content);
            _parsed_gen = _content_gen;
        }

        // Clamp _focus_index to valid link range
        int total = static_cast<int>(_builder.link_targets().size());
        if (_focus_index >= total) {
            _focus_index = total > 0 ? total - 1 : -1;
        }
        _focused_link = _focus_index;

        // Rebuild element when content, focused link, theme, or builder config changes
        if (_parsed_gen != _built_gen ||
            _focused_link != _last_focused_link ||
            _theme_gen != _built_theme_gen ||
            _builder_gen != _built_builder_gen) {
            _cached_element = _builder.build(_cached_ast, _focused_link,
                                             _theme);
            _built_gen = _parsed_gen;
            _last_focused_link = _focused_link;
            _built_theme_gen = _theme_gen;
            _built_builder_gen = _builder_gen;
        }
        auto el = _cached_element;

        // Embed mode: return raw element; caller handles framing.
        if (_embed) return el;

        if (_show_scrollbar) el = el | ftxui::vscroll_indicator;
        el = direct_scroll(std::move(el), _scroll_ratio, &_scroll_info);
        return el | ftxui::flex;
    });

    auto inner = ftxui::CatchEvent(renderer, [this](ftxui::Event event) {
        if (!event.is_mouse()) return false;
        auto& mouse = event.mouse();
        if (mouse.button != ftxui::Mouse::Left ||
            mouse.motion != ftxui::Mouse::Pressed) {
            return false;
        }
        auto const& flat = _builder.flat_link_boxes();
        // Binary search for first box where y_min <= mouse.y
        auto it = std::lower_bound(flat.begin(), flat.end(), mouse.y,
            [](FlatLinkBox const& fb, int y) { return fb.box.y_min < y; });
        // Scan backwards for boxes that started before mouse.y but span it
        while (it != flat.begin()) {
            auto prev = std::prev(it);
            if (prev->box.y_max < mouse.y) break;
            it = prev;
        }
        for (; it != flat.end() && it->box.y_min <= mouse.y; ++it) {
            if (it->box.Contain(mouse.x, mouse.y)) {
                auto const& link = _builder.link_targets()[it->link_index];
                if (_link_callback) {
                    _link_callback(link.url, LinkEvent::Press);
                }
                return true;
            }
        }
        return false;
    });

    // Wrap with active + scroll + link navigation behavior
    _component = std::make_shared<ViewerWrap>(
        inner, _active, _scroll_ratio, _focus_index,
        _builder, _link_callback, _tab_exit_callback,
        _keys, _scroll_info,
        [this] { scroll_to_focus(); });
    return _component;
}

} // namespace markdown
