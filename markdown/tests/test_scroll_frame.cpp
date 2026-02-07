#include "test_helper.hpp"
#include "markdown/scroll_frame.hpp"

#include <ftxui/dom/elements.hpp>
#include <ftxui/screen/screen.hpp>

using namespace markdown;

// Helper: render an element into a screen and return the output string.
std::string render(ftxui::Element el, int width, int height) {
    auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(width),
                                        ftxui::Dimension::Fixed(height));
    ftxui::Render(screen, el);
    return screen.ToString();
}

int main() {
    // Test 1: ratio=0 shows the top of content
    {
        // 10 lines of content in a 3-line viewport
        ftxui::Elements lines;
        for (int i = 0; i < 10; ++i) {
            lines.push_back(ftxui::text("Line " + std::to_string(i)));
        }
        auto content = ftxui::vbox(std::move(lines));
        auto el = direct_scroll(std::move(content), 0.0f);
        auto output = render(el, 20, 3);
        ASSERT_CONTAINS(output, "Line 0");
        ASSERT_CONTAINS(output, "Line 1");
        ASSERT_CONTAINS(output, "Line 2");
    }

    // Test 2: ratio=1 shows the bottom of content
    {
        ftxui::Elements lines;
        for (int i = 0; i < 10; ++i) {
            lines.push_back(ftxui::text("Line " + std::to_string(i)));
        }
        auto content = ftxui::vbox(std::move(lines));
        auto el = direct_scroll(std::move(content), 1.0f);
        auto output = render(el, 20, 3);
        ASSERT_CONTAINS(output, "Line 9");
    }

    // Test 3: ratio=0.5 shows middle content
    {
        ftxui::Elements lines;
        for (int i = 0; i < 10; ++i) {
            lines.push_back(ftxui::text("Line " + std::to_string(i)));
        }
        auto content = ftxui::vbox(std::move(lines));
        auto el = direct_scroll(std::move(content), 0.5f);
        auto output = render(el, 20, 3);
        // Should NOT contain the first or last line
        ASSERT_TRUE(output.find("Line 0") == std::string::npos);
        ASSERT_TRUE(output.find("Line 9") == std::string::npos);
    }

    // Test 4: content smaller than viewport — no scrolling needed
    {
        auto content = ftxui::vbox({
            ftxui::text("Only line"),
        });
        auto el = direct_scroll(std::move(content), 0.5f);
        auto output = render(el, 20, 5);
        ASSERT_CONTAINS(output, "Only line");
    }

    // Test 5: ratio clamped — negative ratio acts as 0
    {
        ftxui::Elements lines;
        for (int i = 0; i < 10; ++i) {
            lines.push_back(ftxui::text("Line " + std::to_string(i)));
        }
        auto content = ftxui::vbox(std::move(lines));
        auto el = direct_scroll(std::move(content), -1.0f);
        auto output = render(el, 20, 3);
        ASSERT_CONTAINS(output, "Line 0");
    }

    // Test 6: ratio clamped — ratio > 1 acts as 1
    {
        ftxui::Elements lines;
        for (int i = 0; i < 10; ++i) {
            lines.push_back(ftxui::text("Line " + std::to_string(i)));
        }
        auto content = ftxui::vbox(std::move(lines));
        auto el = direct_scroll(std::move(content), 5.0f);
        auto output = render(el, 20, 3);
        ASSERT_CONTAINS(output, "Line 9");
    }

    // Test 7: empty content doesn't crash
    {
        auto content = ftxui::vbox({});
        auto el = direct_scroll(std::move(content), 0.0f);
        auto output = render(el, 20, 3);
        (void)output; // just verify no crash
    }

    // Test 8: stencil clipping — content outside viewport is not rendered
    {
        ftxui::Elements lines;
        for (int i = 0; i < 10; ++i) {
            lines.push_back(ftxui::text("Row_" + std::to_string(i)));
        }
        auto content = ftxui::vbox(std::move(lines));
        auto el = direct_scroll(std::move(content), 0.0f);
        auto output = render(el, 20, 3);
        // Only first 3 rows should be visible
        ASSERT_CONTAINS(output, "Row_0");
        ASSERT_CONTAINS(output, "Row_2");
        ASSERT_TRUE(output.find("Row_5") == std::string::npos);
    }

    return 0;
}
