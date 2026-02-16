#include "test_helper.hpp"
#include "markdown/dom_builder.hpp"
#include "markdown/parser.hpp"
#include "markdown/scroll_frame.hpp"

#include <ftxui/screen/screen.hpp>
#include <ftxui/dom/elements.hpp>
#include <iostream>

using namespace markdown;
using namespace ftxui;

// Reproduce the iOS Dev Weekly newsletter mega-paragraph:
// Multiple sections with links but NO blank lines between them,
// which cmark treats as ONE huge paragraph.
static const char* kMegaParagraph =
    "\xe2\x80\x93 Dave Verwer\n"
    "Sponsored Link\n"
    "[The complete guide to high-converting paywalls](https://example.com/paywalls)\n"
    "What actually makes a paywall convert? We analyzed real-world subscription "
    "data and design patterns to break down what works, what doesn't, and why. "
    "This study guide pulls together research, examples, and practical takeaways "
    "to help you design paywalls that turn more users into paying customers.\n"
    "[Read the paywalls study guide](https://example.com/paywalls) .\n"
    "News\n"
    "[The Swift Programming Language - PDF edition](https://example.com/swift-book)\n"
    "What a lovely thing [Peter Friese](https://example.com/peter) has done! "
    "He has automated the production of a beautiful PDF file from the DocC version of\n"
    "[The Swift Programming Language](https://example.com/tspl) . You can\n"
    "[download a pre-built release](https://example.com/release) or\n"
    "[build it yourself](https://example.com/build) if you're curious how he did it, too!\n"
    "Tools\n"
    "[Let Steve test your macOS app](https://example.com/steve)\n"
    "Here's yet another tool to help LLM coding agents work with your project!";

int main() {
    auto parser = make_cmark_parser();
    DomBuilder builder;

    // --- Test 1: Direct render (baseline) ---
    {
        constexpr int kWidth = 65;
        constexpr int kHeight = 80;
        auto ast = parser->parse(kMegaParagraph);
        auto element = builder.build(ast);
        auto screen = Screen::Create(Dimension::Fixed(kWidth),
                                     Dimension::Fixed(kHeight));
        Render(screen, element);
        auto output = screen.ToString();
        std::cout << "=== Test 1: Direct render (" << kWidth << " cols) ===\n";
        std::cout << output << "\n";
        ASSERT_CONTAINS(output, "Dave Verwer");
        ASSERT_CONTAINS(output, "Sponsored Link");
        ASSERT_CONTAINS(output, "What actually makes");
        ASSERT_CONTAINS(output, "News");
        ASSERT_CONTAINS(output, "Tools");
        std::cout << "PASS\n\n";
    }

    // --- Test 2: Wrapped in vbox | flex + hbox + vscroll_indicator + direct_scroll
    //     (mirrors conversation_view.cpp structure) ---
    {
        constexpr int kWidth = 65;
        constexpr int kHeight = 30;  // typical viewport
        auto ast = parser->parse(kMegaParagraph);
        auto body = builder.build(ast);

        // Mimic conversation_view: header rows + body in vbox
        Elements rows;
        rows.push_back(text("From: Test Sender"));
        rows.push_back(separator());
        rows.push_back(body);

        float scroll_ratio = 0.0f;
        auto inner = hbox({vbox(std::move(rows)) | flex, text(" ")})
                   | vscroll_indicator;
        auto content = direct_scroll(std::move(inner), scroll_ratio)
                     | flex;

        auto screen = Screen::Create(Dimension::Fixed(kWidth),
                                     Dimension::Fixed(kHeight));
        Render(screen, content);
        auto output = screen.ToString();
        std::cout << "=== Test 2: TUI layout (" << kWidth << "x" << kHeight << ") ===\n";
        std::cout << output << "\n";
        ASSERT_CONTAINS(output, "Dave Verwer");
        ASSERT_CONTAINS(output, "Sponsored Link");
        ASSERT_CONTAINS(output, "What actually makes");
        ASSERT_CONTAINS(output, "News");
        std::cout << "PASS\n\n";
    }

    // --- Test 3: Full email content - direct render (no scroll) ---
    const char* full_email =
        "\n"
        "[Read this issue on the web](https://iosdevweekly.com/issues/739)"
        " \xe2\x80\x93 [Browse the archives](https://iosdevweekly.com/issues)\n"
        "[iOS Dev Weekly logo](https://iosdevweekly.com/issues/739)\n"
        "# Issue 739\n\n"
        "16 th January 2026\n\n"
        "Written by Dave Verwer\n"
        "Comment\n\n"
        "If you\xe2\x80\x99" "ve spent any time at all reading about AI coding agents, "
        "you\xe2\x80\x99" "ll likely have heard that people get better results when "
        "working with languages _other_ than Swift.\n\n"
        "Is that because Swift isn\xe2\x80\x99" "t as AI-friendly as other languages? "
        "Absolutely not. In fact, I\xe2\x80\x99" "ve seen people say that type-safe "
        "languages produce _better_ results as the agent can rely on the "
        "compiler to automatically catch basic errors. No, I believe there "
        "are two main reasons you don\xe2\x80\x99" "t get quite as good results with "
        "Swift and SwiftUI:\n"
        "1. Swift has a much smaller footprint of publicly available "
        "open-source code compared to other languages like Python and "
        "JavaScript. Less training data means less language knowledge.\n"
        "2. Swift and SwiftUI have both seen sustained and significant "
        "changes since their releases.\n\n"
        "So I\xe2\x80\x99" "ve been interested to receive several recommendations of "
        "a tool that _really_ helps the agents get to grips with the "
        "current state of Swift.\n"
        "[Cupertino](https://github.com/mihaelamj/cupertino) is a "
        "[Model Context Protocol (MCP)](https://modelcontextprotocol.io/) "
        "server that crawls Apple\xe2\x80\x99" "s locally installed developer documentation.\n\n"
        "I\xe2\x80\x99" "ve not had a chance to try it in a real-world project yet, "
        "but I\xe2\x80\x99" "ve had enough people reach out and suggest I link to it "
        "that I thought I\xe2\x80\x99" "d make it the focus of this week\xe2\x80\x99" "s comment.\n\n"
        // THE MEGA-PARAGRAPH (no blank lines between sections):
        "\xe2\x80\x93 Dave Verwer\n"
        "Sponsored Link\n"
        "[The complete guide to high-converting paywalls](https://example.com/p)\n"
        "What actually makes a paywall convert? We analyzed real-world "
        "subscription data and design patterns to break down what works, "
        "what doesn\xe2\x80\x99" "t, and why.\n"
        "[Read the paywalls study guide](https://example.com/p) .\n"
        "News\n"
        "[The Swift Programming Language - PDF edition](https://example.com/s)\n"
        "What a lovely thing [Peter Friese](https://example.com/pf) has done!\n"
        "[The Swift Programming Language](https://example.com/tspl) . You can\n"
        "[download a pre-built release](https://example.com/r) or\n"
        "[build it yourself](https://example.com/b) if you\xe2\x80\x99" "re curious!\n"
        "Tools\n"
        "[Let Steve test your macOS app](https://example.com/steve)\n"
        "Here\xe2\x80\x99" "s yet another tool to help LLM coding agents!\n"
        "---\n"
        "[Make nice tools](https://example.com/tools)\n"
        "What a great article from [Paul Samuels](https://example.com/paul) on Docker.\n"
        "Code\n"
        "[Validator](https://example.com/validator)\n"
        "[Nikita Vasilev](https://example.com/nikita) \xe2\x80\x99" "s SwiftUI library.\n"
        "---\n"
        "[Universal Links At Scale](https://example.com/links)\n"
        "Every time I hear people talk about Universal Links, I hear stories.\n\n"
        "Follow Dave on [Bluesky](https://example.com/bs) or [Mastodon](https://example.com/m).\n";

    {
        constexpr int kWidth = 65;
        constexpr int kHeight = 120;
        auto ast = parser->parse(full_email);
        auto body = builder.build(ast);

        auto screen = Screen::Create(Dimension::Fixed(kWidth),
                                     Dimension::Fixed(kHeight));
        Render(screen, body);
        auto output = screen.ToString();
        std::cout << "=== Test 3: Full email direct render (" << kWidth
                  << "x" << kHeight << ") ===\n";
        std::cout << output << "\n";
        ASSERT_CONTAINS(output, "Sponsored Link");
        ASSERT_CONTAINS(output, "Tools");
        std::cout << "PASS\n\n";
    }

    // --- Test 4: Same email but with blank lines before --- (fix) ---
    {
        // Replace "!\n---\n" with "!\n\n---\n" to prevent setext headings
        std::string fixed_email = full_email;
        // Add blank line before each "---" that follows text
        std::string bad = "!\n---\n";
        std::string good = "!\n\n---\n\n";
        size_t pos = 0;
        while ((pos = fixed_email.find(bad, pos)) != std::string::npos) {
            fixed_email.replace(pos, bad.size(), good);
            pos += good.size();
        }

        constexpr int kWidth = 65;
        constexpr int kHeight = 120;
        auto ast = parser->parse(fixed_email);
        auto body = builder.build(ast);

        auto screen = Screen::Create(Dimension::Fixed(kWidth),
                                     Dimension::Fixed(kHeight));
        Render(screen, body);
        auto output = screen.ToString();
        std::cout << "=== Test 4: Fixed email (blank before ---) ===\n";
        std::cout << output << "\n";
        ASSERT_CONTAINS(output, "Sponsored Link");
        ASSERT_CONTAINS(output, "Tools");
        std::cout << "PASS\n\n";
    }

    std::cout << "test passed successfully.\n";
    return 0;
}
