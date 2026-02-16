#include "markdown/dom_builder.hpp"

#include <string_view>

#include <ftxui/dom/flexbox_config.hpp>

namespace markdown {
namespace {

constexpr int kMaxDepth = 40;

using Links = std::vector<LinkTarget>;

// Iteratively collect all text from a subtree (no recursion — safe at any
// depth).  Used as the plain-text fallback when nesting exceeds kMaxDepth.
std::string collect_text(ASTNode const& root) {
    std::string result;
    std::vector<ASTNode const*> stack{&root};
    while (!stack.empty()) {
        auto* n = stack.back();
        stack.pop_back();
        if (!n->text.empty()) result += n->text;
        if (n->type == NodeType::SoftBreak) result += ' ';
        if (n->type == NodeType::HardBreak) result += '\n';
        // Push children in reverse so leftmost is processed first.
        for (auto it = n->children.rbegin(); it != n->children.rend(); ++it) {
            stack.push_back(&*it);
        }
    }
    return result;
}

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

// Register a link: create a LinkTarget, wrap each element in elems[from..]
// with reflect for click detection, and apply focus to the first element.
void register_link(Links& links, ftxui::Elements& elems, size_t from,
                   std::string const& url, bool is_focused) {
    links.emplace_back(LinkTarget{.url = url});
    auto& target = links.back();
    size_t count = elems.size() - from;
    target.boxes.resize(count);
    for (size_t i = from; i < elems.size(); ++i) {
        elems[i] = elems[i] | ftxui::reflect(target.boxes[i - from]);
    }
    if (is_focused && count > 0) {
        elems[from] = elems[from] | ftxui::focus;
    }
}

ftxui::Element build_node(ASTNode const& node, int depth, int qd, int mqd,
                          Links& links, int focused_link,
                          Theme const& theme);

ftxui::Elements build_children(ASTNode const& node, int depth, int qd,
                               int mqd, Links& links, int focused_link,
                               Theme const& theme) {
    ftxui::Elements result;
    for (auto const& child : node.children) {
        result.push_back(build_node(child, depth, qd, mqd, links,
                                    focused_link, theme));
    }
    return result;
}

// Collect inline children into a single hbox (for paragraphs, etc.)
ftxui::Element build_inline_container(ASTNode const& node, int depth, int qd,
                                      int mqd, Links& links, int focused_link,
                                      Theme const& theme) {
    ftxui::Elements parts;
    for (auto const& child : node.children) {
        parts.push_back(build_node(child, depth, qd, mqd, links,
                                   focused_link, theme));
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
void collect_inline_words(ASTNode const& node, int depth, int qd, int mqd,
                          ftxui::Elements& words,
                          ftxui::Decorator style,
                          Links& links, int focused_link,
                          Theme const& theme) {
    if (depth > kMaxDepth) {
        auto text = collect_text(node);
        if (!text.empty()) words.push_back(ftxui::text(text) | style);
        return;
    }
    for (auto const& child : node.children) {
        switch (child.type) {
        case NodeType::Text: {
            auto const& t = child.text;
            size_t pos = 0;
            while (pos < t.size()) {
                size_t space_start = pos;
                while (pos < t.size() && t[pos] == ' ') ++pos;
                if (pos >= t.size()) {
                    // Trailing spaces: emit separator for next sibling.
                    if (space_start < pos && !words.empty()) {
                        words.push_back(ftxui::text(" ") | style);
                    }
                    break;
                }
                auto end = t.find(' ', pos);
                if (end == std::string::npos) end = t.size();
                std::string word;
                bool needs_space = (space_start < pos);
                word.reserve((end - pos) + (needs_space ? 1 : 0));
                if (needs_space) word += ' ';
                word.append(t.data() + pos, end - pos);
                words.push_back(ftxui::text(std::move(word)) | style);
                pos = end;
            }
            break;
        }
        case NodeType::SoftBreak:
            words.push_back(ftxui::text(" ") | style);
            break;
        case NodeType::HardBreak:
            break; // handled by build_wrapping_container
        case NodeType::Strong:
            collect_inline_words(child, depth + 1, qd, mqd, words,
                                 style | ftxui::bold, links, focused_link,
                                 theme);
            break;
        case NodeType::Emphasis:
            collect_inline_words(child, depth + 1, qd, mqd, words,
                                 style | ftxui::italic, links, focused_link,
                                 theme);
            break;
        case NodeType::Link: {
            bool is_focused = is_next_link_focused(links, focused_link);
            auto ls = link_style(is_focused, style, theme);
            size_t before = words.size();
            collect_inline_words(child, depth + 1, qd, mqd, words,
                                 ls, links, focused_link, theme);
            register_link(links, words, before, child.url, is_focused);
            break;
        }
        case NodeType::CodeInline:
            words.push_back(ftxui::text(child.text) | theme.code_inline
                            | style);
            break;
        default:
            words.push_back(build_node(child, depth, qd, mqd, links,
                                       focused_link, theme) | style);
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

// Check if a node contains any HardBreak children.
bool has_hard_break(ASTNode const& node) {
    for (auto const& child : node.children) {
        if (child.type == NodeType::HardBreak) return true;
    }
    return false;
}

// Build a flexbox row from a flat list of word elements.
ftxui::Element words_to_element(ftxui::Elements& words) {
    static const auto wrap_config = ftxui::FlexboxConfig().SetGap(0, 0);
    if (words.empty()) return ftxui::text("");
    // Always use flexbox — even for a single element.  Without this,
    // a lone underlined link stretches to the full vbox width and its
    // underline extends across the entire line (looks like a separator).
    return ftxui::flexbox(std::move(words), wrap_config);
}

// Wrapping version of build_inline_container for block-level paragraphs.
// Splits all inline content into word-level flexbox items for line wrapping.
// HardBreak nodes force a new line by splitting into separate flexbox rows.
ftxui::Element build_wrapping_container(ASTNode const& node, int depth, int qd,
                                        int mqd, Links& links,
                                        int focused_link,
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

    // If no hard breaks, single flexbox row (common case).
    if (!has_hard_break(node)) {
        ftxui::Elements words;
        collect_inline_words(node, depth, qd, mqd, words, ftxui::nothing,
                             links, focused_link, theme);
        return words_to_element(words);
    }

    // Split at HardBreak boundaries: each segment becomes its own row.
    // Build a temporary ASTNode per segment and collect words from it.
    ftxui::Elements rows;
    ASTNode segment{.type = node.type};
    auto flush_segment = [&] {
        if (segment.children.empty()) {
            rows.push_back(ftxui::text(""));
            return;
        }
        ftxui::Elements words;
        collect_inline_words(segment, depth, qd, mqd, words, ftxui::nothing,
                             links, focused_link, theme);
        rows.push_back(words_to_element(words));
        segment.children.clear();
    };

    for (auto const& child : node.children) {
        if (child.type == NodeType::HardBreak) {
            flush_segment();
        } else {
            segment.children.push_back(child);
        }
    }
    flush_segment();

    if (rows.size() == 1) return std::move(rows[0]);
    return ftxui::vbox(std::move(rows));
}

// Build a ListItem: first Paragraph gets bullet/number prefix,
// subsequent children (nested lists) rendered below with indentation.
ftxui::Element build_list_item(ASTNode const& node, int depth, int qd,
                               int mqd, std::string const& prefix,
                               Links& links, int focused_link,
                               Theme const& theme) {
    std::string indent(depth * 2, ' ');

    ftxui::Elements rows;
    bool first_para = true;
    for (auto const& child : node.children) {
        if (first_para && (child.type == NodeType::Paragraph ||
                           child.type == NodeType::Text)) {
            // First paragraph: render with wrapping, bullet/number prefix
            auto content = build_wrapping_container(child, depth, qd, mqd,
                                                    links, focused_link, theme);
            rows.push_back(ftxui::hbox({
                ftxui::text(indent + prefix),
                content | ftxui::flex,
            }));
            first_para = false;
        } else {
            // Nested lists or additional paragraphs
            rows.push_back(build_node(child, depth, qd, mqd, links,
                                      focused_link, theme));
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

ftxui::Element build_document(ASTNode const& node, int depth, int qd, int mqd,
                              Links& links, int focused_link,
                              Theme const& theme) {
    auto children = build_children(node, depth, qd, mqd, links, focused_link,
                                   theme);
    if (children.empty()) return ftxui::text("");
    ftxui::Elements spaced;
    for (size_t i = 0; i < children.size(); ++i) {
        if (i > 0) spaced.push_back(ftxui::text(""));
        spaced.push_back(std::move(children[i]));
    }
    return ftxui::vbox(std::move(spaced));
}

ftxui::Element build_heading(ASTNode const& node, int depth, int qd, int mqd,
                             Links& links, int focused_link,
                             Theme const& theme) {
    auto content = build_inline_container(node, depth, qd, mqd, links,
                                          focused_link, theme);
    if (node.level == 1) return content | theme.heading1;
    if (node.level == 2) return content | theme.heading2;
    return content | theme.heading3;
}

ftxui::Element build_link(ASTNode const& node, int depth, int qd, int mqd,
                          Links& links, int focused_link,
                          Theme const& theme) {
    bool is_focused = is_next_link_focused(links, focused_link);
    auto el = build_inline_container(node, depth, qd, mqd, links,
                                     focused_link, theme)
        | link_style(is_focused, ftxui::nothing, theme);
    ftxui::Elements elems;
    elems.push_back(std::move(el));
    register_link(links, elems, 0, node.url, is_focused);
    return std::move(elems[0]);
}

ftxui::Element build_bullet_list(ASTNode const& node, int depth, int qd,
                                 int mqd, Links& links, int focused_link,
                                 Theme const& theme) {
    ftxui::Elements items;
    for (auto const& child : node.children) {
        items.push_back(build_list_item(child, depth + 1, qd, mqd, "\u2022 ",
                                        links, focused_link, theme));
    }
    return ftxui::vbox(std::move(items));
}

ftxui::Element build_ordered_list(ASTNode const& node, int depth, int qd,
                                  int mqd, Links& links, int focused_link,
                                  Theme const& theme) {
    ftxui::Elements items;
    int num = node.list_start;
    for (auto const& child : node.children) {
        items.push_back(build_list_item(child, depth + 1, qd, mqd,
                                        std::to_string(num++) + ". ",
                                        links, focused_link, theme));
    }
    return ftxui::vbox(std::move(items));
}

ftxui::Element build_blockquote(ASTNode const& node, int depth, int qd,
                                int mqd, Links& links, int focused_link,
                                Theme const& theme) {
    auto content = ftxui::vbox(build_children(node, depth, qd + 1, mqd, links,
                                              focused_link, theme));
    // Cap visual indentation at max_quote_depth; content still renders.
    if (qd >= mqd) {
        return content | theme.blockquote;
    }
    return ftxui::hbox({
        ftxui::text("\u2502 "),
        content | theme.blockquote,
    });
}

ftxui::Element build_code_block(ASTNode const& node, Theme const& theme) {
    std::string_view code = node.text;
    if (!code.empty() && code.back() == '\n') code.remove_suffix(1);
    ftxui::Elements lines;
    size_t start = 0;
    while (start <= code.size()) {
        auto end = code.find('\n', start);
        if (end == std::string_view::npos) {
            lines.push_back(ftxui::text(std::string(code.substr(start))));
            break;
        }
        lines.push_back(
            ftxui::text(std::string(code.substr(start, end - start))));
        start = end + 1;
    }
    if (lines.empty()) lines.push_back(ftxui::text(""));
    auto content = ftxui::vbox(std::move(lines)) | theme.code_block;
    if (!node.info.empty()) {
        return ftxui::window(ftxui::text(" " + node.info + " ") | ftxui::dim,
                             content);
    }
    return content | ftxui::border;
}

ftxui::Element build_image(ASTNode const& node, int depth, int qd, int mqd,
                           Links& links, int focused_link,
                           Theme const& theme) {
    auto alt = build_inline_container(node, depth, qd, mqd, links,
                                      focused_link, theme);
    return ftxui::hbox({
        ftxui::text("[IMG: ") | ftxui::dim,
        alt,
        ftxui::text("]") | ftxui::dim,
    });
}

ftxui::Element build_node(ASTNode const& node, int depth, int qd, int mqd,
                          Links& links, int focused_link,
                          Theme const& theme) {
    // Depth guard: fall back to plain text to prevent stack overflow.
    if (depth + qd > kMaxDepth) {
        return ftxui::paragraph(collect_text(node));
    }

    switch (node.type) {
    case NodeType::Document:
        return build_document(node, depth, qd, mqd, links, focused_link,
                              theme);
    case NodeType::Heading:
        return build_heading(node, depth, qd, mqd, links, focused_link,
                             theme);
    case NodeType::Paragraph:
        return build_wrapping_container(node, depth, qd, mqd, links,
                                        focused_link, theme);
    case NodeType::Strong:
        return build_inline_container(node, depth, qd, mqd, links,
                                      focused_link, theme) | ftxui::bold;
    case NodeType::Emphasis:
        return build_inline_container(node, depth, qd, mqd, links,
                                      focused_link, theme) | ftxui::italic;
    case NodeType::Link:
        return build_link(node, depth, qd, mqd, links, focused_link, theme);
    case NodeType::BulletList:
        return build_bullet_list(node, depth, qd, mqd, links, focused_link,
                                 theme);
    case NodeType::OrderedList:
        return build_ordered_list(node, depth, qd, mqd, links, focused_link,
                                  theme);
    case NodeType::ListItem:
        return build_list_item(node, depth, qd, mqd, "\u2022 ", links,
                               focused_link, theme);
    case NodeType::BlockQuote:
        return build_blockquote(node, depth, qd, mqd, links, focused_link,
                                theme);
    case NodeType::CodeInline:
        return ftxui::text(node.text) | theme.code_inline;
    case NodeType::CodeBlock:
        return build_code_block(node, theme);
    case NodeType::ThematicBreak:
        return ftxui::separator();
    case NodeType::Image:
        return build_image(node, depth, qd, mqd, links, focused_link, theme);
    case NodeType::Text:
        return ftxui::text(node.text);
    case NodeType::SoftBreak:
        return ftxui::text(" ");
    case NodeType::HardBreak:
        return ftxui::text("");
    default:
        return ftxui::text(node.text);
    }
}

} // namespace

ftxui::Element DomBuilder::build(MarkdownAST const& ast, int focused_link,
                                 Theme const& theme) {
    _link_targets.clear();
    auto result = build_node(ast, 0, 0, _max_quote_depth, _link_targets,
                             focused_link, theme);

    // Build flat sorted index for O(log n) click detection.
    _flat_boxes.clear();
    for (int i = 0; i < static_cast<int>(_link_targets.size()); ++i) {
        for (auto const& box : _link_targets[i].boxes) {
            _flat_boxes.push_back({box, i});
        }
    }
    std::sort(_flat_boxes.begin(), _flat_boxes.end(),
              [](FlatLinkBox const& a, FlatLinkBox const& b) {
                  return a.box.y_min < b.box.y_min;
              });

    return result;
}

} // namespace markdown
