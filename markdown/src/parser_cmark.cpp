#include "markdown/parser.hpp"

#include <cmark-gfm.h>

namespace markdown {
namespace {

ASTNode convert_node(cmark_node* node) {
    ASTNode result;

    switch (cmark_node_get_type(node)) {
    case CMARK_NODE_DOCUMENT:
        result.type = NodeType::Document;
        break;
    case CMARK_NODE_PARAGRAPH:
        result.type = NodeType::Paragraph;
        break;
    case CMARK_NODE_TEXT: {
        result.type = NodeType::Text;
        auto const* literal = cmark_node_get_literal(node);
        if (literal) {
            result.text = literal;
        }
        return result; // leaf node, no children
    }
    case CMARK_NODE_SOFTBREAK:
        result.type = NodeType::SoftBreak;
        return result;
    case CMARK_NODE_LINEBREAK:
        result.type = NodeType::HardBreak;
        return result;
    case CMARK_NODE_HEADING:
        result.type = NodeType::Heading;
        result.level = cmark_node_get_heading_level(node);
        break;
    case CMARK_NODE_EMPH:
        result.type = NodeType::Emphasis;
        break;
    case CMARK_NODE_STRONG:
        result.type = NodeType::Strong;
        break;
    case CMARK_NODE_LINK: {
        result.type = NodeType::Link;
        auto const* url = cmark_node_get_url(node);
        if (url) {
            result.url = url;
        }
        break;
    }
    case CMARK_NODE_LIST:
        if (cmark_node_get_list_type(node) == CMARK_ORDERED_LIST) {
            result.type = NodeType::OrderedList;
            result.list_start = cmark_node_get_list_start(node);
        } else {
            result.type = NodeType::BulletList;
        }
        break;
    case CMARK_NODE_ITEM:
        result.type = NodeType::ListItem;
        break;
    case CMARK_NODE_CODE: {
        result.type = NodeType::CodeInline;
        auto const* literal = cmark_node_get_literal(node);
        if (literal) {
            result.text = literal;
        }
        return result;
    }
    case CMARK_NODE_BLOCK_QUOTE:
        result.type = NodeType::BlockQuote;
        break;
    case CMARK_NODE_THEMATIC_BREAK:
        result.type = NodeType::ThematicBreak;
        return result;
    case CMARK_NODE_IMAGE: {
        result.type = NodeType::Image;
        auto const* url = cmark_node_get_url(node);
        if (url) {
            result.url = url;
        }
        break; // children become alt text
    }
    case CMARK_NODE_HTML_INLINE: {
        result.type = NodeType::Text;
        auto const* literal = cmark_node_get_literal(node);
        if (literal) {
            result.text = literal;
        }
        return result;
    }
    case CMARK_NODE_HTML_BLOCK: {
        result.type = NodeType::Text;
        auto const* literal = cmark_node_get_literal(node);
        if (literal) {
            result.text = literal;
        }
        return result;
    }
    case CMARK_NODE_CODE_BLOCK: {
        result.type = NodeType::CodeBlock;
        auto const* literal = cmark_node_get_literal(node);
        if (literal) {
            result.text = literal;
        }
        auto const* fence_info = cmark_node_get_fence_info(node);
        if (fence_info && fence_info[0]) {
            result.info = fence_info;
        }
        return result;
    }
    default:
        // Unsupported node types rendered as plain text
        result.type = NodeType::Paragraph;
        break;
    }

    // Recurse into children
    for (auto* child = cmark_node_first_child(node); child;
         child = cmark_node_next(child)) {
        result.children.push_back(convert_node(child));
    }

    return result;
}

class CmarkParser : public MarkdownParser {
public:
    bool parse(std::string_view input, MarkdownAST& out) override {
        cmark_node* doc = cmark_parse_document(
            input.data(), input.size(), CMARK_OPT_DEFAULT);

        if (!doc) {
            // Parsing failed — provide raw text as fallback
            out = ASTNode{.type = NodeType::Document};
            ASTNode para{.type = NodeType::Paragraph};
            para.children.push_back(
                ASTNode{.type = NodeType::Text, .text = std::string(input)});
            out.children.push_back(std::move(para));
            return false;
        }

        out = convert_node(doc);
        cmark_node_free(doc);
        return true;
    }
};

} // namespace

std::unique_ptr<MarkdownParser> make_cmark_parser() {
    return std::make_unique<CmarkParser>();
}

} // namespace markdown
