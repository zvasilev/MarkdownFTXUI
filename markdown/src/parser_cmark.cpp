#include "markdown/parser.hpp"

namespace markdown {

class CmarkParser : public MarkdownParser {
public:
    MarkdownAST parse(std::string_view /*input*/) override {
        // Stub — will be implemented in Phase 1
        return MarkdownAST{.type = NodeType::Document};
    }
};

} // namespace markdown
