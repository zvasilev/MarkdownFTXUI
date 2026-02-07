#include "test_helper.hpp"
#include "markdown/dom_builder.hpp"
#include "markdown/parser.hpp"

#include <ftxui/screen/screen.hpp>
#include <ftxui/dom/elements.hpp>

using namespace markdown;

int main() {
    auto parser = make_cmark_parser();
    DomBuilder builder;

    // Test 1: Simple text renders correctly
    {
        auto ast = parser->parse("Hello world");
        auto element = builder.build(ast);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(1));
        ftxui::Render(screen, element);
        auto output = screen.ToString();
        ASSERT_CONTAINS(output, "Hello world");
    }

    // Test 2: Two paragraphs both appear
    {
        auto ast = parser->parse("Line one\n\nLine two");
        auto element = builder.build(ast);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(3));
        ftxui::Render(screen, element);
        auto output = screen.ToString();
        ASSERT_CONTAINS(output, "Line one");
        ASSERT_CONTAINS(output, "Line two");
    }

    // Test 3: Empty document does not crash
    {
        auto ast = parser->parse("");
        auto element = builder.build(ast);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(1));
        ftxui::Render(screen, element);
        // Just verify no crash
        ASSERT_TRUE(true);
    }

    // Test 4: Whitespace-only input does not crash
    {
        auto ast = parser->parse("   \n\n  ");
        auto element = builder.build(ast);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(1));
        ftxui::Render(screen, element);
        ASSERT_TRUE(true);
    }

    // Test 5: Manually constructed AST renders correctly
    {
        MarkdownAST ast{.type = NodeType::Document};
        ASTNode para{.type = NodeType::Paragraph};
        para.children.push_back(ASTNode{.type = NodeType::Text, .text = "Manual"});
        ast.children.push_back(std::move(para));

        auto element = builder.build(ast);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(1));
        ftxui::Render(screen, element);
        auto output = screen.ToString();
        ASSERT_CONTAINS(output, "Manual");
    }

    return 0;
}
