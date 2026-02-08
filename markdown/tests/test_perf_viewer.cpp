#include "test_helper.hpp"
#include "markdown/viewer.hpp"
#include "markdown/parser.hpp"

#include <chrono>
#include <iostream>
#include <string>

#include <ftxui/screen/screen.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/component/event.hpp>

using namespace markdown;

int main() {
    // Build a document with 200 links
    std::string doc;
    for (int i = 0; i < 200; ++i) {
        doc += "- [link" + std::to_string(i) + "](https://example.com/"
               + std::to_string(i) + ")\n";
    }

    Viewer viewer(make_cmark_parser());
    viewer.set_content(doc);
    auto comp = viewer.component();

    // Initial render to populate link targets
    auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(80),
                                        ftxui::Dimension::Fixed(40));
    ftxui::Render(screen, comp->Render());

    // Activate the viewer for link cycling
    viewer.set_active(true);

    // Time: Tab through all 200 links, rendering after each
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < 200; ++i) {
        comp->OnEvent(ftxui::Event::Tab);
        ftxui::Render(screen, comp->Render());
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto total_ms = std::chrono::duration<double, std::milli>(end - start).count();
    double per_tab_ms = total_ms / 200.0;

    std::cout << "Viewer focus cycling: " << per_tab_ms
              << " ms/tab (" << total_ms << " ms total for 200 links)\n";

    // Budget: 100ms per tab, 5s total (generous for CI)
    ASSERT_TRUE(per_tab_ms < 100.0);
    ASSERT_TRUE(total_ms < 5000.0);

    return 0;
}
