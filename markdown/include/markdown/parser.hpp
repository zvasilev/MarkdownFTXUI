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

// Factory — creates a parser backed by cmark-gfm.
// cmark types are fully hidden inside the implementation.
std::unique_ptr<MarkdownParser> make_cmark_parser();

} // namespace markdown
