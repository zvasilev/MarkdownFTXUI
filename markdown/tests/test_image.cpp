#include "test_helper.hpp"
#include "markdown/parser.hpp"
#include "markdown/dom_builder.hpp"

#include <ftxui/screen/screen.hpp>
#include <ftxui/dom/elements.hpp>

using namespace markdown;

int main() {
    auto parser = make_cmark_parser();
    DomBuilder builder;

    // Test 1: Image parsed correctly — has url and alt text child
    {
        auto ast = parser->parse("![alt text](https://img.url/photo.png)");
        // Document > Paragraph > Image
        ASSERT_EQ(ast.children.size(), 1u);
        auto& para = ast.children[0];
        ASSERT_EQ(para.type, NodeType::Paragraph);
        ASSERT_EQ(para.children.size(), 1u);
        auto& img = para.children[0];
        ASSERT_EQ(img.type, NodeType::Image);
        ASSERT_EQ(img.url, "https://img.url/photo.png");
        ASSERT_EQ(img.children.size(), 1u);
        ASSERT_EQ(img.children[0].type, NodeType::Text);
        ASSERT_EQ(img.children[0].text, "alt text");
    }

    // Test 2: Image renders as [IMG: alt text]
    {
        auto ast = parser->parse("![my photo](https://example.com/img.jpg)");
        auto element = builder.build(ast);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(1));
        ftxui::Render(screen, element);
        auto output = screen.ToString();
        ASSERT_CONTAINS(output, "[IMG:");
        ASSERT_CONTAINS(output, "my photo");
    }

    // Test 3: Image with empty alt text — no crash
    {
        auto ast = parser->parse("![](https://example.com/img.jpg)");
        auto element = builder.build(ast);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(1));
        ftxui::Render(screen, element);
        auto output = screen.ToString();
        ASSERT_CONTAINS(output, "[IMG:");
    }

    // Test 4: Image inside paragraph with text
    {
        auto ast = parser->parse("Check out ![logo](https://x.com/logo.png) here");
        auto element = builder.build(ast);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(1));
        ftxui::Render(screen, element);
        auto output = screen.ToString();
        ASSERT_CONTAINS(output, "Check out");
        ASSERT_CONTAINS(output, "logo");
        ASSERT_CONTAINS(output, "here");
    }

    return 0;
}
