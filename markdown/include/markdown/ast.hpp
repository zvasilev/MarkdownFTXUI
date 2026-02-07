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
    StrongEmphasis,
    Link,
    ListItem,
    BulletList,
    CodeInline,
    BlockQuote,
    SoftBreak,
    HardBreak,
};

struct ASTNode {
    NodeType type = NodeType::Document;
    std::string text;
    std::string url;
    int level = 0;
    std::vector<ASTNode> children;
};

using MarkdownAST = ASTNode;

} // namespace markdown
