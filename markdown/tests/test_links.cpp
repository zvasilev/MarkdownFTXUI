#include "test_helper.hpp"
#include "markdown/parser.hpp"
#include "markdown/dom_builder.hpp"

#include <ftxui/screen/screen.hpp>
#include <ftxui/dom/elements.hpp>

using namespace markdown;

int main() {
    auto parser = make_cmark_parser();
    DomBuilder builder;

    // Test 1: Simple link parsed correctly
    {
        auto ast = parser->parse("[Docs](https://example.com)");
        auto& para = ast.children[0];
        ASSERT_EQ(para.children.size(), 1u);
        ASSERT_EQ(para.children[0].type, NodeType::Link);
        ASSERT_EQ(para.children[0].url, "https://example.com");
        ASSERT_EQ(para.children[0].children.size(), 1u);
        ASSERT_EQ(para.children[0].children[0].type, NodeType::Text);
        ASSERT_EQ(para.children[0].children[0].text, "Docs");
    }

    // Test 2: Link renders underlined, shows text not URL
    {
        auto ast = parser->parse("[Docs](https://example.com)");
        auto element = builder.build(ast);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(1));
        ftxui::Render(screen, element);
        auto output = screen.ToString();
        ASSERT_CONTAINS(output, "Docs");

        auto& pixel = screen.PixelAt(0, 0);
        ASSERT_TRUE(pixel.underlined);
    }

    // Test 3: Bold link — [**bold link**](url)
    {
        auto ast = parser->parse("See [**bold link**](https://example.com)");
        auto& para = ast.children[0];
        ASSERT_EQ(para.children.size(), 2u);
        ASSERT_EQ(para.children[0].type, NodeType::Text);
        ASSERT_EQ(para.children[0].text, "See ");
        ASSERT_EQ(para.children[1].type, NodeType::Link);
        ASSERT_EQ(para.children[1].children[0].type, NodeType::Strong);
        ASSERT_EQ(para.children[1].children[0].children[0].text, "bold link");
    }

    // Test 4: Bold link renders underlined + bold
    {
        auto ast = parser->parse("[**bold**](url)");
        auto element = builder.build(ast);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(1));
        ftxui::Render(screen, element);

        auto& pixel = screen.PixelAt(0, 0);
        ASSERT_TRUE(pixel.underlined);
        ASSERT_TRUE(pixel.bold);
    }

    // Test 5: Link mixed with plain text
    {
        auto ast = parser->parse("Visit [here](url) for info");
        auto& para = ast.children[0];
        ASSERT_EQ(para.children.size(), 3u);
        ASSERT_EQ(para.children[0].type, NodeType::Text);
        ASSERT_EQ(para.children[0].text, "Visit ");
        ASSERT_EQ(para.children[1].type, NodeType::Link);
        ASSERT_EQ(para.children[2].type, NodeType::Text);
        ASSERT_EQ(para.children[2].text, " for info");
    }

    return 0;
}
