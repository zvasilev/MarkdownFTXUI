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
    int _focused_link = -1;
    int _last_focused_link = -1;
    Theme _theme{theme_default()};
    uint64_t _theme_gen = 0;
    uint64_t _built_theme_gen = 0;
    std::function<void(std::string const&, LinkEvent)> _link_callback;
    ftxui::Component _component;
};

} // namespace markdown
