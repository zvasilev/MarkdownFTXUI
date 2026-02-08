#include "test_helper.hpp"
#include "markdown/parser.hpp"
#include "markdown/dom_builder.hpp"

#include <chrono>
#include <iostream>
#include <string>

#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>

using namespace markdown;

int main() {
    // Large document: 200 mixed blocks
    std::string doc;
    for (int i = 0; i < 200; ++i) {
        doc += "## Section " + std::to_string(i) + "\n\n";
        doc += "Paragraph with **bold**, *italic*, and [link"
               + std::to_string(i) + "](https://example.com/"
               + std::to_string(i) + ").\n\n";
        doc += "> Quote " + std::to_string(i) + "\n\n";
        doc += "- item " + std::to_string(i) + "\n\n";
    }

    auto parser = make_cmark_parser();
    DomBuilder builder;

    // Warm up
    {
        auto ast = parser->parse(doc);
        auto el = builder.build(ast);
        auto screen = ftxui::Screen::Create(
            ftxui::Dimension::Fixed(80), ftxui::Dimension::Fixed(40));
        ftxui::Render(screen, el);
    }

    // Time first 10 iterations
    auto t1_start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 10; ++i) {
        auto ast = parser->parse(doc);
        auto el = builder.build(ast);
        auto screen = ftxui::Screen::Create(
            ftxui::Dimension::Fixed(80), ftxui::Dimension::Fixed(40));
        ftxui::Render(screen, el);
    }
    auto t1_end = std::chrono::high_resolution_clock::now();
    double first10 = std::chrono::duration<double, std::milli>(
        t1_end - t1_start).count();

    // Run 80 more iterations
    for (int i = 0; i < 80; ++i) {
        auto ast = parser->parse(doc);
        auto el = builder.build(ast);
        (void)el;
    }

    // Time last 10 iterations
    auto t2_start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 10; ++i) {
        auto ast = parser->parse(doc);
        auto el = builder.build(ast);
        auto screen = ftxui::Screen::Create(
            ftxui::Dimension::Fixed(80), ftxui::Dimension::Fixed(40));
        ftxui::Render(screen, el);
    }
    auto t2_end = std::chrono::high_resolution_clock::now();
    double last10 = std::chrono::duration<double, std::milli>(
        t2_end - t2_start).count();

    std::cout << "First 10 iterations: " << first10 << " ms\n";
    std::cout << "Last 10 iterations:  " << last10 << " ms\n";
    double ratio = last10 / first10;
    std::cout << "Ratio (last/first):  " << ratio << "\n";

    // No significant degradation
    ASSERT_TRUE(ratio < 2.0);
    // Absolute budget per cycle
    ASSERT_TRUE(first10 / 10.0 < 500.0);
    ASSERT_TRUE(last10 / 10.0 < 500.0);

    return 0;
}
