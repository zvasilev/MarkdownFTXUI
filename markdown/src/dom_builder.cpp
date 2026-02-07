#include "markdown/dom_builder.hpp"

#include <sstream>

#include <ftxui/dom/flexbox_config.hpp>

namespace markdown {
namespace {

ftxui::Element build_node(ASTNode const& node, int depth = 0);

ftxui::Elements build_children(ASTNode const& node, int depth) {
    ftxui::Elements result;
    for (auto const& child : node.children) {
        result.push_back(build_node(child, depth));
    }
    return result;
}

// Collect inline children into a single hbox (for paragraphs, etc.)
ftxui::Element build_inline_container(ASTNode const& node, int depth) {
    ftxui::Elements parts;
    for (auto const& child : node.children) {
        parts.push_back(build_node(child, depth));
    }
    if (parts.empty()) {
        return ftxui::text("");
    }
    if (parts.size() == 1) {
        return std::move(parts[0]);
    }
    return ftxui::hbox(std::move(parts));
}

// Recursively collect words from inline AST nodes, preserving decorators.
// Each word becomes a separate flexbox item so wrapping works at word
// boundaries even inside bold/italic/link runs.
void collect_inline_words(ASTNode const& node, int depth,
                          ftxui::Elements& words,
                          ftxui::Decorator style) {
    for (auto const& child : node.children) {
        switch (child.type) {
        case NodeType::Text: {
            std::istringstream ss(child.text);
            std::string word;
            while (ss >> word) {
                words.push_back(ftxui::text(word) | style);
            }
            break;
        }
        case NodeType::SoftBreak:
            break; // flexbox gap handles spacing
        case NodeType::Strong:
            collect_inline_words(child, depth, words,
                                 style | ftxui::bold);
            break;
        case NodeType::Emphasis:
            collect_inline_words(child, depth, words,
                                 style | ftxui::italic);
            break;
        case NodeType::Link:
            collect_inline_words(child, depth, words,
                                 style | ftxui::underlined);
            break;
        case NodeType::CodeInline:
            words.push_back(ftxui::text(child.text) | ftxui::inverted | style);
            break;
        default:
            words.push_back(build_node(child, depth) | style);
            break;
        }
    }
}

// Wrapping version of build_inline_container for block-level paragraphs.
// Splits all inline content into word-level flexbox items for line wrapping.
ftxui::Element build_wrapping_container(ASTNode const& node, int depth) {
    static const auto wrap_config = ftxui::FlexboxConfig().SetGap(1, 0);
    ftxui::Elements words;
    collect_inline_words(node, depth, words, ftxui::nothing);
    if (words.empty()) return ftxui::text("");
    if (words.size() == 1) return std::move(words[0]);
    return ftxui::flexbox(std::move(words), wrap_config);
}

// Build a ListItem: first Paragraph gets bullet/number prefix,
// subsequent children (nested lists) rendered below with indentation.
ftxui::Element build_list_item(ASTNode const& node, int depth,
                               std::string const& prefix) {
    std::string indent(depth * 2, ' ');

    ftxui::Elements rows;
    bool first_para = true;
    for (auto const& child : node.children) {
        if (first_para && (child.type == NodeType::Paragraph ||
                           child.type == NodeType::Text)) {
            // First paragraph: render inline with bullet/number prefix
            auto content = build_inline_container(child, depth);
            rows.push_back(ftxui::hbox({
                ftxui::text(indent + prefix),
                content,
            }));
            first_para = false;
        } else {
            // Nested lists or additional paragraphs
            rows.push_back(build_node(child, depth));
        }
    }
    if (rows.empty()) {
        return ftxui::text(indent + prefix);
    }
    if (rows.size() == 1) {
        return std::move(rows[0]);
    }
    return ftxui::vbox(std::move(rows));
}

ftxui::Element build_node(ASTNode const& node, int depth) {
    switch (node.type) {
    case NodeType::Document: {
        auto children = build_children(node, depth);
        if (children.empty()) {
            return ftxui::text("");
        }
        // Insert blank line between top-level blocks for readability
        ftxui::Elements spaced;
        for (size_t i = 0; i < children.size(); ++i) {
            if (i > 0) {
                spaced.push_back(ftxui::text(""));
            }
            spaced.push_back(std::move(children[i]));
        }
        return ftxui::vbox(std::move(spaced));
    }
    case NodeType::Heading: {
        auto content = build_inline_container(node, depth);
        if (node.level == 1) {
            return content | ftxui::bold | ftxui::underlined;
        } else if (node.level == 2) {
            return content | ftxui::bold;
        } else {
            return content | ftxui::bold | ftxui::dim;
        }
    }
    case NodeType::Paragraph:
        return build_wrapping_container(node, depth);
    case NodeType::Strong:
        return build_inline_container(node, depth) | ftxui::bold;
    case NodeType::Emphasis:
        return build_inline_container(node, depth) | ftxui::italic;
    case NodeType::Link:
        return build_inline_container(node, depth) | ftxui::underlined;
    case NodeType::BulletList: {
        ftxui::Elements items;
        for (auto const& child : node.children) {
            items.push_back(build_list_item(child, depth + 1, "\u2022 "));
        }
        return ftxui::vbox(std::move(items));
    }
    case NodeType::OrderedList: {
        ftxui::Elements items;
        int num = node.list_start;
        for (auto const& child : node.children) {
            items.push_back(
                build_list_item(child, depth + 1,
                                std::to_string(num++) + ". "));
        }
        return ftxui::vbox(std::move(items));
    }
    case NodeType::ListItem: {
        // Fallback if ListItem rendered outside of list context
        return build_list_item(node, depth, "\u2022 ");
    }
    case NodeType::BlockQuote: {
        auto content = ftxui::vbox(build_children(node, depth));
        return ftxui::hbox({
            ftxui::text("\u2502 "),
            content | ftxui::dim,
        });
    }
    case NodeType::CodeInline:
        return ftxui::text(node.text) | ftxui::inverted;
    case NodeType::CodeBlock: {
        // Strip trailing newline that cmark-gfm includes in the literal
        std::string code = node.text;
        if (!code.empty() && code.back() == '\n') {
            code.pop_back();
        }
        // Split into lines for vbox rendering
        ftxui::Elements lines;
        size_t start = 0;
        while (start <= code.size()) {
            auto end = code.find('\n', start);
            if (end == std::string::npos) {
                lines.push_back(ftxui::text(code.substr(start)));
                break;
            }
            lines.push_back(ftxui::text(code.substr(start, end - start)));
            start = end + 1;
        }
        if (lines.empty()) {
            lines.push_back(ftxui::text(""));
        }
        return ftxui::vbox(std::move(lines)) | ftxui::border;
    }
    case NodeType::ThematicBreak:
        return ftxui::separator();
    case NodeType::Image: {
        auto alt = build_inline_container(node, depth);
        return ftxui::hbox({
            ftxui::text("[IMG: ") | ftxui::dim,
            alt,
            ftxui::text("]") | ftxui::dim,
        });
    }
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
