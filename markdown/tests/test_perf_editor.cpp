#include "test_helper.hpp"
#include "markdown/highlight.hpp"

#include <chrono>
#include <iostream>
#include <string>

using namespace markdown;

int main() {
    // Build a 500-line mixed markdown document
    std::string doc;
    for (int i = 0; i < 500; ++i) {
        switch (i % 5) {
        case 0: doc += "# Heading " + std::to_string(i) + "\n"; break;
        case 1: doc += "Normal paragraph with **bold** and *italic* text\n"; break;
        case 2: doc += "> Blockquote line " + std::to_string(i) + "\n"; break;
        case 3: doc += "- List item with `code` here\n"; break;
        case 4: doc += "[link](https://example.com/" + std::to_string(i) + ")\n"; break;
        }
    }

    int cursor_pos = static_cast<int>(doc.size() / 2);

    // Warm up
    auto warm = highlight_markdown_with_cursor(doc, cursor_pos, true, false, true);
    (void)warm;

    // Time 100 iterations
    auto start = std::chrono::high_resolution_clock::now();
    int iterations = 100;
    for (int i = 0; i < iterations; ++i) {
        auto el = highlight_markdown_with_cursor(
            doc, cursor_pos + (i % 10), true, false, true);
        (void)el;
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto total_ms = std::chrono::duration<double, std::milli>(end - start).count();
    double per_call_ms = total_ms / iterations;

    std::cout << "highlight_markdown_with_cursor: " << per_call_ms
              << " ms/call (" << iterations << " iterations, "
              << doc.size() << " bytes, 500 lines)\n";

    // Budget: 50ms per call (generous for CI)
    ASSERT_TRUE(per_call_ms < 50.0);

    return 0;
}
