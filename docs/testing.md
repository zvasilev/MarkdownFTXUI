# Testing

## Philosophy

MarkdownFTXUI uses **CTest with standalone executables** -- no external test framework. Each test file is a plain C++ program with a `main()` function that returns 0 on success or 1 on failure.

This approach was chosen for simplicity:
- No test framework dependency to build or manage
- Each test is a standalone executable that can be debugged directly
- CTest provides test discovery, filtering, and parallel execution
- Minimal boilerplate -- just include `test_helper.hpp` and write assertions

## Test Helper Macros

All tests include `test_helper.hpp`, which provides three assertion macros:

### ASSERT_TRUE(expr)

Fails if `expr` evaluates to false. Prints the expression text and source location.

```cpp
ASSERT_TRUE(ast.children.size() > 0);
ASSERT_TRUE(output.find("Line 0") == std::string::npos);
```

### ASSERT_EQ(a, b)

Fails if `a != b`. Prints both actual and expected values along with source location. Handles enum types via `test_detail::printable()`, which casts enums to their underlying integer type for display.

```cpp
ASSERT_EQ(node.type, markdown::NodeType::Heading);
ASSERT_EQ(node.level, 2);
ASSERT_EQ(node.text, "Hello");
ASSERT_EQ(static_cast<int>(children.size()), 3);
```

### ASSERT_CONTAINS(haystack, needle)

Fails if `needle` is not found as a substring of `haystack`. Useful for checking rendered output.

```cpp
auto screen_output = screen.ToString();
ASSERT_CONTAINS(screen_output, "Hello World");
ASSERT_CONTAINS(screen_output, "bold text");
```

## Test Structure

Each test file follows this pattern:

```cpp
#include "test_helper.hpp"
#include "markdown/parser.hpp"  // or whatever header is being tested

using namespace markdown;

int main() {
    // Test 1: Description
    {
        auto parser = make_cmark_parser();
        auto ast = parser->parse("# Heading");
        ASSERT_EQ(ast.type, NodeType::Document);
        ASSERT_EQ(ast.children[0].type, NodeType::Heading);
        ASSERT_EQ(ast.children[0].level, 1);
    }

    // Test 2: Another test
    {
        // ...
    }

    return 0;  // All tests passed
}
```

Tests are grouped in blocks within `main()`. Each block is independent and can be read in isolation. Early return on failure (via `return 1` inside the macros) means the first failing assertion stops execution.

## Complete Test List

24 test executables covering the full library API:

### Parser Tests

| Test File | What It Tests |
|-----------|---------------|
| `test_trivial.cpp` | Sanity check: `true == true`. Verifies build and link. |
| `test_parser.cpp` | `MarkdownParser::parse()`: plain text, paragraphs, empty documents, multi-paragraph, fallback on failure. |

### AST / DOM Builder Tests

| Test File | What It Tests |
|-----------|---------------|
| `test_dom_builder.cpp` | `DomBuilder::build()`: basic rendering of paragraphs and text nodes to FTXUI Elements. |
| `test_headings.cpp` | Headings H1-H6: correct `NodeType::Heading`, `level` field, bold/dim decorators in rendered output. |
| `test_bold.cpp` | `**bold**` and `__bold__`: AST produces `NodeType::Strong`, rendered output contains bold text. |
| `test_italic.cpp` | `*italic*` and `_italic_`: AST produces `NodeType::Emphasis`, rendered output contains italic text. |
| `test_bold_italic.cpp` | `***bold italic***` and mixed nesting: AST produces nested Strong/Emphasis nodes. |
| `test_links.cpp` | `[text](url)`: AST structure, URL extraction, link nesting with bold/italic, DomBuilder link_targets(). |
| `test_lists.cpp` | `- item` unordered lists: AST structure, bullet prefix rendering, list item content. |
| `test_code.cpp` | `` `inline code` ``: AST produces `NodeType::CodeInline`, inverted rendering in output. |
| `test_quotes.cpp` | `> blockquote`: AST produces `NodeType::BlockQuote`, prefix rendering, nested formatting. |

### Editor Tests

| Test File | What It Tests |
|-----------|---------------|
| `test_highlight.cpp` | `highlight_markdown_syntax()`: syntax markers (`#`, `*`, `` ` ``) are colored, regular text is not. |
| `test_highlight_cursor.cpp` | `highlight_markdown_with_cursor()`: cursor rendering at various positions, line numbers gutter. |
| `test_editor.cpp` | `Editor` class: `content()`, `set_content()`, `cursor_line()`, `cursor_col()`, `total_lines()`, `cursor_position()`. |

### Viewer Tests

| Test File | What It Tests |
|-----------|---------------|
| `test_viewer.cpp` | `Viewer` class: content setting, rendering, scroll control, link callback, active/inactive state. |
| `test_dom_focus.cpp` | `DomBuilder` link focus highlighting: focused link gets inverted style, unfocused links get underlined only. |
| `test_tab_exit.cpp` | Tab focus integration: `on_tab_exit` forward/backward exit, `enter_focus` activation, round-trip cycling, Escape behavior, backward compatibility (no callback = wrap), custom key bindings via `ViewerKeys`. |
| `test_scroll_frame.cpp` | `DirectScrollFrame`: ratio 0 (top), ratio 0.5 (middle), ratio 1 (bottom), clamping, empty content, stencil clipping. |

### Theme Tests

| Test File | What It Tests |
|-----------|---------------|
| `test_theme.cpp` | Three built-in themes: `theme_default()`, `theme_high_contrast()`, `theme_colorful()`. Verifies names and that decorators apply without crashing. |

### Integration / Edge Case Tests

| Test File | What It Tests |
|-----------|---------------|
| `test_mixed.cpp` | All Markdown features combined in a single document: headings, bold, italic, links, lists, code, quotes interleaved. |
| `test_edge_cases.cpp` | Robustness: empty input, malformed Markdown, unsupported syntax (tables, HTML), deeply nested elements, very long lines. |
| `test_unicode.cpp` | Unicode handling: CJK characters, accented characters, wide characters, mixed ASCII/Unicode, multi-byte sequences. |
| `test_text_utils.cpp` | `text_utils.hpp` functions: `utf8_byte_length()`, `utf8_char_count()`, `utf8_display_width()`, `visual_col_to_byte()`, `split_lines()`, `gutter_width()`. |

## Running Tests

### Run all tests

```bash
ctest --test-dir build -C Release
```

### Verbose output (show failures)

```bash
ctest --test-dir build -C Release --output-on-failure
```

### Run a specific test

```bash
ctest --test-dir build -C Release -R test_parser
```

### Run tests matching a pattern

```bash
ctest --test-dir build -C Release -R "test_bold|test_italic"
```

### Run a test executable directly (for debugging)

```bash
# Windows
build\markdown\tests\Release\test_parser.exe

# Linux / macOS
./build/markdown/tests/test_parser
```

Direct execution gives the same output as CTest but allows debugger attachment.

## Adding a New Test

1. Create a new `.cpp` file in `markdown/tests/`:

```cpp
#include "test_helper.hpp"
#include "markdown/parser.hpp"  // include what you need

using namespace markdown;

int main() {
    // Test 1: Your test
    {
        auto parser = make_cmark_parser();
        auto ast = parser->parse("test input");
        ASSERT_EQ(ast.children.size(), 1u);
    }

    return 0;
}
```

2. Add three lines to `markdown/tests/CMakeLists.txt`:

```cmake
add_executable(test_my_feature test_my_feature.cpp)
target_link_libraries(test_my_feature PRIVATE markdown-ui)
add_test(NAME test_my_feature COMMAND test_my_feature)
```

3. Rebuild and run:

```bash
cmake --build build --config Release
ctest --test-dir build -C Release -R test_my_feature
```
