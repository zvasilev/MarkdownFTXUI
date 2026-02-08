#include "test_helper.hpp"
#include "markdown/parser.hpp"
#include "markdown/dom_builder.hpp"

#include <ftxui/screen/screen.hpp>
#include <ftxui/dom/elements.hpp>

#include <fstream>
#include <sstream>

using namespace markdown;

static std::string read_file(std::string const& path) {
    std::ifstream f(path);
    if (!f.is_open()) {
        std::cerr << "FAIL: cannot open snippet file: " << path << "\n";
        return {};
    }
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

static int test_snippet(std::string const& path,
                        std::vector<std::string> const& expected_texts) {
    auto content = read_file(path);
    ASSERT_TRUE(!content.empty());

    auto parser = make_cmark_parser();

    // Phase 1: parse must succeed
    auto ast = parser->parse(content);
    ASSERT_EQ(ast.type, NodeType::Document);
    ASSERT_TRUE(ast.children.size() > 0);

    // Phase 2: build DOM and render without crash
    DomBuilder builder;
    auto element = builder.build(ast);
    auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(120),
                                        ftxui::Dimension::Fixed(200));
    ftxui::Render(screen, element);
    auto output = screen.ToString();
    ASSERT_TRUE(!output.empty());

    // Phase 3: key text fragments appear in rendered output
    for (auto const& text : expected_texts) {
        ASSERT_CONTAINS(output, text);
    }

    return 0;
}

int main(int argc, char* argv[]) {
    // Snippets directory can be overridden via argv[1], otherwise use
    // the compile-time default set by CMake.
    std::string snippets_dir;
    if (argc > 1) {
        snippets_dir = argv[1];
    } else {
#ifdef SNIPPETS_DIR
        snippets_dir = SNIPPETS_DIR;
#else
        std::cerr << "FAIL: no snippets directory (pass as arg or define SNIPPETS_DIR)\n";
        return 1;
#endif
    }
    // Ensure trailing slash/backslash
    if (!snippets_dir.empty() && snippets_dir.back() != '/' && snippets_dir.back() != '\\')
        snippets_dir += '/';

    // --- email1.md: Apple Developer newsletter ---
    {
        int rc = test_snippet(snippets_dir + "email1.md", {
            "MEET WITH APPLE",
            "LEVEL UP YOUR SKILLS",
            "SHOWCASE",
            "Liquid Glass",
            "Swift Student Challenge",
            "WATCH ANYTIME",
            "Privacy Policy",
            "Copyright",
        });
        if (rc != 0) return rc;
    }

    return 0;
}
