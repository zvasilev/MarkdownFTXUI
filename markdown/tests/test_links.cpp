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

    // Test 6: Autolink parsed as regular link
    {
        auto ast = parser->parse("<https://auto.example.com>");
        auto& para = ast.children[0];
        ASSERT_EQ(para.children.size(), 1u);
        ASSERT_EQ(para.children[0].type, NodeType::Link);
        ASSERT_EQ(para.children[0].url, "https://auto.example.com");
        ASSERT_EQ(para.children[0].children.size(), 1u);
        ASSERT_EQ(para.children[0].children[0].type, NodeType::Text);
        ASSERT_EQ(para.children[0].children[0].text,
                  "https://auto.example.com");
    }

    // Test 7: Autolink renders underlined + blue
    {
        auto ast = parser->parse("<https://auto.example.com>");
        auto element = builder.build(ast);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(1));
        ftxui::Render(screen, element);
        auto& pixel = screen.PixelAt(0, 0);
        ASSERT_TRUE(pixel.underlined);
        ASSERT_TRUE(pixel.foreground_color == ftxui::Color::Blue);
    }

    // Test 8: Italic link — [*italic*](url)
    {
        auto ast = parser->parse("[*italic*](https://example.com)");
        auto& para = ast.children[0];
        ASSERT_EQ(para.children.size(), 1u);
        ASSERT_EQ(para.children[0].type, NodeType::Link);
        ASSERT_EQ(para.children[0].url, "https://example.com");
        ASSERT_EQ(para.children[0].children[0].type, NodeType::Emphasis);
        ASSERT_EQ(para.children[0].children[0].children[0].text, "italic");
    }

    // Test 9: Empty link text — [](url) parses without crash
    {
        auto ast = parser->parse("[](https://example.com)");
        auto element = builder.build(ast);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(1));
        ftxui::Render(screen, element);
        // Just verify no crash
    }

    // Test 10: Two links in one paragraph — both correct URLs
    {
        auto ast = parser->parse("[a](https://a.com) and [b](https://b.com)");
        auto& para = ast.children[0];
        int link_count = 0;
        for (auto const& child : para.children) {
            if (child.type == NodeType::Link) {
                ++link_count;
                if (link_count == 1) {
                    ASSERT_EQ(child.url, "https://a.com");
                } else {
                    ASSERT_EQ(child.url, "https://b.com");
                }
            }
        }
        ASSERT_EQ(link_count, 2);
    }

    // Test 11: Link with fragment — URL preserved
    {
        auto ast = parser->parse("[text](https://example.com/page#section)");
        auto& para = ast.children[0];
        ASSERT_EQ(para.children[0].type, NodeType::Link);
        ASSERT_EQ(para.children[0].url, "https://example.com/page#section");
    }

    return 0;
}
