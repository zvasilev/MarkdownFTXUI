# API Reference

All public types live in the `markdown` namespace. Headers are in `markdown/include/markdown/`.

---

## ast.hpp -- AST Node Types

Defines the Markdown abstract syntax tree representation.

### NodeType (enum class)

```cpp
enum class NodeType {
    Document,        // Root container node
    Heading,         // Heading block (level 1-6)
    Paragraph,       // Paragraph block
    Text,            // Leaf text content
    Emphasis,        // Italic (*text* or _text_)
    Strong,          // Bold (**text** or __text__)
    StrongEmphasis,  // Bold + italic (***text***)
    Link,            // Hyperlink [text](url)
    ListItem,        // Single list item
    BulletList,      // Unordered list container
    OrderedList,     // Ordered list container
    CodeInline,      // Inline code (`code`)
    CodeBlock,       // Fenced code block (```)
    BlockQuote,      // Block quote (> text)
    SoftBreak,       // Soft line break (single newline)
    HardBreak,       // Hard line break (two spaces + newline)
    ThematicBreak,   // Horizontal rule (---)
    Image,           // Image (![alt](url))
};
```

### ASTNode (struct)

```cpp
struct ASTNode {
    NodeType type = NodeType::Document;
    std::string text;          // Leaf text content (Text, CodeInline, CodeBlock)
    std::string url;           // URL for Link and Image nodes
    int level = 0;             // Heading level (1-6), 0 for non-headings
    int list_start = 1;        // Starting number for OrderedList
    std::vector<ASTNode> children;
};
```

### MarkdownAST (type alias)

```cpp
using MarkdownAST = ASTNode;
```

The root node is always `NodeType::Document` with children representing top-level blocks.

### Example: Walking an AST

```cpp
#include "markdown/ast.hpp"
#include "markdown/parser.hpp"

void print_tree(markdown::ASTNode const& node, int depth = 0) {
    for (int i = 0; i < depth; ++i) std::cout << "  ";
    std::cout << static_cast<int>(node.type);
    if (!node.text.empty()) std::cout << " \"" << node.text << "\"";
    if (!node.url.empty()) std::cout << " url=" << node.url;
    if (node.level > 0) std::cout << " level=" << node.level;
    std::cout << "\n";
    for (auto const& child : node.children)
        print_tree(child, depth + 1);
}

auto parser = markdown::make_cmark_parser();
auto ast = parser->parse("# Hello **world**");
print_tree(ast);
// Output:
//   0           (Document)
//     1 level=1 (Heading)
//       3 "Hello " (Text)
//       5          (Strong)
//         3 "world" (Text)
```

---

## parser.hpp -- Markdown Parser

### MarkdownParser (abstract class)

```cpp
class MarkdownParser {
public:
    virtual ~MarkdownParser() = default;

    // Parse Markdown text into an AST.
    // Returns false if parsing failed; out will contain a raw-text fallback.
    virtual bool parse(std::string_view input, MarkdownAST& out) = 0;

    // Convenience overload: returns AST directly, ignoring success/failure.
    MarkdownAST parse(std::string_view input);
};
```

### make_cmark_parser()

```cpp
std::unique_ptr<MarkdownParser> make_cmark_parser();
```

Factory function that creates a parser backed by cmark-gfm. The cmark-gfm headers and types are completely hidden inside the implementation. This is the only way to obtain a parser instance.

### Example: Parsing Markdown

```cpp
#include "markdown/parser.hpp"

auto parser = markdown::make_cmark_parser();

// Two-overload usage:
// 1. Convenience (returns AST)
auto ast = parser->parse("**bold** and *italic*");

// 2. Output parameter (returns success flag)
markdown::MarkdownAST ast2;
bool ok = parser->parse("Some text", ast2);
if (!ok) {
    // Parsing failed -- ast2 contains a raw-text fallback
}
```

---

## viewer.hpp -- Markdown Viewer Component

### LinkEvent (enum class)

```cpp
enum class LinkEvent {
    Focus,  // Item received Tab focus
    Press,  // Item was activated with Enter
};
```

### ExternalFocusable (struct)

```cpp
struct ExternalFocusable {
    std::string label;  // Display label (e.g. "From")
    std::string value;  // Associated value (e.g. "alice@example.com")
};
```

External focusable items appear before links in the Tab ring. When focused, the `on_link_click` callback fires with the item's `value`.

### Viewer (class)

```cpp
class Viewer {
public:
    explicit Viewer(std::unique_ptr<MarkdownParser> parser);
```

#### Content Management

```cpp
    // Set the Markdown text to display.
    // Triggers re-parse on next render.
    void set_content(std::string_view markdown_text);
```

#### Scroll Control

```cpp
    // Set scroll position as a ratio: 0.0 = top, 1.0 = bottom.
    void set_scroll(float ratio);

    // Show or hide the vertical scroll indicator. Default: true.
    void show_scrollbar(bool show);

    // Query current scroll ratio.
    float scroll() const;

    // Query scrollbar visibility.
    bool scrollbar_visible() const;
```

#### Link Interaction

```cpp
    // Register a callback for link/external focus and press events.
    // The callback receives the URL (for links) or value (for externals)
    // and the event type (Focus or Press).
    void on_link_click(
        std::function<void(std::string const&, LinkEvent)> callback);
```

#### Theming

```cpp
    // Apply a theme. Only triggers re-build if the theme name changed.
    void set_theme(Theme const& theme);

    // Query current theme.
    Theme const& theme() const;
```

#### Component

```cpp
    // Get the FTXUI Component for embedding in layouts.
    // The component is created on first call and cached.
    ftxui::Component component();
```

#### Active State

```cpp
    // Whether the viewer is in "active" mode (accepting link navigation).
    bool active() const;
    void set_active(bool a);
```

#### External Focusables

```cpp
    // Add an external focusable item to the Tab ring.
    void add_focusable(std::string label, std::string value);

    // Remove all external focusable items and reset focus index.
    void clear_focusables();

    // Get the list of registered externals.
    std::vector<ExternalFocusable> const& externals() const;
```

#### Focus Tracking

```cpp
    // Unified focus index across externals and links:
    //   -1          = nothing focused
    //   0..N-1      = external item index
    //   N..N+M-1    = link index (where N = externals count, M = link count)
    int focused_index() const;

    // True if external item at the given index has focus.
    bool is_external_focused(int external_index) const;

    // Value of the currently focused item (external value or link URL).
    // Returns empty string if nothing is focused.
    std::string focused_value() const;

    // True if a link (not an external) is focused.
    bool is_link_focused() const;
```

#### Embed Mode

```cpp
    // In embed mode, the viewer skips its internal scroll frame and flex.
    // The caller is responsible for wrapping the element in a scroll container.
    // Useful for combining viewer content with other elements (like email headers)
    // in a single scrollable frame.
    void set_embed(bool embed);
    bool is_embed() const;
};
```

### Example: Basic Viewer

```cpp
#include "markdown/parser.hpp"
#include "markdown/viewer.hpp"

#include <ftxui/component/screen_interactive.hpp>

int main() {
    auto viewer = std::make_shared<markdown::Viewer>(
        markdown::make_cmark_parser());

    viewer->set_content("# Hello\n\nThis is **bold** and [a link](https://example.com).");
    viewer->show_scrollbar(true);

    viewer->on_link_click([](std::string const& url, markdown::LinkEvent ev) {
        if (ev == markdown::LinkEvent::Press) {
            // Handle link activation
        }
    });

    auto screen = ftxui::ScreenInteractive::Fullscreen();
    screen.Loop(viewer->component());
}
```

### Example: Viewer with External Focusables

```cpp
auto viewer = std::make_shared<markdown::Viewer>(
    markdown::make_cmark_parser());

viewer->set_content("Check the [docs](https://docs.example.com).");
viewer->add_focusable("From", "alice@example.com");
viewer->add_focusable("To", "bob@example.com");

viewer->on_link_click([](std::string const& value, markdown::LinkEvent ev) {
    // value is "alice@example.com", "bob@example.com", or the link URL
    // depending on which item is focused/pressed.
});

// In renderer, check which external is focused:
for (int i = 0; i < viewer->externals().size(); ++i) {
    if (viewer->is_external_focused(i)) {
        // Highlight this header
    }
}
```

---

## editor.hpp -- Markdown Editor Component

### Editor (class)

```cpp
class Editor {
public:
    explicit Editor();
```

#### Component

```cpp
    // Get the FTXUI Component for embedding in layouts.
    ftxui::Component component();
```

#### Content

```cpp
    // Read-only reference to the current document text.
    std::string const& content() const;

    // Replace the entire document content.
    void set_content(std::string text);
```

#### Cursor Information

```cpp
    // Current cursor line (1-based).
    int cursor_line() const;

    // Current cursor column (1-based).
    int cursor_col() const;

    // Cursor byte offset in the text.
    int cursor_position() const;

    // Total number of lines in the document.
    int total_lines() const;
```

#### State

```cpp
    // Whether the editor is in "active" mode (accepting text input).
    // Enter activates, Esc deactivates.
    bool active() const;
```

#### Cursor Manipulation

```cpp
    // Set cursor position by byte offset.
    void set_cursor_position(int byte_offset);

    // Set cursor position by line and column (1-based).
    void set_cursor(int line, int col);
```

#### Theming

```cpp
    // Apply a theme for syntax highlighting colors.
    void set_theme(Theme const& theme);
};
```

### Example: Editor with Live Preview

```cpp
#include "markdown/editor.hpp"
#include "markdown/viewer.hpp"
#include "markdown/parser.hpp"

auto editor = std::make_shared<markdown::Editor>();
editor->set_content("# My Document\n\nStart typing...");

auto viewer = std::make_shared<markdown::Viewer>(
    markdown::make_cmark_parser());

// In a Renderer lambda:
viewer->set_content(editor->content());  // Live update

// Sync scroll to cursor position:
float ratio = 0.0f;
if (editor->total_lines() > 1) {
    ratio = static_cast<float>(editor->cursor_line() - 1) /
            static_cast<float>(editor->total_lines() - 1);
}
viewer->set_scroll(ratio);
```

---

## dom_builder.hpp -- AST to FTXUI DOM

### LinkTarget (struct)

```cpp
struct LinkTarget {
    std::vector<ftxui::Box> boxes;  // Bounding boxes on screen
    std::string url;                // The link URL
};
```

Each link in the rendered output produces a `LinkTarget` with one or more bounding boxes (a link that wraps across lines will have multiple boxes).

### DomBuilder (class)

```cpp
class DomBuilder {
public:
    // Convert an AST into an FTXUI Element tree.
    // focused_link: index of link to highlight (-1 for none).
    // theme: styling configuration.
    ftxui::Element build(MarkdownAST const& ast,
                         int focused_link = -1,
                         Theme const& theme = theme_default());

    // Query link targets after build().
    // Returns the list of links found during the last build.
    std::vector<LinkTarget> const& link_targets() const;
};
```

### Example: Direct DOM Building

```cpp
#include "markdown/parser.hpp"
#include "markdown/dom_builder.hpp"

#include <ftxui/screen/screen.hpp>

auto parser = markdown::make_cmark_parser();
auto ast = parser->parse("Hello **world** with a [link](https://example.com)");

markdown::DomBuilder builder;
auto element = builder.build(ast, /*focused_link=*/0, markdown::theme_colorful());

// Render to a screen
auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(60),
                                     ftxui::Dimension::Fixed(5));
ftxui::Render(screen, element);
std::cout << screen.ToString() << std::endl;

// Query link positions
for (auto const& link : builder.link_targets()) {
    std::cout << "Link: " << link.url
              << " at " << link.boxes.size() << " box(es)\n";
}
```

### Rendering Rules

| AST Node | FTXUI Rendering |
|----------|-----------------|
| Heading (level 1) | `theme.heading1` decorator (default: bold + underlined) |
| Heading (level 2) | `theme.heading2` decorator (default: bold) |
| Heading (level 3+) | `theme.heading3` decorator (default: bold + dim) |
| Strong | `ftxui::bold` |
| Emphasis | `ftxui::italic` (where supported) |
| StrongEmphasis | `ftxui::bold` (italic not always combined) |
| Link | `ftxui::underlined` + `theme.link` + `ftxui::reflect()` for boxes |
| Link (focused) | adds `ftxui::inverted` + `ftxui::focus` |
| BulletList | `"  * "` prefix per item, indentation for nesting |
| OrderedList | `"  N. "` prefix per item, numbering from `list_start` |
| CodeInline | `theme.code_inline` decorator (default: inverted) |
| CodeBlock | bordered box + `theme.code_block` decorator |
| BlockQuote | `"  \| "` prefix + `theme.blockquote` decorator |
| ThematicBreak | `ftxui::separator()` |
| Image | `"[img: alt_text]"` placeholder |
| SoftBreak | single space |
| HardBreak | line break |

---

## highlight.hpp -- Lexical Syntax Highlighting

### highlight_markdown_syntax()

```cpp
ftxui::Element highlight_markdown_syntax(
    std::string_view text,
    Theme const& theme = theme_default());
```

Highlights Markdown syntax markers in raw text. Returns an FTXUI Element where syntax characters (`#`, `*`, `_`, `` ` ``, `[`, `]`, `(`, `)`, `>`, `-`) are styled with `theme.syntax`. Regular text is unstyled.

This is **lexical** highlighting -- it colors characters based on pattern matching, not AST structure. It does not understand nesting or context.

### highlight_markdown_with_cursor()

```cpp
ftxui::Element highlight_markdown_with_cursor(
    std::string_view text,
    int cursor_position,         // Byte offset of cursor in text
    bool focused,                // Whether cursor should be visible
    bool hovered,                // Whether hover styling applies
    bool show_line_numbers = false,
    Theme const& theme = theme_default());
```

Same as `highlight_markdown_syntax()` but with an embedded cursor at the given byte position. When `focused` is true, the character at the cursor position is rendered with inverted colors. When `show_line_numbers` is true, a line number gutter is prepended.

Used internally by the Editor's `InputOption::transform` callback.

### Example: Rendering Highlighted Text

```cpp
#include "markdown/highlight.hpp"

#include <ftxui/screen/screen.hpp>

std::string text = "# Hello **world**";
auto element = markdown::highlight_markdown_syntax(text);

auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(40),
                                     ftxui::Dimension::Fixed(1));
ftxui::Render(screen, element);
// Output: "# Hello **world**" with # and ** in yellow+dim
```

---

## theme.hpp -- Theme Configuration

### Theme (struct)

```cpp
struct Theme {
    std::string name;               // Theme identifier (used for change detection)

    // Editor syntax highlighting
    ftxui::Decorator syntax;        // Color for syntax markers (#, *, `, etc.)
    ftxui::Decorator gutter;        // Color for line number gutter

    // Viewer elements
    ftxui::Decorator heading1;      // H1 style
    ftxui::Decorator heading2;      // H2 style
    ftxui::Decorator heading3;      // H3-H6 style
    ftxui::Decorator link;          // Link style (added on top of underlined)
    ftxui::Decorator code_inline;   // Inline code style
    ftxui::Decorator code_block;    // Code block content style
    ftxui::Decorator blockquote;    // Block quote text style
};
```

### Built-in Themes

```cpp
Theme const& theme_default();        // "Default" -- minimal, works everywhere
Theme const& theme_high_contrast();  // "Contrast" -- bold + white, high visibility
Theme const& theme_colorful();       // "Colorful" -- color-coded elements
```

| Field | Default | Contrast | Colorful |
|-------|---------|----------|----------|
| syntax | yellow + dim | white + bold | green |
| gutter | dim | white | blue + dim |
| heading1 | bold + underlined | bold + underlined | bold + underlined + cyan |
| heading2 | bold | bold | bold + green |
| heading3 | bold + dim | bold | bold + yellow |
| link | nothing | bold | magenta |
| code_inline | inverted | inverted | inverted + yellow |
| code_block | nothing | nothing | green |
| blockquote | dim | nothing | blue |

### Example: Custom Theme

```cpp
#include "markdown/theme.hpp"

markdown::Theme my_theme{
    "MyTheme",
    ftxui::color(ftxui::Color::Red),                              // syntax
    ftxui::dim,                                                    // gutter
    ftxui::Decorator(ftxui::bold) | ftxui::underlined,            // heading1
    ftxui::bold,                                                   // heading2
    ftxui::Decorator(ftxui::bold) | ftxui::dim,                   // heading3
    ftxui::color(ftxui::Color::Blue),                              // link
    ftxui::inverted,                                               // code_inline
    ftxui::nothing,                                                // code_block
    ftxui::dim,                                                    // blockquote
};

viewer->set_theme(my_theme);
editor->set_theme(my_theme);
```

> **MSVC note:** When composing decorators, `ftxui::bold | ftxui::underlined` fails on MSVC because both are raw function pointers. Wrap the first operand: `ftxui::Decorator(ftxui::bold) | ftxui::underlined`. Decorators returned by `ftxui::color()` are already `std::function` and compose normally.

---

## scroll_frame.hpp -- Custom Scroll Container

### DirectScrollFrame (class)

```cpp
class DirectScrollFrame : public ftxui::Node {
public:
    DirectScrollFrame(ftxui::Element child, float ratio);
};
```

A custom FTXUI Node that scrolls its child by a direct ratio. The scroll offset is computed as `ratio * scrollable_height`, where `scrollable_height = content_height - viewport_height`. The ratio is clamped to [0.0, 1.0].

Unlike FTXUI's `yframe`, which centers the element that has `ftxui::focus`, DirectScrollFrame ignores focus and positions purely by ratio.

### direct_scroll()

```cpp
ftxui::Element direct_scroll(ftxui::Element child, float ratio);
```

Convenience function that creates a DirectScrollFrame.

### Example: Scrollable Content

```cpp
#include "markdown/scroll_frame.hpp"

ftxui::Elements lines;
for (int i = 0; i < 100; ++i) {
    lines.push_back(ftxui::text("Line " + std::to_string(i)));
}

float scroll_ratio = 0.5f;  // Middle of content
auto scrollable = markdown::direct_scroll(
    ftxui::vbox(std::move(lines)) | ftxui::vscroll_indicator,
    scroll_ratio);

// Render in a fixed viewport
auto screen = ftxui::Screen::Create(ftxui::Dimension::Fixed(40),
                                     ftxui::Dimension::Fixed(10));
ftxui::Render(screen, scrollable);
```

---

## text_utils.hpp -- UTF-8 Utilities

All functions are `inline` and header-only.

### Character Length and Counting

```cpp
// Byte length of the UTF-8 character starting at leading_byte.
size_t utf8_byte_length(char leading_byte);

// Count UTF-8 characters (not bytes) in text.
int utf8_char_count(std::string_view text);

// Convert character index (0-based) to byte offset.
size_t utf8_char_to_byte(std::string_view text, int char_index);
```

### Unicode Codepoint

```cpp
// Decode UTF-8 codepoint starting at data[0..len).
uint32_t utf8_codepoint(const char* data, size_t len);

// Terminal display width of a Unicode codepoint.
// Returns 2 for CJK Unified Ideographs, Fullwidth forms, Hangul,
// Hiragana, Katakana, and other East Asian Wide characters.
// Returns 1 for everything else.
int codepoint_width(uint32_t cp);
```

### Display Width

```cpp
// Total terminal display width of a UTF-8 string.
// Accounts for wide characters (CJK etc.) that occupy 2 columns.
int utf8_display_width(std::string_view text);

// Map terminal visual column (0-based) to byte offset in text.
// Accounts for wide characters.
size_t visual_col_to_byte(std::string_view text, int col);
```

### Line Utilities

```cpp
// Split text into lines (without newline characters).
// A trailing newline produces an empty final element.
std::vector<std::string_view> split_lines(std::string_view text);
```

### Gutter Utilities

```cpp
// Number of digits needed to display total_lines (e.g. 100 -> 3).
int gutter_width(int total_lines);

// Total gutter width: digits + 3 (for " | " separator).
int gutter_chars(int total_lines);
```

### Example: UTF-8 Width Calculation

```cpp
#include "markdown/text_utils.hpp"

// ASCII
assert(markdown::utf8_display_width("Hello") == 5);

// CJK characters (2 columns each)
assert(markdown::utf8_display_width("\xe4\xb8\x96\xe7\x95\x8c") == 4);  // "世界"

// Mixed
assert(markdown::utf8_display_width("Hi\xe4\xb8\x96") == 4);  // "Hi世" = 2 + 2
```
