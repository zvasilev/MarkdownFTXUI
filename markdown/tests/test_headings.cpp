#include "test_helper.hpp"
#include "markdown/parser.hpp"
#include "markdown/dom_builder.hpp"

#include <ftxui/screen/screen.hpp>
#include <ftxui/dom/elements.hpp>

using namespace markdown;

int main() {
    auto parser = make_cmark_parser();
    DomBuilder builder;

    // Test 1: H1 parsed correctly
    {
        auto ast = parser->parse("# Title");
        ASSERT_EQ(ast.type, NodeType::Document);
        ASSERT_EQ(ast.children.size(), 1u);
        ASSERT_EQ(ast.children[0].type, NodeType::Heading);
        ASSERT_EQ(ast.children[0].level, 1);
        ASSERT_EQ(ast.children[0].children.size(), 1u);
        ASSERT_EQ(ast.children[0].children[0].type, NodeType::Text);
        ASSERT_EQ(ast.children[0].children[0].text, "Title");
    }

    // Test 2: H2 with body text
    {
        auto ast = parser->parse("## Section\n\nBody text");
        ASSERT_EQ(ast.children.size(), 2u);
        ASSERT_EQ(ast.children[0].type, NodeType::Heading);
        ASSERT_EQ(ast.children[0].level, 2);
        ASSERT_EQ(ast.children[0].children[0].text, "Section");
        ASSERT_EQ(ast.children[1].type, NodeType::Paragraph);
        ASSERT_EQ(ast.children[1].children[0].text, "Body text");
    }

    // Test 3: H6 (deepest level)
    {
        auto ast = parser->parse("###### Deep");
        ASSERT_EQ(ast.children.size(), 1u);
        ASSERT_EQ(ast.children[0].type, NodeType::Heading);
        ASSERT_EQ(ast.children[0].level, 6);
        ASSERT_EQ(ast.children[0].children[0].text, "Deep");
    }

    // Test 4: All heading levels
    {
        for (int level = 1; level <= 6; ++level) {
            std::string input(static_cast<size_t>(level), '#');
            input += " Level";
            auto ast = parser->parse(input);
            ASSERT_EQ(ast.children[0].type, NodeType::Heading);
            ASSERT_EQ(ast.children[0].level, level);
        }
    }

    // Test 5: H1 renders with bold
    {
        auto ast = parser->parse("# Title");
        auto element = builder.build(ast);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(1));
        ftxui::Render(screen, element);
        auto output = screen.ToString();
        ASSERT_CONTAINS(output, "Title");

        // Check that the text is bold via pixel inspection
        auto& pixel = screen.PixelAt(0, 0);
        ASSERT_TRUE(pixel.bold);
    }

    // Test 6: H3 renders with bold + dim
    {
        auto ast = parser->parse("### Subtitle");
        auto element = builder.build(ast);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(1));
        ftxui::Render(screen, element);
        auto output = screen.ToString();
        ASSERT_CONTAINS(output, "Subtitle");

        auto& pixel = screen.PixelAt(0, 0);
        ASSERT_TRUE(pixel.bold);
        ASSERT_TRUE(pixel.dim);
    }

    // Test 7: H2 renders bold but not dim
    {
        auto ast = parser->parse("## Section");
        auto element = builder.build(ast);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(1));
        ftxui::Render(screen, element);

        auto& pixel = screen.PixelAt(0, 0);
        ASSERT_TRUE(pixel.bold);
        ASSERT_TRUE(!pixel.dim);
    }

    // Test 8: H4 renders with heading3 style (bold + dim)
    {
        auto ast = parser->parse("#### H4 title");
        ASSERT_EQ(ast.children[0].type, NodeType::Heading);
        ASSERT_EQ(ast.children[0].level, 4);

        auto element = builder.build(ast);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(1));
        ftxui::Render(screen, element);
        ASSERT_CONTAINS(screen.ToString(), "H4 title");
        ASSERT_TRUE(screen.PixelAt(0, 0).bold);
        ASSERT_TRUE(screen.PixelAt(0, 0).dim);
    }

    // Test 9: H5 and H6 also render with heading3 style
    {
        auto ast5 = parser->parse("##### H5");
        auto el5 = builder.build(ast5);
        auto scr5 = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                           ftxui::Dimension::Fixed(1));
        ftxui::Render(scr5, el5);
        ASSERT_TRUE(scr5.PixelAt(0, 0).bold);
        ASSERT_TRUE(scr5.PixelAt(0, 0).dim);

        auto ast6 = parser->parse("###### H6");
        auto el6 = builder.build(ast6);
        auto scr6 = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                           ftxui::Dimension::Fixed(1));
        ftxui::Render(scr6, el6);
        ASSERT_TRUE(scr6.PixelAt(0, 0).bold);
        ASSERT_TRUE(scr6.PixelAt(0, 0).dim);
    }

    return 0;
}
