#pragma once

#include <list>
#include <string>

#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/box.hpp>

#include "markdown/ast.hpp"
#include "markdown/theme.hpp"

namespace markdown {

struct LinkTarget {
    ftxui::Box box{};
    std::string url;
};

class DomBuilder {
public:
    ftxui::Element build(MarkdownAST const& ast, int focused_link = -1,
                         Theme const& theme = theme_default());
    std::list<LinkTarget> const& link_targets() const { return _link_targets; }

private:
    std::list<LinkTarget> _link_targets;
};

} // namespace markdown
