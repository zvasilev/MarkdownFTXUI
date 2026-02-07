#pragma once

#include <memory>
#include <string_view>

#include "markdown/ast.hpp"

namespace markdown {

class MarkdownParser {
public:
    virtual ~MarkdownParser() = default;
    virtual MarkdownAST parse(std::string_view input) = 0;
};

} // namespace markdown
