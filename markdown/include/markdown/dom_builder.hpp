#pragma once

#include <ftxui/dom/elements.hpp>

#include "markdown/ast.hpp"

namespace markdown {

class DomBuilder {
public:
    ftxui::Element build(MarkdownAST const& ast);
};

} // namespace markdown
