#pragma once

#include <string>
#include <vector>

#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/box.hpp>

#include "markdown/ast.hpp"
#include "markdown/theme.hpp"

namespace markdown {

struct LinkTarget {
    std::vector<ftxui::Box> boxes;
    std::string url;
};

class DomBuilder {
public:
    ftxui::Element build(MarkdownAST const& ast, int focused_link = -1,
                         Theme const& theme = theme_default());
    std::vector<LinkTarget> const& link_targets() const { return _link_targets; }

private:
    std::vector<LinkTarget> _link_targets;
};

} // namespace markdown
