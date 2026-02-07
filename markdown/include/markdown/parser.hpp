#pragma once

#include <memory>
#include <string_view>

#include "markdown/ast.hpp"

namespace markdown {

class MarkdownParser {
public:
    virtual ~MarkdownParser() = default;

    // Returns false if parsing failed (out will contain a raw-text fallback).
    virtual bool parse(std::string_view input, MarkdownAST& out) = 0;

    // Convenience overload — returns AST directly, ignoring success/failure.
    MarkdownAST parse(std::string_view input) {
        MarkdownAST ast;
        parse(input, ast);
        return ast;
    }
};

// Factory — creates a parser backed by cmark-gfm.
// cmark types are fully hidden inside the implementation.
std::unique_ptr<MarkdownParser> make_cmark_parser();

} // namespace markdown
