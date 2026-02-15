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

void Viewer::add_focusable(std::string label, std::string value) {
    _externals.push_back({std::move(label), std::move(value)});
}

void Viewer::clear_focusables() {
    _externals.clear();
    _focus_index = -1;
}

bool Viewer::is_external_focused(int external_index) const {
    return _focus_index >= 0 &&
           _focus_index == external_index &&
           external_index < static_cast<int>(_externals.size());
}

std::string Viewer::focused_value() const {
    if (_focus_index < 0) return {};
    int ec = static_cast<int>(_externals.size());
    if (_focus_index < ec) return _externals[_focus_index].value;
    int li = _focus_index - ec;
    auto const& targets = _builder.link_targets();
    if (li < static_cast<int>(targets.size())) return targets[li].url;
    return {};
}

bool Viewer::is_link_focused() const {
    return _focus_index >= 0 &&
           _focus_index >= static_cast<int>(_externals.size());
}

namespace {

constexpr float kScrollArrowStep = 0.05f;
constexpr float kScrollWheelStep = 0.05f;

// Wrapper with built-in scroll and link navigation for the viewer.
// Two modes:
//   Tab ring (externals registered): Tab/arrows always work, no Enter gate.
//   Normal (no externals): Enter activates, Esc deactivates.
class ViewerWrap : public ftxui::ComponentBase {
    bool& _active;
    float& _scroll_ratio;
    int& _focus_index;
    DomBuilder& _builder;
    std::vector<ExternalFocusable>& _externals;
    std::function<void(std::string const&, LinkEvent)>& _link_callback;
    ScrollInfo const& _scroll_info;
public:
    ViewerWrap(ftxui::Component child, bool& active, float& scroll,
               int& focus_index, DomBuilder& builder,
               std::vector<ExternalFocusable>& externals,
               std::function<void(std::string const&, LinkEvent)>& link_cb,
               ScrollInfo const& scroll_info)
        : _active(active), _scroll_ratio(scroll),
          _focus_index(focus_index), _builder(builder),
          _externals(externals), _link_callback(link_cb),
          _scroll_info(scroll_info) {
        Add(std::move(child));
    }

    bool Focusable() const override { return true; }

    bool OnEvent(ftxui::Event event) override {
        bool has_ring = !_externals.empty();

        // Mouse: wheel scroll + activate on click (both modes)
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

        // --- Tab ring mode (externals registered) ---
        if (has_ring) {
            if (event == ftxui::Event::Tab) {
                cycle_focus(+1);
                return true;
            }
            if (event == ftxui::Event::TabReverse) {
                cycle_focus(-1);
                return true;
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
            if (event == ftxui::Event::Return) {
                if (_focus_index >= 0) {
                    notify_focus(LinkEvent::Press);
                }
                return true;
            }
            if (event == ftxui::Event::Escape) {
                _focus_index = -1;
                return false;  // let parent handle (e.g. go to menu)
            }
            return false;
        }

        // --- Normal mode (no externals, existing behavior) ---
        if (_active) {
            if (event == ftxui::Event::Escape) {
                _active = false;
                _focus_index = -1;
                return true;
            }
            if (event == ftxui::Event::Tab) {
                cycle_focus(+1);
                return true;
            }
            if (event == ftxui::Event::TabReverse) {
                cycle_focus(-1);
                return true;
            }
            if (event == ftxui::Event::Return) {
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
        if (event == ftxui::Event::Return) {
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

    int ext_count() const { return static_cast<int>(_externals.size()); }
    int link_count() const {
        return static_cast<int>(_builder.link_targets().size());
    }
    int total_focusable() const { return ext_count() + link_count(); }

    void cycle_focus(int direction) {
        int total = total_focusable();
        if (total == 0) return;
        if (_focus_index < 0) {
            _focus_index = (direction > 0) ? 0 : total - 1;
        } else {
            _focus_index = (_focus_index + direction + total) % total;
        }
        // Scroll to top when focusing an external so headers are visible.
        if (_focus_index < ext_count()) {
            _scroll_ratio = 0.0f;
        }
        notify_focus(LinkEvent::Focus);
    }

    void notify_focus(LinkEvent event) {
        if (_focus_index < 0) return;
        int ec = ext_count();
        std::string value;
        if (_focus_index < ec) {
            value = _externals[_focus_index].value;
        } else {
            int li = _focus_index - ec;
            auto const& targets = _builder.link_targets();
            if (li >= static_cast<int>(targets.size())) return;
            value = targets[li].url;
        }
        if (_link_callback) {
            _link_callback(value, event);
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

        // Derive _focused_link from unified _focus_index
        int ec = static_cast<int>(_externals.size());
        int total = ec + static_cast<int>(_builder.link_targets().size());
        if (_focus_index >= total) {
            _focus_index = total > 0 ? total - 1 : -1;
        }
        _focused_link = (_focus_index >= ec) ? _focus_index - ec : -1;

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
        // When a link is focused, yframe scrolls to the ftxui::focus
        // element. Otherwise use direct_scroll for linear offset.
        if (_focused_link < 0) {
            el = direct_scroll(std::move(el), _scroll_ratio, &_scroll_info);
        } else {
            el = el | ftxui::yframe;
        }
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
        _builder, _externals, _link_callback, _scroll_info);
    return _component;
}

} // namespace markdown
