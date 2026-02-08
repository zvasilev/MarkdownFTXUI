#include "test_helper.hpp"
#include "markdown/parser.hpp"
#include "markdown/dom_builder.hpp"

#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>

using namespace markdown;

// Helper: parse + build + render to a screen.  Returns 0 on success.
// The sole assertion is "didn't crash / didn't hang".
static int render_ok(MarkdownParser& parser, DomBuilder& builder,
                     std::string const& input, int width = 80,
                     int height = 40) {
    auto ast = parser.parse(input);
    auto element = builder.build(ast);
    auto screen =
        ftxui::Screen::Create(ftxui::Dimension::Fixed(width),
                              ftxui::Dimension::Fixed(height));
    ftxui::Render(screen, element);
    (void)screen.ToString();  // force serialization
    return 0;
}

// ── Generators ──────────────────────────────────────────────────────────

// Deeply nested blockquotes:  > > > > > ...text
static std::string deep_blockquotes(int depth) {
    std::string prefix;
    for (int i = 0; i < depth; ++i) prefix += "> ";
    return prefix + "hello from the abyss\n";
}

// Deeply nested bullet lists (each level indented 2 more spaces)
static std::string deep_bullet_list(int depth) {
    std::string out;
    for (int i = 0; i < depth; ++i) {
        out += std::string(i * 2, ' ') + "- level " + std::to_string(i) + "\n";
    }
    return out;
}

// Deeply nested ordered lists
static std::string deep_ordered_list(int depth) {
    std::string out;
    for (int i = 0; i < depth; ++i) {
        out += std::string(i * 3, ' ') + "1. level " + std::to_string(i) + "\n";
    }
    return out;
}

// Blockquote containing a list containing a blockquote ...
static std::string alternating_block_list(int depth) {
    std::string out;
    std::string prefix;
    for (int i = 0; i < depth; ++i) {
        if (i % 2 == 0) {
            prefix += "> ";
        } else {
            prefix += "- ";
        }
    }
    return prefix + "deep\n";
}

// Lots of **bold** wrapping:   ****...**text**..****
// cmark-gfm collapses some of these, but this is a real-world copy-paste mess
static std::string nested_bold_italic(int depth) {
    std::string open, close;
    for (int i = 0; i < depth; ++i) {
        if (i % 3 == 0) {
            open += "**";
            close = "**" + close;
        } else if (i % 3 == 1) {
            open += "*";
            close = "*" + close;
        } else {
            open += "***";
            close = "***" + close;
        }
    }
    return open + "text" + close + "\n";
}

// Hundreds of links in one paragraph (email footers with many tracked URLs)
static std::string many_links(int count) {
    std::string out;
    for (int i = 0; i < count; ++i) {
        out += "[link" + std::to_string(i) + "](https://example.com/" +
               std::to_string(i) + ") ";
    }
    return out + "\n";
}

// Every block type interleaved rapidly
static std::string rapid_type_changes(int count) {
    std::string out;
    for (int i = 0; i < count; ++i) {
        switch (i % 7) {
        case 0: out += "# Heading " + std::to_string(i) + "\n\n"; break;
        case 1: out += "Paragraph " + std::to_string(i) + "\n\n"; break;
        case 2: out += "> Quote " + std::to_string(i) + "\n\n"; break;
        case 3: out += "- bullet " + std::to_string(i) + "\n\n"; break;
        case 4: out += "1. ordered " + std::to_string(i) + "\n\n"; break;
        case 5: out += "```\ncode " + std::to_string(i) + "\n```\n\n"; break;
        case 6: out += "---\n\n"; break;
        }
    }
    return out;
}

int main() {
    auto parser = make_cmark_parser();
    DomBuilder builder;

    // ── 1. Deep blockquotes (20 levels) ─────────────────────────────────
    {
        auto r = render_ok(*parser, builder, deep_blockquotes(20));
        ASSERT_EQ(r, 0);
    }

    // ── 2. Very deep blockquotes (50 levels) ────────────────────────────
    {
        auto r = render_ok(*parser, builder, deep_blockquotes(50));
        ASSERT_EQ(r, 0);
    }

    // ── 3. Extreme blockquotes (100 levels) ─────────────────────────────
    {
        auto r = render_ok(*parser, builder, deep_blockquotes(100));
        ASSERT_EQ(r, 0);
    }

    // ── 4. Deep bullet lists (30 levels) ────────────────────────────────
    {
        auto r = render_ok(*parser, builder, deep_bullet_list(30));
        ASSERT_EQ(r, 0);
    }

    // ── 5. Deep bullet lists (80 levels) ────────────────────────────────
    {
        auto r = render_ok(*parser, builder, deep_bullet_list(80));
        ASSERT_EQ(r, 0);
    }

    // ── 6. Deep ordered lists (50 levels) ───────────────────────────────
    {
        auto r = render_ok(*parser, builder, deep_ordered_list(50));
        ASSERT_EQ(r, 0);
    }

    // ── 7. Alternating blockquote/list nesting (40 levels) ──────────────
    {
        auto r = render_ok(*parser, builder, alternating_block_list(40));
        ASSERT_EQ(r, 0);
    }

    // ── 8. Deeply nested bold/italic (20 layers) ────────────────────────
    {
        auto r = render_ok(*parser, builder, nested_bold_italic(20));
        ASSERT_EQ(r, 0);
    }

    // ── 9. Deeply nested bold/italic (50 layers) ────────────────────────
    {
        auto r = render_ok(*parser, builder, nested_bold_italic(50));
        ASSERT_EQ(r, 0);
    }

    // ── 10. Hundreds of links in one paragraph ──────────────────────────
    {
        auto r = render_ok(*parser, builder, many_links(200));
        ASSERT_EQ(r, 0);
    }

    // ── 11. Thousands of links ──────────────────────────────────────────
    {
        auto r = render_ok(*parser, builder, many_links(1000), 120, 200);
        ASSERT_EQ(r, 0);
    }

    // ── 12. Very long single line (50KB of text) ────────────────────────
    {
        std::string line(50000, 'A');
        auto r = render_ok(*parser, builder, line);
        ASSERT_EQ(r, 0);
    }

    // ── 13. Very long line with inline formatting ───────────────────────
    {
        std::string line;
        for (int i = 0; i < 5000; ++i) {
            line += "**bold** normal *italic* ";
        }
        auto r = render_ok(*parser, builder, line, 80, 2000);
        ASSERT_EQ(r, 0);
    }

    // ── 14. Huge document — 2000 paragraphs ─────────────────────────────
    {
        std::string doc;
        for (int i = 0; i < 2000; ++i) {
            doc += "Paragraph number " + std::to_string(i) +
                   " with some words.\n\n";
        }
        auto r = render_ok(*parser, builder, doc, 80, 4000);
        ASSERT_EQ(r, 0);
    }

    // ── 15. Code block with 10K lines ───────────────────────────────────
    {
        std::string code = "```\n";
        for (int i = 0; i < 10000; ++i) {
            code += "line " + std::to_string(i) + "\n";
        }
        code += "```\n";
        auto r = render_ok(*parser, builder, code, 80, 100);
        ASSERT_EQ(r, 0);
    }

    // ── 16. Rapid type changes (200 blocks) ─────────────────────────────
    {
        auto r = render_ok(*parser, builder, rapid_type_changes(200), 80, 600);
        ASSERT_EQ(r, 0);
    }

    // ── 17. Unclosed formatting delimiters repeated ─────────────────────
    //    Real emails have broken markdown from copy-paste
    {
        std::string mess;
        for (int i = 0; i < 500; ++i) mess += "** ";
        mess += "oops";
        auto r = render_ok(*parser, builder, mess);
        ASSERT_EQ(r, 0);
    }

    // ── 18. Unclosed code fences ────────────────────────────────────────
    {
        std::string doc = "before\n\n```\nthis code block never closes\n"
                          "and keeps going\nfor many lines\n";
        for (int i = 0; i < 200; ++i) doc += "more code\n";
        auto r = render_ok(*parser, builder, doc, 80, 250);
        ASSERT_EQ(r, 0);
    }

    // ── 19. Extremely long URL in a link ────────────────────────────────
    {
        std::string url(8000, 'x');
        std::string input = "[click](https://example.com/" + url + ")\n";
        auto r = render_ok(*parser, builder, input);
        ASSERT_EQ(r, 0);
    }

    // ── 20. Image with very long alt text ───────────────────────────────
    {
        std::string alt(5000, 'a');
        std::string input = "![" + alt + "](https://example.com/img.png)\n";
        auto r = render_ok(*parser, builder, input);
        ASSERT_EQ(r, 0);
    }

    // ── 21. Only `>` markers with nothing else (empty blockquotes) ──────
    {
        std::string doc;
        for (int i = 0; i < 100; ++i) doc += ">\n>\n";
        auto r = render_ok(*parser, builder, doc);
        ASSERT_EQ(r, 0);
    }

    // ── 22. Only list markers, no content ───────────────────────────────
    {
        std::string doc;
        for (int i = 0; i < 200; ++i) doc += "- \n";
        auto r = render_ok(*parser, builder, doc, 80, 200);
        ASSERT_EQ(r, 0);
    }

    // ── 23. Heading levels 1-6 then invalid (level 7+) ─────────────────
    {
        std::string doc;
        for (int i = 1; i <= 10; ++i) {
            doc += std::string(i, '#') + " heading " + std::to_string(i) + "\n\n";
        }
        auto r = render_ok(*parser, builder, doc);
        ASSERT_EQ(r, 0);
    }

    // ── 24. Null bytes embedded in input ────────────────────────────────
    //    Emails from broken encoders sometimes contain \0
    {
        std::string input = "hello";
        input += '\0';
        input += "world";
        // cmark takes data+size, so null doesn't truncate
        auto ast = parser->parse(input);
        auto element = builder.build(ast);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                            ftxui::Dimension::Fixed(1));
        ftxui::Render(screen, element);
        // Just survive
    }

    // ── 25. Only newlines (10K) ─────────────────────────────────────────
    {
        std::string doc(10000, '\n');
        auto r = render_ok(*parser, builder, doc);
        ASSERT_EQ(r, 0);
    }

    // ── 26. Tab characters everywhere ───────────────────────────────────
    {
        std::string doc;
        for (int i = 0; i < 100; ++i) {
            doc += "\t\t\tindented\twith\ttabs\n";
        }
        auto r = render_ok(*parser, builder, doc);
        ASSERT_EQ(r, 0);
    }

    // ── 27. Unicode stress: emoji, combining chars, RTL, ZWJ ────────────
    {
        std::string doc =
            "# \xF0\x9F\x94\xA5 Fire heading\n\n"                    // 🔥
            "Emoji paragraph: \xF0\x9F\x91\xA8\xE2\x80\x8D"         // 👨‍
            "\xF0\x9F\x91\xA9\xE2\x80\x8D"                           // 👩‍
            "\xF0\x9F\x91\xA7\xE2\x80\x8D"                           // 👧‍
            "\xF0\x9F\x91\xA6\n\n"                                    // 👦
            "> \xD7\xA9\xD7\x9C\xD7\x95\xD7\x9D "                   // שלום (Hebrew)
            "\xD8\xA7\xD9\x84\xD8\xB3\xD9\x84\xD8\xA7\xD9\x85\n\n" // السلام (Arabic)
            "Combining: e\xCC\x81 a\xCC\x80 o\xCC\x88\n\n"           // é à ö
            "Zero-width: a\xE2\x80\x8B" "b\xE2\x80\x8C" "c\xE2\x80\x8D" "d\n"; // ZWSP, ZWNJ, ZWJ
        auto r = render_ok(*parser, builder, doc);
        ASSERT_EQ(r, 0);
    }

    // ── 28. Link inside blockquote inside list (real email pattern) ─────
    {
        std::string doc;
        for (int i = 0; i < 50; ++i) {
            doc += "> - [link " + std::to_string(i) +
                   "](https://example.com/" + std::to_string(i) + ")\n";
        }
        auto r = render_ok(*parser, builder, doc, 80, 60);
        ASSERT_EQ(r, 0);
    }

    // ── 29. Pathological backtracking bait: `[` repeated ────────────────
    //    Some markdown parsers choke on repeated open brackets
    {
        std::string doc;
        for (int i = 0; i < 1000; ++i) doc += "[";
        doc += "text";
        for (int i = 0; i < 1000; ++i) doc += "]";
        auto r = render_ok(*parser, builder, doc);
        ASSERT_EQ(r, 0);
    }

    // ── 30. Inline code spanning entire pathological string ─────────────
    {
        std::string doc = "`";
        doc += std::string(10000, 'x');
        doc += "`";
        auto r = render_ok(*parser, builder, doc);
        ASSERT_EQ(r, 0);
    }

    // ── 31. Mixed inline: bold link with code inside blockquote ─────────
    {
        std::string doc;
        for (int i = 0; i < 30; ++i) {
            doc += "> **[`code link " + std::to_string(i) +
                   "`](https://x.com)**\n>\n";
        }
        auto r = render_ok(*parser, builder, doc);
        ASSERT_EQ(r, 0);
    }

    // ── 32. Many headings (email thread with many forwarded subjects) ───
    {
        std::string doc;
        for (int i = 0; i < 500; ++i) {
            doc += "## Re: Re: Re: Subject " + std::to_string(i) + "\n\n"
                   "body\n\n";
        }
        auto r = render_ok(*parser, builder, doc, 80, 2000);
        ASSERT_EQ(r, 0);
    }

    // ── 33. Single character documents ──────────────────────────────────
    {
        for (char c : {'#', '>', '-', '*', '`', '[', ']', '(', ')', '!',
                       '\\', '|', '\n', '\r', '\t', ' '}) {
            std::string input(1, c);
            auto r = render_ok(*parser, builder, input);
            ASSERT_EQ(r, 0);
        }
    }

    // ── 34. CRLF line endings (Windows emails) ─────────────────────────
    {
        std::string doc = "# Title\r\n\r\n> quote\r\n\r\n- item\r\n";
        auto r = render_ok(*parser, builder, doc);
        ASSERT_EQ(r, 0);
    }

    // ── 35. Mixed line endings in same document ─────────────────────────
    {
        std::string doc = "para1\n\npara2\r\n\r\npara3\r\rpara4\n\r";
        auto r = render_ok(*parser, builder, doc);
        ASSERT_EQ(r, 0);
    }

    // ── 36. Blockquote with every inline type ───────────────────────────
    {
        std::string doc =
            "> **bold** *italic* ***both*** `code` "
            "[link](https://x.com) ![img](https://x.com/i.png)\n"
            "> \n"
            "> More text with **nested *emphasis* inside** bold\n";
        auto r = render_ok(*parser, builder, doc);
        ASSERT_EQ(r, 0);
    }

    // ── 37. Ordered list starting at huge number ────────────────────────
    {
        std::string doc = "999999999. big start\n1000000000. next\n";
        auto r = render_ok(*parser, builder, doc);
        ASSERT_EQ(r, 0);
    }

    // ── 38. Emphasis delimiter soup (real copy-paste mess) ──────────────
    {
        std::string doc;
        for (int i = 0; i < 200; ++i) {
            doc += (i % 2 == 0) ? "*_" : "_*";
        }
        doc += " oops ";
        for (int i = 0; i < 200; ++i) {
            doc += (i % 2 == 0) ? "_*" : "*_";
        }
        doc += "\n";
        auto r = render_ok(*parser, builder, doc);
        ASSERT_EQ(r, 0);
    }

    // ── 39. Render to zero-size screen ──────────────────────────────────
    {
        auto ast = parser->parse("# Hello\n\nworld");
        auto element = builder.build(ast);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(0),
                                            ftxui::Dimension::Fixed(0));
        ftxui::Render(screen, element);
        // Don't crash on degenerate screen
    }

    // ── 40. Render to 1x1 screen ───────────────────────────────────────
    {
        auto ast = parser->parse(many_links(50));
        auto element = builder.build(ast);
        auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(1),
                                            ftxui::Dimension::Fixed(1));
        ftxui::Render(screen, element);
    }

    // ── 41. Deeply nested blockquotes with inline formatting ────────────
    //    This is the real killer: recursion in build_blockquote AND
    //    collect_inline_words simultaneously.
    {
        std::string prefix;
        for (int i = 0; i < 30; ++i) prefix += "> ";
        std::string doc = prefix + "**bold *and italic* together** with "
                          "[a link](https://example.com)\n";
        auto r = render_ok(*parser, builder, doc);
        ASSERT_EQ(r, 0);
    }

    // ── 42. Email signature with many horizontal rules ──────────────────
    {
        std::string doc;
        for (int i = 0; i < 100; ++i) {
            doc += "---\n\n";
        }
        auto r = render_ok(*parser, builder, doc, 80, 200);
        ASSERT_EQ(r, 0);
    }

    // ── 43. Link whose text is also a link (nested link attempt) ────────
    {
        std::string doc = "[[nested]](https://a.com) "
                          "[text [with] brackets](https://b.com)\n";
        auto r = render_ok(*parser, builder, doc);
        ASSERT_EQ(r, 0);
    }

    // ── 44. Paragraph that is entirely images ───────────────────────────
    {
        std::string doc;
        for (int i = 0; i < 100; ++i) {
            doc += "![img" + std::to_string(i) + "](https://x.com/" +
                   std::to_string(i) + ".png) ";
        }
        doc += "\n";
        auto r = render_ok(*parser, builder, doc, 80, 100);
        ASSERT_EQ(r, 0);
    }

    // ── 45. Realistic forwarded email: nested quotes with attribution ───
    {
        std::string doc;
        for (int i = 5; i >= 1; --i) {
            std::string prefix(i, '>');
            prefix += ' ';
            doc += prefix + "On Jan " + std::to_string(i) +
                   ", person" + std::to_string(i) + "@email.com wrote:\n";
            doc += prefix + "\n";
            doc += prefix + "Thanks for the **update**. See "
                   "[details](https://jira.example.com/PROJ-" +
                   std::to_string(i * 100) + ").\n";
            doc += prefix + "\n";
        }
        doc += "Top-level reply with `inline code` here.\n";
        auto r = render_ok(*parser, builder, doc);
        ASSERT_EQ(r, 0);
    }

    // ── 46. HTML tags mixed in (email clients produce raw HTML) ─────────
    {
        std::string doc =
            "<div>some html</div>\n\n"
            "<table><tr><td>cell</td></tr></table>\n\n"
            "<script>alert('xss')</script>\n\n"
            "Normal **markdown** after HTML\n";
        auto r = render_ok(*parser, builder, doc);
        ASSERT_EQ(r, 0);
    }

    // ── 47. Backslash escapes everywhere ────────────────────────────────
    {
        std::string doc;
        for (int i = 0; i < 200; ++i) {
            doc += "\\* \\# \\> \\- \\` \\[ \\] \\( \\) ";
        }
        doc += "\n";
        auto r = render_ok(*parser, builder, doc, 80, 100);
        ASSERT_EQ(r, 0);
    }

    // ── 48. Huge list with links in every item ──────────────────────────
    //    Tests LinkTarget vector growth under many build_list_item calls
    {
        std::string doc;
        for (int i = 0; i < 500; ++i) {
            doc += "- item with [link" + std::to_string(i) +
                   "](https://example.com/" + std::to_string(i) + ")\n";
        }
        auto r = render_ok(*parser, builder, doc, 80, 600);
        ASSERT_EQ(r, 0);
    }

    // ── 49. Build same AST twice (reuse DomBuilder) ─────────────────────
    //    Ensures _link_targets.clear() properly resets state
    {
        auto ast = parser->parse(many_links(100));
        builder.build(ast, 0);
        builder.build(ast, 5);   // different focused link
        builder.build(ast, 99);  // last link
        builder.build(ast, -1);  // no focus
        // Just survive repeated builds
    }

    // ── 50. Everything at once: the kitchen sink ────────────────────────
    {
        std::string doc;
        doc += "# Newsletter\n\n";
        doc += "---\n\n";
        doc += "> Forward from **[sender](mailto:a@b.com)**:\n";
        doc += "> > Original:\n";
        doc += "> > > *Very* important `code` snippet:\n";
        doc += "> > > ```\n> > > x = 1\n> > > ```\n";
        doc += "\n";
        for (int i = 0; i < 50; ++i) {
            doc += "- [Item " + std::to_string(i) +
                   "](https://example.com/" + std::to_string(i) + ") — ";
            doc += "**bold** and *italic* and `code`\n";
        }
        doc += "\n---\n\n";
        doc += "![banner](https://example.com/banner.png)\n\n";
        doc += std::string(2000, 'x') + "\n\n";  // long paragraph
        for (int i = 0; i < 20; ++i) {
            doc += std::string(i + 1, '#') + " h" + std::to_string(i + 1) +
                   "\n\n";
        }
        auto r = render_ok(*parser, builder, doc, 120, 500);
        ASSERT_EQ(r, 0);
    }

    return 0;
}
