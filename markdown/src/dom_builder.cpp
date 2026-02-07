#include "markdown/dom_builder.hpp"

namespace markdown {
namespace {

ftxui::Element build_node(ASTNode const& node);

ftxui::Elements build_children(ASTNode const& node) {
    ftxui::Elements result;
    for (auto const& child : node.children) {
        result.push_back(build_node(child));
    }
    return result;
}

// Collect inline children into a single hbox (for paragraphs, etc.)
ftxui::Element build_inline_container(ASTNode const& node) {
    ftxui::Elements parts;
    for (auto const& child : node.children) {
        parts.push_back(build_node(child));
    }
    if (parts.empty()) {
        return ftxui::text("");
    }
    if (parts.size() == 1) {
        return std::move(parts[0]);
    }
    return ftxui::hbox(std::move(parts));
}

ftxui::Element build_node(ASTNode const& node) {
    switch (node.type) {
    case NodeType::Document: {
        auto children = build_children(node);
        if (children.empty()) {
            return ftxui::text("");
        }
        return ftxui::vbox(std::move(children));
    }
    case NodeType::Heading: {
        auto content = build_inline_container(node);
        if (node.level == 1) {
            return content | ftxui::bold;
        } else if (node.level == 2) {
            return content | ftxui::bold;
        } else {
            return content | ftxui::bold | ftxui::dim;
        }
    }
    case NodeType::Paragraph:
        return build_inline_container(node);
    case NodeType::Text:
        return ftxui::text(node.text);
    case NodeType::SoftBreak:
        return ftxui::text(" ");
    case NodeType::HardBreak:
        return ftxui::text("\n");
    default:
        // Fallback: render as plain text for unsupported types
        return ftxui::text(node.text);
    }
}

} // namespace

ftxui::Element DomBuilder::build(MarkdownAST const& ast) {
    return build_node(ast);
}

} // namespace markdown
