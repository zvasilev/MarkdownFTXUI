#include "test_helper.hpp"
#include "markdown/text_utils.hpp"
#include "markdown/parser.hpp"
#include "markdown/dom_builder.hpp"

#include <chrono>
#include <iostream>
#include <string>

#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>

using namespace markdown;

int main() {
    // Test 1: CJK display width correctness
    {
        // U+4E16 (世) U+754C (界) — each should be width 2
        std::string cjk = "\xE4\xB8\x96\xE7\x95\x8C"; // 世界
        ASSERT_EQ(utf8_display_width(cjk), 4);
    }

    // Test 2: Mixed ASCII + CJK width
    {
        std::string mixed = "Hi\xE4\xB8\x96\xE7\x95\x8Cok"; // Hi世界ok
        ASSERT_EQ(utf8_display_width(mixed), 8); // H(1)+i(1)+世(2)+界(2)+o(1)+k(1)
    }

    // Test 3: visual_col_to_byte with CJK
    {
        std::string text = "A\xE4\xB8\x96" "B"; // A世B
        ASSERT_EQ(visual_col_to_byte(text, 0), 0u);  // A
        ASSERT_EQ(visual_col_to_byte(text, 1), 1u);  // start of 世
        ASSERT_EQ(visual_col_to_byte(text, 3), 4u);  // B (世 occupies cols 1-2)
    }

    // Test 4: Performance — large CJK document through full pipeline
    {
        std::string doc;
        for (int i = 0; i < 500; ++i) {
            // 标题 = heading in Chinese
            doc += "# \xE6\xA0\x87\xE9\xA2\x98 " + std::to_string(i) + "\n\n";
            // 这是一个测试段落。= This is a test paragraph.
            doc += "\xE8\xBF\x99\xE6\x98\xAF\xE4\xB8\x80\xE4\xB8\xAA"
                   "\xE6\xB5\x8B\xE8\xAF\x95\xE6\xAE\xB5\xE8\x90\xBD"
                   "\xE3\x80\x82\n\n";
        }

        auto parser = make_cmark_parser();
        DomBuilder builder;

        auto t_start = std::chrono::high_resolution_clock::now();
        auto ast = parser->parse(doc);
        auto el = builder.build(ast);
        auto screen = ftxui::Screen::Create(
            ftxui::Dimension::Fixed(80), ftxui::Dimension::Fixed(40));
        ftxui::Render(screen, el);
        auto t_end = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(
            t_end - t_start).count();

        std::cout << "CJK document (500 headings + paragraphs): " << ms << " ms\n";
        ASSERT_TRUE(ms < 5000.0);
    }

    // Test 5: Performance — utf8_display_width on 10K-char CJK string
    {
        std::string long_cjk;
        for (int i = 0; i < 10000; ++i) {
            long_cjk += "\xE4\xB8\x96"; // 世 repeated 10K times
        }

        auto t_start = std::chrono::high_resolution_clock::now();
        int width = 0;
        for (int i = 0; i < 100; ++i) {
            width = utf8_display_width(long_cjk);
        }
        auto t_end = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(
            t_end - t_start).count();

        ASSERT_EQ(width, 20000); // 10K chars * 2 width each
        std::cout << "utf8_display_width (10K CJK, 100 calls): " << ms << " ms\n";
        ASSERT_TRUE(ms < 1000.0);
    }

    // Test 6: Performance — visual_col_to_byte near end of 10K-char string
    {
        std::string long_cjk;
        for (int i = 0; i < 10000; ++i) {
            long_cjk += "\xE4\xB8\x96";
        }

        auto t_start = std::chrono::high_resolution_clock::now();
        size_t result = 0;
        for (int i = 0; i < 100; ++i) {
            result = visual_col_to_byte(long_cjk, 19998);
        }
        auto t_end = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(
            t_end - t_start).count();

        ASSERT_EQ(result, 29997u); // 9999th char * 3 bytes
        std::cout << "visual_col_to_byte (col 19998, 100 calls): " << ms << " ms\n";
        ASSERT_TRUE(ms < 1000.0);
    }

    return 0;
}
