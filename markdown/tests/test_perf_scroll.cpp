#include "test_helper.hpp"
#include "markdown/viewer.hpp"
#include "markdown/parser.hpp"

#include <chrono>
#include <iostream>
#include <string>

#include <ftxui/screen/screen.hpp>
#include <ftxui/dom/elements.hpp>

using namespace markdown;

int main() {
    // Build a 2000-paragraph document with formatting
    std::string doc;
    for (int i = 0; i < 2000; ++i) {
        doc += "Paragraph " + std::to_string(i)
               + " with some content to make it nontrivial. "
               + "**Bold** and *italic* mixed in.\n\n";
    }

    Viewer viewer(make_cmark_parser());
    viewer.set_content(doc);
    viewer.show_scrollbar(true);
    auto comp = viewer.component();

    auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                        ftxui::Dimension::Fixed(40));

    // Cold render (parse + build + render)
    auto t_cold_start = std::chrono::high_resolution_clock::now();
    ftxui::Render(screen, comp->Render());
    auto t_cold_end = std::chrono::high_resolution_clock::now();
    double cold_ms = std::chrono::duration<double, std::milli>(
        t_cold_end - t_cold_start).count();
    std::cout << "Cold render (2000 paragraphs): " << cold_ms << " ms\n";

    // Warm scroll renders at various positions
    double total_scroll_ms = 0;
    int scroll_positions = 20;
    for (int i = 0; i < scroll_positions; ++i) {
        float ratio = static_cast<float>(i)
                    / static_cast<float>(scroll_positions - 1);
        viewer.set_scroll(ratio);

        auto t_start = std::chrono::high_resolution_clock::now();
        ftxui::Render(screen, comp->Render());
        auto t_end = std::chrono::high_resolution_clock::now();
        double ms = std::chrono::duration<double, std::milli>(
            t_end - t_start).count();
        total_scroll_ms += ms;
    }
    double per_scroll_ms = total_scroll_ms / scroll_positions;

    std::cout << "Warm scroll render (avg of " << scroll_positions
              << " positions): " << per_scroll_ms << " ms\n";
    std::cout << "Total scroll time: " << total_scroll_ms << " ms\n";

    // Budgets (generous for CI)
    ASSERT_TRUE(cold_ms < 5000.0);
    ASSERT_TRUE(per_scroll_ms < 200.0);

    return 0;
}
