#include "markdown/dom_builder.hpp"

#include <ftxui/dom/flexbox_config.hpp>

namespace markdown {
namespace {

using Links = std::vector<LinkTarget>;

// Returns true if the next link to be inserted matches focused_link.
bool is_next_link_focused(Links const& links, int focused_link) {
    return static_cast<int>(links.size()) == focused_link;
}

// Compute the decorator for a link based on whether it is focused.
ftxui::Decorator link_style(bool is_focused, ftxui::Decorator base,
                            Theme const& theme) {
    if (is_focused) return base | ftxui::underlined | ftxui::inverted;
    return base | ftxui::underlined | theme.link;
}

ftxui::Element build_node(ASTNode const& node, int depth, Links& links,
                          int focused_link, Theme const& theme);

ftxui::Elements build_children(ASTNode const& node, int depth, Links& links,
                               int focused_link, Theme const& theme) {
    ftxui::Elements result;
    for (auto const& child : node.children) {
        result.push_back(build_node(child, depth, links, focused_link, theme));
    }
    return result;
}

// Collect inline children into a single hbox (for paragraphs, etc.)
ftxui::Element build_inline_container(ASTNode const& node, int depth,
                                      Links& links, int focused_link,
                                      Theme const& theme) {
    ftxui::Elements parts;
    for (auto const& child : node.children) {
        parts.push_back(build_node(child, depth, links, focused_link, theme));
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
                          ftxui::Decorator style,
                          Links& links, int focused_link,
                          Theme const& theme) {
    for (auto const& child : node.children) {
        switch (child.type) {
        case NodeType::Text: {
            auto const& t = child.text;
            size_t pos = 0;
            while (pos < t.size()) {
                while (pos < t.size() && t[pos] == ' ') ++pos;
                if (pos >= t.size()) break;
                auto end = t.find(' ', pos);
                if (end == std::string::npos) end = t.size();
                words.push_back(
                    ftxui::text(t.substr(pos, end - pos)) | style);
                pos = end;
            }
            break;
        }
        case NodeType::SoftBreak:
            break; // flexbox gap handles spacing
        case NodeType::Strong:
            collect_inline_words(child, depth, words,
                                 style | ftxui::bold, links, focused_link,
                                 theme);
            break;
        case NodeType::Emphasis:
            collect_inline_words(child, depth, words,
                                 style | ftxui::italic, links, focused_link,
                                 theme);
            break;
        case NodeType::Link: {
            bool is_focused = is_next_link_focused(links, focused_link);
            auto ls = link_style(is_focused, style, theme);
            // Collect link words with underline, then wrap in reflect
            size_t before = words.size();
            collect_inline_words(child, depth, words,
                                 ls, links, focused_link, theme);
            // Wrap every word of this link with reflect for click detection
            links.emplace_back(LinkTarget{.url = child.url});
            auto& target = links.back();
            size_t word_count = words.size() - before;
            target.boxes.resize(word_count);
            bool first_word = true;
            for (size_t i = before; i < words.size(); ++i) {
                words[i] = words[i] | ftxui::reflect(target.boxes[i - before]);
                if (is_focused && first_word) {
                    words[i] = words[i] | ftxui::focus;
                    first_word = false;
                }
            }
            break;
        }
        case NodeType::CodeInline:
            words.push_back(ftxui::text(child.text) | theme.code_inline
                            | style);
            break;
        default:
            words.push_back(build_node(child, depth, links, focused_link,
                                       theme) | style);
            break;
        }
    }
}

// Check if a paragraph node contains only plain text (Text + SoftBreak).
bool is_plain_text_paragraph(ASTNode const& node) {
    for (auto const& child : node.children) {
        if (child.type != NodeType::Text && child.type != NodeType::SoftBreak) {
            return false;
        }
    }
    return true;
}

// Wrapping version of build_inline_container for block-level paragraphs.
// Splits all inline content into word-level flexbox items for line wrapping.
ftxui::Element build_wrapping_container(ASTNode const& node, int depth,
                                        Links& links, int focused_link,
                                        Theme const& theme) {
    // Fast path: plain text paragraphs use ftxui::paragraph() directly,
    // avoiding per-word flexbox overhead.
    if (is_plain_text_paragraph(node)) {
        std::string combined;
        for (auto const& child : node.children) {
            if (child.type == NodeType::Text) {
                if (!combined.empty() && combined.back() != ' ') {
                    combined += ' ';
                }
                combined += child.text;
            } else if (child.type == NodeType::SoftBreak) {
                if (!combined.empty() && combined.back() != ' ') {
                    combined += ' ';
                }
            }
        }
        return ftxui::paragraph(combined);
    }

    static const auto wrap_config = ftxui::FlexboxConfig().SetGap(1, 0);
    ftxui::Elements words;
    collect_inline_words(node, depth, words, ftxui::nothing, links,
                         focused_link, theme);
    if (words.empty()) return ftxui::text("");
    if (words.size() == 1) return std::move(words[0]);
    return ftxui::flexbox(std::move(words), wrap_config);
}

// Build a ListItem: first Paragraph gets bullet/number prefix,
// subsequent children (nested lists) rendered below with indentation.
ftxui::Element build_list_item(ASTNode const& node, int depth,
                               std::string const& prefix, Links& links,
                               int focused_link, Theme const& theme) {
    std::string indent(depth * 2, ' ');

    ftxui::Elements rows;
    bool first_para = true;
    for (auto const& child : node.children) {
        if (first_para && (child.type == NodeType::Paragraph ||
                           child.type == NodeType::Text)) {
            // First paragraph: render inline with bullet/number prefix
            auto content = build_inline_container(child, depth, links,
                                                  focused_link, theme);
            rows.push_back(ftxui::hbox({
                ftxui::text(indent + prefix),
                content,
            }));
            first_para = false;
        } else {
            // Nested lists or additional paragraphs
            rows.push_back(build_node(child, depth, links, focused_link,
                                      theme));
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

ftxui::Element build_node(ASTNode const& node, int depth, Links& links,
                          int focused_link, Theme const& theme) {
    switch (node.type) {
    case NodeType::Document: {
        auto children = build_children(node, depth, links, focused_link,
                                       theme);
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
        auto content = build_inline_container(node, depth, links,
                                              focused_link, theme);
        if (node.level == 1) return content | theme.heading1;
        if (node.level == 2) return content | theme.heading2;
        return content | theme.heading3;
    }
    case NodeType::Paragraph:
        return build_wrapping_container(node, depth, links, focused_link,
                                        theme);
    case NodeType::Strong:
        return build_inline_container(node, depth, links, focused_link,
                                      theme) | ftxui::bold;
    case NodeType::Emphasis:
        return build_inline_container(node, depth, links, focused_link,
                                      theme) | ftxui::italic;
    case NodeType::Link: {
        bool is_focused = is_next_link_focused(links, focused_link);
        auto el = build_inline_container(node, depth, links, focused_link,
                                         theme)
            | link_style(is_focused, ftxui::nothing, theme);
        if (is_focused) el = el | ftxui::focus;
        links.emplace_back(LinkTarget{.url = node.url});
        links.back().boxes.emplace_back();
        return el | ftxui::reflect(links.back().boxes.back());
    }
    case NodeType::BulletList: {
        ftxui::Elements items;
        for (auto const& child : node.children) {
            items.push_back(build_list_item(child, depth + 1, "\u2022 ",
                                            links, focused_link, theme));
        }
        return ftxui::vbox(std::move(items));
    }
    case NodeType::OrderedList: {
        ftxui::Elements items;
        int num = node.list_start;
        for (auto const& child : node.children) {
            items.push_back(
                build_list_item(child, depth + 1,
                                std::to_string(num++) + ". ", links,
                                focused_link, theme));
        }
        return ftxui::vbox(std::move(items));
    }
    case NodeType::ListItem: {
        // Fallback if ListItem rendered outside of list context
        return build_list_item(node, depth, "\u2022 ", links, focused_link,
                               theme);
    }
    case NodeType::BlockQuote: {
        auto content = ftxui::vbox(build_children(node, depth, links,
                                                  focused_link, theme));
        return ftxui::hbox({
            ftxui::text("\u2502 "),
            content | theme.blockquote,
        });
    }
    case NodeType::CodeInline:
        return ftxui::text(node.text) | theme.code_inline;
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
        return ftxui::vbox(std::move(lines)) | theme.code_block
            | ftxui::border;
    }
    case NodeType::ThematicBreak:
        return ftxui::separator();
    case NodeType::Image: {
        auto alt = build_inline_container(node, depth, links, focused_link,
                                          theme);
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

ftxui::Element DomBuilder::build(MarkdownAST const& ast, int focused_link,
                                 Theme const& theme) {
    _link_targets.clear();
    return build_node(ast, 0, _link_targets, focused_link, theme);
}

} // namespace markdown
