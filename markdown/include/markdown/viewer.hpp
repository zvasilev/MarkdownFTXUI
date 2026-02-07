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

enum class LinkEvent { Focus, Press };

struct ExternalFocusable {
    std::string label;
    std::string value;
};

class Viewer {
public:
    explicit Viewer(std::unique_ptr<MarkdownParser> parser);

    void set_content(std::string_view markdown_text);
    void set_scroll(float ratio);
    void show_scrollbar(bool show);
    void on_link_click(
        std::function<void(std::string const&, LinkEvent)> callback);
    void set_theme(Theme const& theme) { _theme = theme; ++_theme_gen; }

    ftxui::Component component();
    bool active() const { return _active; }
    void set_active(bool a) { _active = a; }

    // External focusable items (appear before links in Tab ring).
    // When externals are registered, Tab always cycles without Enter gate.
    void add_focusable(std::string label, std::string value);
    void clear_focusables();
    std::vector<ExternalFocusable> const& externals() const {
        return _externals;
    }

    // Unified focus index: -1=none, 0..N-1=external, N..N+M-1=links.
    int focused_index() const { return _focus_index; }
    bool is_external_focused(int external_index) const;
    std::string focused_value() const;

    // Embed mode: skip internal yframe/vscroll/flex so the caller can
    // wrap the viewer element together with other content in a single frame.
    void set_embed(bool embed) { _embed = embed; }
    float scroll() const { return _scroll_ratio; }
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
    bool _show_scrollbar = true;
    bool _active = false;
    int _focus_index = -1;
    int _focused_link = -1;       // derived from _focus_index for DomBuilder
    int _last_focused_link = -1;
    Theme _theme{theme_default()};
    uint64_t _theme_gen = 0;
    uint64_t _built_theme_gen = 0;
    std::function<void(std::string const&, LinkEvent)> _link_callback;
    ftxui::Component _component;
    std::vector<ExternalFocusable> _externals;
    bool _embed = false;
};

} // namespace markdown
