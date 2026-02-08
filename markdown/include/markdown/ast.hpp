#pragma once

#include <string>
#include <vector>

namespace markdown {

enum class NodeType {
    Document,
    Heading,
    Paragraph,
    Text,
    Emphasis,
    Strong,
    Link,
    ListItem,
    BulletList,
    OrderedList,
    CodeInline,
    CodeBlock,
    BlockQuote,
    SoftBreak,
    HardBreak,
    ThematicBreak,
    Image,
};

struct ASTNode {
    NodeType type = NodeType::Document;
    std::string text;
    std::string url;
    std::string info;       // code block language (e.g. "python")
    int level = 0;
    int list_start = 1;
    std::vector<ASTNode> children;
};

using MarkdownAST = ASTNode;

} // namespace markdown
