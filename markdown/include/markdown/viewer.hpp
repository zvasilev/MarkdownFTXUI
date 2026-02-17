#pragma once

#include <functional>
#include <memory>
#include <string>
#include <string_view>

#include <ftxui/component/component.hpp>
#include <ftxui/dom/elements.hpp>

#include "markdown/dom_builder.hpp"
#include "markdown/parser.hpp"
#include "markdown/scroll_frame.hpp"

namespace markdown {

enum class LinkEvent { Focus, Press };

class Viewer {
public:
    explicit Viewer(std::unique_ptr<MarkdownParser> parser);
    Viewer(Viewer&&) = delete;
    Viewer& operator=(Viewer&&) = delete;

    void set_content(std::string_view markdown_text);
    void set_scroll(float ratio);
    void show_scrollbar(bool show);
    void on_link_click(
        std::function<void(std::string const&, LinkEvent)> callback);
    void on_tab_exit(std::function<void(int direction)> callback);
    bool enter_focus(int direction);
    void set_theme(Theme const& theme) {
        if (_theme.name != theme.name) { _theme = theme; ++_theme_gen; }
    }
    void set_max_quote_depth(int d) {
        _builder.set_max_quote_depth(d);
        ++_builder_gen;
    }
    int max_quote_depth() const { return _builder.max_quote_depth(); }

    /// Returns the FTXUI component. Created on first call, cached thereafter.
    /// Configure the object (set_content, set_theme, on_link_click, etc.)
    /// before OR after this call — the renderer reads live state each frame.
    /// However, do not move this object after calling component().
    ftxui::Component component();
    bool active() const { return _active; }
    void set_active(bool a) { _active = a; }

    // Focus index into link_targets: -1=none, 0..N-1=links.
    int focused_index() const { return _focus_index; }
    std::string focused_value() const;

    // Embed mode: skip internal yframe/vscroll/flex so the caller can
    // wrap the viewer element together with other content in a single frame.
    void set_embed(bool embed) { _embed = embed; }
    bool is_embed() const { return _embed; }
    float scroll() const { return _scroll_ratio; }
    ScrollInfo const& scroll_info() const { return _scroll_info; }
    bool scrollbar_visible() const { return _show_scrollbar; }
    Theme const& theme() const { return _theme; }
    bool is_link_focused() const;

private:
    std::unique_ptr<MarkdownParser> _parser;
    DomBuilder _builder;
    std::string _content;
    uint64_t _content_gen = 0;
    uint64_t _parsed_gen = 0;
    uint64_t _built_gen = 0;
    MarkdownAST _cached_ast;
    ftxui::Element _cached_element = ftxui::text("");
    float _scroll_ratio = 0.0f;
    ScrollInfo _scroll_info;
    bool _show_scrollbar = true;
    bool _active = false;
    int _focus_index = -1;
    int _focused_link = -1;       // derived from _focus_index for DomBuilder
    int _last_focused_link = -1;
    Theme _theme{theme_default()};
    uint64_t _theme_gen = 0;
    uint64_t _built_theme_gen = 0;
    uint64_t _builder_gen = 0;
    uint64_t _built_builder_gen = 0;
    std::function<void(std::string const&, LinkEvent)> _link_callback;
    std::function<void(int direction)> _tab_exit_callback;
    ftxui::Component _component;
    bool _embed = false;
};

} // namespace markdown
