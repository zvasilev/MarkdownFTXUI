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
    int ext_n = static_cast<int>(_externals.size());
    if (_focus_index < ext_n) {
        return _externals[_focus_index].value;
    }
    int link_idx = _focus_index - ext_n;
    auto const& targets = _builder.link_targets();
    if (link_idx < static_cast<int>(targets.size())) {
        return targets[link_idx].url;
    }
    return {};
}

bool Viewer::is_link_focused() const {
    return _focus_index >= static_cast<int>(_externals.size()) &&
           _focus_index >= 0;
}

namespace {

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
public:
    ViewerWrap(ftxui::Component child, bool& active, float& scroll,
               int& focus_index, DomBuilder& builder,
               std::vector<ExternalFocusable>& externals,
               std::function<void(std::string const&, LinkEvent)>& link_cb)
        : _active(active), _scroll_ratio(scroll),
          _focus_index(focus_index), _builder(builder),
          _externals(externals), _link_callback(link_cb) {
        Add(std::move(child));
    }

    bool Focusable() const override { return true; }

    bool OnEvent(ftxui::Event event) override {
        bool has_ring = !_externals.empty();

        // Mouse: activate + take focus (both modes)
        if (event.is_mouse()) {
            if (event.mouse().button == ftxui::Mouse::Left &&
                event.mouse().motion == ftxui::Mouse::Pressed) {
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
            if (event == ftxui::Event::ArrowUp) {
                _scroll_ratio = std::max(0.0f, _scroll_ratio - 0.05f);
                return true;
            }
            if (event == ftxui::Event::ArrowDown) {
                _scroll_ratio = std::min(1.0f, _scroll_ratio + 0.05f);
                return true;
            }
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
            if (event == ftxui::Event::ArrowUp) {
                _scroll_ratio = std::max(0.0f, _scroll_ratio - 0.05f);
                return true;
            }
            if (event == ftxui::Event::ArrowDown) {
                _scroll_ratio = std::min(1.0f, _scroll_ratio + 0.05f);
                return true;
            }
            return false;
        }
        if (event == ftxui::Event::Return) {
            _active = true;
            return true;
        }
        return false;
    }

private:
    void cycle_focus(int direction) {
        int ext_n = static_cast<int>(_externals.size());
        int link_n = static_cast<int>(_builder.link_targets().size());
        int total = ext_n + link_n;
        if (total == 0) return;
        _focus_index = (_focus_index + direction + total) % total;
        notify_focus(LinkEvent::Focus);
    }

    void notify_focus(LinkEvent event) {
        if (_focus_index < 0) return;
        int ext_n = static_cast<int>(_externals.size());
        std::string value;
        if (_focus_index < ext_n) {
            value = _externals[_focus_index].value;
        } else {
            int link_idx = _focus_index - ext_n;
            auto const& targets = _builder.link_targets();
            if (link_idx >= static_cast<int>(targets.size())) return;
            value = targets[link_idx].url;
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
        int ext_n = static_cast<int>(_externals.size());
        int link_n = static_cast<int>(_builder.link_targets().size());
        int total = ext_n + link_n;
        if (_focus_index >= total) {
            _focus_index = total > 0 ? total - 1 : -1;
        }
        _focused_link = (_focus_index >= ext_n) ? _focus_index - ext_n : -1;

        // Rebuild element when content, focused link, or theme changes
        if (_parsed_gen != _built_gen ||
            _focused_link != _last_focused_link ||
            _theme_gen != _built_theme_gen) {
            _cached_element = _builder.build(_cached_ast, _focused_link,
                                             _theme);
            _built_gen = _parsed_gen;
            _last_focused_link = _focused_link;
            _built_theme_gen = _theme_gen;
        }
        auto el = _cached_element;

        // Embed mode: return raw element; caller handles framing.
        if (_embed) return el;

        if (_show_scrollbar) el = el | ftxui::vscroll_indicator;
        // When a link is focused, yframe scrolls to the ftxui::focus
        // element. Otherwise use direct_scroll for linear offset.
        if (_focused_link < 0) {
            el = direct_scroll(std::move(el), _scroll_ratio);
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
        for (auto const& link : _builder.link_targets()) {
            for (auto const& box : link.boxes) {
                if (box.Contain(mouse.x, mouse.y)) {
                    if (_link_callback) {
                        _link_callback(link.url, LinkEvent::Press);
                    }
                    return true;
                }
            }
        }
        return false;
    });

    // Wrap with active + scroll + link navigation behavior
    _component = std::make_shared<ViewerWrap>(
        inner, _active, _scroll_ratio, _focus_index,
        _builder, _externals, _link_callback);
    return _component;
}

} // namespace markdown
