# Terminal Markdown Editor & Viewer Library (FTXUI)

## 1. Purpose

Build a **terminal-based Markdown editor and live viewer** using **FTXUI**, intended for plain-text planning documents (notes, TODOs, design plans).

The project must be:

- Implemented from scratch
- Written in modern C++ (see §5 for standard details)
- Structured as a reusable, open-source library
- Cross-platform (Windows, macOS, Linux)
- Backed by tests for every feature

---

## 2. High-level Goals

- Provide two UI components:
  1. A lightweight Markdown **Editor**
  2. A semantic Markdown **Viewer**
- Render both components side by side in a demo application
- Viewer must update immediately on editor changes (with debounce — see §10)
- Focus on readability, not full Markdown compliance

---

## 3. Explicit Non-Goals (Out of Scope)

The following are intentionally excluded:

- Tables
- Images
- HTML blocks
- Footnotes
- Math / LaTeX
- Syntax highlighting for programming languages
- Bidirectional editor ↔ viewer mapping
- Cursor-to-AST synchronization
- Ordered lists (numbered `1. 2. 3.` — only unordered bullet lists are supported)
- Fenced code blocks (` ``` `) — only inline code is supported
- Horizontal rules (`---`, `***`)
- Nested lists (only single-level bullet lists)
- File I/O (load/save) — the demo works with in-memory text only
- Undo/redo beyond what FTXUI's Input provides natively

---

## 4. Platform & Build Requirements

### Supported platforms

| Platform | Toolchain              | Minimum version         |
| -------- | ---------------------- | ----------------------- |
| Windows  | MSVC + CMake           | Visual Studio 2022 17.8 |
| macOS    | Clang + CMake + make   | Clang 17                |
| Linux    | GCC or Clang + CMake   | GCC 13 / Clang 17       |

### CMake version

- Minimum CMake 3.25

### Build commands

```
cmake -S . -B build -DCMAKE_CXX_STANDARD=23
cmake --build build
ctest --test-dir build
```

### Build constraints

- No platform-specific UI code
- No POSIX-only APIs
- No compiler extensions

---

## 5. Language & Style Requirements

### Standard: C++23

The following C++23 features are expected to be used. The project does not depend on features with spotty compiler support beyond these:

- `std::expected` (error handling in parser)
- `std::string_view` (pervasive)
- `std::optional`, `std::variant` (AST node types)
- Deducing `this` (if useful for CRTP-free patterns, optional)
- `constexpr` standard library improvements

If a C++23 feature is unavailable on a target compiler, fall back to C++20 equivalents (e.g., use a `Result<T,E>` alias over `tl::expected` as a polyfill for `std::expected`).

### Forbidden

- Raw owning pointers
- Manual memory management (`new`/`delete`)
- C-style casts
- Preprocessor macros for logic
- Global mutable state

### Preferred

- `std::string_view` for non-owning string parameters
- `std::optional`, `std::variant`, `std::expected`
- RAII everywhere
- Value types over pointer indirection where practical
- Explicit ownership via `std::unique_ptr` only when polymorphism requires it
- `constexpr` where appropriate

---

## 6. Dependencies

### UI

- **FTXUI** — DOM-based rendering only (no Canvas)
- Integrated via CMake FetchContent

### Markdown parsing

- **cmark-gfm** — [https://github.com/github/cmark-gfm](https://github.com/github/cmark-gfm)
- Integrated via CMake FetchContent
- Must be **fully abstracted** behind `MarkdownParser` (§8.3) — no cmark types leak outside `parser_cmark.cpp`

### Testing

- **CTest only** — no external test framework
- Each test is a standalone executable that returns 0 on success, non-zero on failure
- A minimal `test_helper.hpp` provides `ASSERT_EQ`, `ASSERT_TRUE`, etc. as simple macros
- Registered via `add_test()` in CMake

---

## 7. Project Structure

```
markdown-ui/
├── CMakeLists.txt
├── markdown/
│   ├── CMakeLists.txt
│   ├── include/markdown/
│   │   ├── editor.hpp
│   │   ├── viewer.hpp
│   │   ├── parser.hpp          # MarkdownParser interface
│   │   ├── ast.hpp             # MarkdownAST and node types
│   │   ├── dom_builder.hpp     # AST → FTXUI DOM
│   │   └── highlight.hpp       # Lexical syntax highlighting
│   ├── src/
│   │   ├── editor.cpp
│   │   ├── viewer.cpp
│   │   ├── parser_cmark.cpp    # cmark-gfm implementation of MarkdownParser
│   │   ├── dom_builder.cpp
│   │   └── highlight.cpp
│   └── tests/
│       ├── CMakeLists.txt
│       ├── test_helper.hpp     # Minimal assert macros for CTest
│       ├── test_parser.cpp     # Parser → AST correctness
│       ├── test_dom_builder.cpp# AST → FTXUI DOM correctness
│       ├── test_highlight.cpp  # Editor lexical highlighting
│       ├── test_headings.cpp
│       ├── test_bold.cpp
│       ├── test_italic.cpp
│       ├── test_bold_italic.cpp
│       ├── test_links.cpp
│       ├── test_lists.cpp
│       ├── test_code.cpp
│       ├── test_quotes.cpp
│       ├── test_mixed.cpp
│       ├── test_unicode.cpp
│       └── test_edge_cases.cpp # Empty, whitespace, malformed input
├── demo/
│   ├── CMakeLists.txt
│   └── main.cpp
└── .github/
    └── workflows/
        └── ci.yml              # GitHub Actions: Windows, macOS, Linux
```

---

## 8. Core Components

### 8.1 Editor Component

**Responsibilities**

- Wrap `ftxui::Input` as the text entry mechanism
- Apply lexical (non-semantic) syntax highlighting
- Expose the current text content as `std::string const&`
- Support standard FTXUI Input behavior: cursor movement, line wrapping, multi-line editing

**What the editor does NOT do**

- Parse Markdown
- Validate Markdown correctness
- Provide undo/redo beyond FTXUI's built-in behavior
- Provide custom key bindings

**Highlighting rules (lexical only)**

Highlight the following characters with a distinct color/dim style when they appear in raw text:

- `*`, `_` (emphasis markers)
- `` ` `` (code markers)
- `[`, `]`, `(`, `)` (link markers)
- `#` at the start of a line (heading markers)
- `>` at the start of a line (blockquote markers)
- `-` at the start of a line followed by a space (list markers)

Highlighting is implemented in `highlight.hpp`/`.cpp` as a pure function:

```cpp
// Takes raw text, returns FTXUI Elements with highlighted characters
ftxui::Element highlight_markdown_syntax(std::string_view text);
```

**Interface sketch**

```cpp
class Editor {
public:
    explicit Editor();

    // Returns the FTXUI component for embedding in a layout
    ftxui::Component component();

    // Current document text (read-only reference)
    std::string const& content() const;

    // Set content programmatically (e.g., for testing)
    void set_content(std::string text);
};
```

---

### 8.2 Viewer Component

**Responsibilities**

- Accept Markdown text
- Parse it into a `MarkdownAST`
- Convert the AST into FTXUI DOM nodes via `DomBuilder`
- Render the result in a scrollable container

**Rendering rules**

| Markdown construct    | FTXUI rendering                              |
| --------------------- | -------------------------------------------- |
| `# Heading 1`        | `ftxui::bold`, large-style (all caps or `═` underline) |
| `## Heading 2`       | `ftxui::bold`                                |
| `### Heading 3–6`    | `ftxui::bold \| ftxui::dim`                  |
| `**bold**`           | `ftxui::bold`                                |
| `*italic*` / `_italic_` | `ftxui::italic`                           |
| `***bold italic***`  | `ftxui::bold \| ftxui::italic`               |
| `[text](url)`        | `ftxui::underlined` (URL discarded in display) |
| `- item`             | `  • ` prefix + indentation                  |
| `` `code` ``         | `ftxui::inverted` (reversed foreground/background) |
| `> quote`            | `ftxui::borderLeft` or `│ ` prefix + `ftxui::dim` |
| Plain text           | `ftxui::paragraph`                           |

**Interface sketch**

```cpp
class Viewer {
public:
    explicit Viewer(std::unique_ptr<MarkdownParser> parser);

    // Update the displayed content. Triggers re-parse + re-render.
    void set_content(std::string_view markdown);

    // Returns the FTXUI component for embedding in a layout
    ftxui::Component component();
};
```

---

### 8.3 Markdown Parser Abstraction

```cpp
// ast.hpp

enum class NodeType {
    Document,
    Heading,       // level: 1–6
    Paragraph,
    Text,
    Emphasis,      // italic
    Strong,        // bold
    StrongEmphasis,// bold + italic
    Link,          // url stored in node
    ListItem,
    BulletList,
    CodeInline,
    BlockQuote,
    SoftBreak,
    HardBreak,
};

struct ASTNode {
    NodeType type;
    std::string text;                        // leaf text content
    std::string url;                         // for Link nodes
    int level = 0;                           // for Heading nodes (1–6)
    std::vector<ASTNode> children;           // child nodes
};

// The full AST is simply the root Document node
using MarkdownAST = ASTNode;
```

```cpp
// parser.hpp

class MarkdownParser {
public:
    virtual ~MarkdownParser() = default;
    virtual MarkdownAST parse(std::string_view input) = 0;
};
```

- `MarkdownAST` is a library-defined tree of `ASTNode` values
- cmark-gfm types are completely hidden inside `parser_cmark.cpp`
- Enables future parser replacement by swapping the implementation

---

### 8.4 DOM Builder

Converts a `MarkdownAST` into an `ftxui::Element` tree.

```cpp
// dom_builder.hpp

class DomBuilder {
public:
    // Convert an AST into an FTXUI element tree ready for rendering
    ftxui::Element build(MarkdownAST const& ast);
};
```

**Responsibilities**

- Walk the AST recursively
- Map each `NodeType` to the corresponding FTXUI DOM construction (per the table in §8.2)
- Handle nesting (e.g., bold text inside a list item inside a blockquote)
- Produce a single `ftxui::Element` suitable for embedding in a `vbox` / scrollable container

**This is a pure transformation**: AST in, Element out. No state, no side effects.

---

## 9. Unicode & Character Positioning

### Critical design decision

> Editor and Viewer are intentionally decoupled.

- Editor works on raw UTF-8 text (FTXUI handles rendering)
- Viewer parses the same raw text via cmark-gfm (which operates on UTF-8)
- No shared character index mapping between editor and viewer

### Why

- cmark-gfm reports byte offsets
- UTF-8 characters may span multiple bytes
- Terminal rendering uses grapheme width (wcwidth)
- Mapping bytes → terminal columns is complex and fragile

### Result

- Unicode-safe rendering in both panes
- No offset synchronization bugs
- Simpler design

---

## 10. Demo Application

### Requirements

- Two panes: Editor (left), Viewer (right)
- Shared document state: a single `std::string` owned by the demo's main function
- Viewer updates when editor content changes
- **Debounce**: viewer re-parses at most once per 50ms to avoid flicker during fast typing
- Focus: editor pane is focused by default; viewer is display-only (not focusable)
- Exit: `Ctrl+C` or `Ctrl+Q` exits the application

### Shared state design

```cpp
// In main.cpp
std::string document_text;
Editor editor;
Viewer viewer(std::make_unique<CmarkParser>());

// On each FTXUI render cycle:
// 1. Read editor.content()
// 2. If changed since last viewer update AND debounce elapsed:
//    viewer.set_content(editor.content());
// 3. Render both side by side
```

### Layout

```
┌─ Markdown Editor ────────┐┌─ Markdown Viewer ─────────┐
│ # Plan                   ││ ═══════════════            │
│                          ││ PLAN                       │
│ > Focus on **important** ││ ═══════════════            │
│   tasks                  ││                            │
│                          ││ │ Focus on important tasks │
│ - Write *code*           ││                            │
│ - Review `tests`         ││   • Write code             │
│ - Read [docs](url)       ││   • Review  tests          │
│                          ││   • Read docs              │
└──────────────────────────┘└────────────────────────────┘
```

---

## 11. Implementation Plan (Phases)

Each phase builds on the previous, adds exactly one capability, and includes tests.

---

### Phase 0 — Project Wiring

**Scope**

- Top-level `CMakeLists.txt` with FetchContent for FTXUI and cmark-gfm
- Library target `markdown-ui` under `markdown/`
- Test target under `markdown/tests/`
- Demo target under `demo/`
- GitHub Actions CI config for Windows (MSVC), macOS (Clang), Linux (GCC)

**Deliverable**

- `cmake --build build` succeeds on all three platforms
- `ctest --test-dir build` runs and passes (a single trivial test)

**Test**

```cpp
#include "test_helper.hpp"

int main() {
    ASSERT_TRUE(true);
    return 0;
}
```

---

### Phase 1 — Parser Abstraction + Plain Text

**Scope**

- Define `ASTNode`, `NodeType`, `MarkdownAST` in `ast.hpp`
- Define `MarkdownParser` interface in `parser.hpp`
- Implement `CmarkParser` in `parser_cmark.cpp`
- For this phase, only handle `Document`, `Paragraph`, `Text` node types
- Implement `DomBuilder::build()` — for now, only `paragraph()` output

**Tests** (`test_parser.cpp`, `test_dom_builder.cpp`)

```
Input:    "Hello world"
AST:      Document → Paragraph → Text("Hello world")
DOM:      paragraph("Hello world")
```

```
Input:    "Line one\n\nLine two"
AST:      Document → [Paragraph → Text("Line one"), Paragraph → Text("Line two")]
```

```
Input:    ""
AST:      Document (no children)
```

```
Input:    "   \n\n  "
AST:      Document (no children or empty paragraph)
```

---

### Phase 2 — Headings

**Scope**

- Parse `# H1` through `###### H6`
- `ASTNode` with `type = Heading`, `level = 1..6`
- `DomBuilder`: H1 → bold + all caps or separator line, H2 → bold, H3–H6 → bold + dim

**Tests** (`test_headings.cpp`)

```
Input:    "# Title"
AST:      Document → Heading(level=1) → Text("Title")
```

```
Input:    "## Section\n\nBody text"
AST:      Document → [Heading(level=2) → Text("Section"), Paragraph → Text("Body text")]
```

```
Input:    "###### Deep"
AST:      Document → Heading(level=6) → Text("Deep")
```

---

### Phase 3 — Bold (`**text**` / `__text__`)

**Scope**

- Parse strong emphasis
- `ASTNode` with `type = Strong`
- `DomBuilder`: wrap children in `ftxui::bold`

**Tests** (`test_bold.cpp`)

```
Input:    "This is **important**"
AST:      Document → Paragraph → [Text("This is "), Strong → Text("important")]
DOM:      Contains bold("important")
```

```
Input:    "**full bold**"
AST:      Document → Paragraph → Strong → Text("full bold")
```

---

### Phase 4 — Italic (`*text*` / `_text_`)

**Scope**

- Parse emphasis
- `ASTNode` with `type = Emphasis`
- `DomBuilder`: wrap children in `ftxui::italic`

**Tests** (`test_italic.cpp`)

```
Input:    "*this matters*"
AST:      Document → Paragraph → Emphasis → Text("this matters")
```

```
Input:    "A _subtle_ point"
AST:      Document → Paragraph → [Text("A "), Emphasis → Text("subtle"), Text(" point")]
```

---

### Phase 5 — Bold + Italic Nesting

**Scope**

- Handle `***bold and italic***`
- Handle `**bold with *italic* inside**`
- Handle `*italic with **bold** inside*`

**Tests** (`test_bold_italic.cpp`)

```
Input:    "***both***"
AST:      Document → Paragraph → Strong → Emphasis → Text("both")
DOM:      bold(italic("both"))
```

```
Input:    "**bold and *italic* here**"
AST:      Document → Paragraph → Strong → [Text("bold and "), Emphasis → Text("italic"), Text(" here")]
```

---

### Phase 6 — Links

**Scope**

- Parse `[text](url)`
- `ASTNode` with `type = Link`, `url` field populated
- `DomBuilder`: render link text as `ftxui::underlined`, URL is not displayed

**Tests** (`test_links.cpp`)

```
Input:    "[Docs](https://example.com)"
AST:      Document → Paragraph → Link(url="https://example.com") → Text("Docs")
DOM:      underlined("Docs")
```

```
Input:    "See [**bold link**](url)"
AST:      Document → Paragraph → [Text("See "), Link → Strong → Text("bold link")]
DOM:      underlined(bold("bold link"))
```

---

### Phase 7 — Unordered Lists

**Scope**

- Parse `- item` and `* item`
- `ASTNode` types: `BulletList` containing `ListItem` children
- `DomBuilder`: `  • ` prefix per item

**Tests** (`test_lists.cpp`)

```
Input:    "- one\n- two\n- three"
AST:      Document → BulletList → [ListItem → Paragraph → Text("one"), ...]
DOM:      vbox(["  • one", "  • two", "  • three"])
```

```
Input:    "- **bold** item"
AST:      Document → BulletList → ListItem → Paragraph → [Strong → Text("bold"), Text(" item")]
```

**Edge case**: nested list input (not supported) should render flat or degrade gracefully without crashing.

---

### Phase 8 — Inline Code

**Scope**

- Parse `` `code` ``
- `ASTNode` with `type = CodeInline`
- `DomBuilder`: `ftxui::inverted`

**Tests** (`test_code.cpp`)

```
Input:    "Use `ls -la`"
AST:      Document → Paragraph → [Text("Use "), CodeInline("ls -la")]
DOM:      Contains inverted("ls -la")
```

```
Input:    "`single`"
AST:      Document → Paragraph → CodeInline("single")
```

---

### Phase 9 — Block Quotes

**Scope**

- Parse `> text`
- `ASTNode` with `type = BlockQuote`
- `DomBuilder`: left border or `│ ` prefix + dim styling

**Tests** (`test_quotes.cpp`)

```
Input:    "> note"
AST:      Document → BlockQuote → Paragraph → Text("note")
DOM:      borderLeft + dim("note")
```

```
Input:    "> **important** note"
AST:      Document → BlockQuote → Paragraph → [Strong → Text("important"), Text(" note")]
```

---

### Phase 10 — Editor with Lexical Highlighting

**Scope**

- Implement `highlight_markdown_syntax()` in `highlight.cpp`
- Integrate highlighting into the `Editor` component
- Wrap `ftxui::Input` so that displayed text has syntax characters colored differently

**Tests** (`test_highlight.cpp`)

```
Input:    "**bold** and *italic*"
Result:   ** are highlighted, bold is normal, * are highlighted, italic is normal
```

```
Input:    "[link](url)"
Result:   [, ], (, ) are highlighted
```

```
Input:    "# Heading"
Result:   # is highlighted
```

---

### Phase 11 — Demo Application (Integration)

**Scope**

- Wire Editor + Viewer in `demo/main.cpp`
- Side-by-side layout with `ftxui::hbox`
- Shared `std::string` state with debounced viewer update
- `Ctrl+C` / `Ctrl+Q` to quit

**Manual verification** (not automated)

- Type Markdown in the editor, see formatted output update live in the viewer
- Unicode input renders correctly in both panes
- Rapid typing does not cause visible flicker

---

### Phase 12 — Mixed Content & Edge Case Tests

**Scope**

- Integration tests combining all features
- Edge case tests for robustness

**Tests** (`test_mixed.cpp`)

```
Input:
# Plan

> Focus on **important** tasks

- Write *code*
- Review `tests`
- Read [docs](url)

Expected: All features render correctly, correct nesting, no crashes
```

**Tests** (`test_edge_cases.cpp`)

```
Input:    ""                              → Empty document, no crash
Input:    "   \n\n\n   "                  → Whitespace only, no crash
Input:    "**unclosed bold"               → Best-effort render, no crash
Input:    "* * * *"                       → Graceful handling (not a horizontal rule since it's a non-goal)
Input:    "1. ordered"                    → Rendered as plain text or best-effort (not supported)
Input:    "```\ncode block\n```"          → Rendered as plain text or best-effort (not supported)
Input:    "- nested\n  - list"            → Flat rendering, no crash
```

**Unicode tests** (`test_unicode.cpp`)

```
Input:    "**重要** task"                  → Bold applied to 重要, "task" is plain
Input:    "*émphasis*"                    → Italic applied correctly
Input:    "- café\n- naïve"              → List renders with correct characters
Input:    "# Ünïcödé"                    → Heading renders correctly
```

---

## 12. Testing Strategy

### Framework

- CTest with standalone test executables — no external test framework
- All tests run via `ctest --test-dir build`
- A minimal `test_helper.hpp` provides assert macros:

```cpp
// test_helper.hpp
#include <cstdlib>
#include <iostream>
#include <string>

#define ASSERT_TRUE(expr) \
    if (!(expr)) { \
        std::cerr << "FAIL: " #expr " at " << __FILE__ << ":" << __LINE__ << "\n"; \
        return 1; \
    }

#define ASSERT_EQ(a, b) \
    if ((a) != (b)) { \
        std::cerr << "FAIL: " #a " != " #b " at " << __FILE__ << ":" << __LINE__ << "\n"; \
        return 1; \
    }

#define ASSERT_CONTAINS(haystack, needle) \
    if (std::string(haystack).find(needle) == std::string::npos) { \
        std::cerr << "FAIL: \"" << needle << "\" not found at " << __FILE__ << ":" << __LINE__ << "\n"; \
        return 1; \
    }
```

Each test file is a standalone `int main()` that returns 0 on success. Registered in CMake:

```cmake
add_executable(test_bold tests/test_bold.cpp)
target_link_libraries(test_bold PRIVATE markdown-ui)
add_test(NAME test_bold COMMAND test_bold)
```

### Testing approach

Tests are organized into two layers:

1. **Parser tests** (`test_parser.cpp` + per-feature files): Parse Markdown string → assert AST structure. Validates that the parser produces the expected tree of `ASTNode` values.

2. **DOM builder tests** (`test_dom_builder.cpp` + per-feature files): Construct an AST manually → call `DomBuilder::build()` → render to an `ftxui::Screen` → inspect screen content for expected text and style markers.

### How FTXUI DOM testing works

```cpp
// Render an Element to a Screen and inspect the output
ftxui::Element element = dom_builder.build(ast);
ftxui::Screen screen(80, 24);
ftxui::Render(screen, element);
std::string output = screen.ToString();

// Assert content presence
ASSERT_CONTAINS(output, "important");
```

Style verification (bold, italic, etc.) can be checked by inspecting individual `Pixel` values on the `Screen` object, which carry style metadata.

### What is NOT tested

- Demo application UI layout (manual verification only)
- FTXUI's own rendering correctness (trusted dependency)
- Performance benchmarks (not required for this scope)

---

## 13. Error Handling

### Parser errors

- Invalid or malformed Markdown must **never** crash the application
- The parser should return a best-effort AST (cmark-gfm is lenient by default)
- If parsing fails entirely, return a `Document` node containing a single `Paragraph` with the raw input text

### DOM builder errors

- Unknown `NodeType` values should be rendered as plain text (defensive)
- Null/empty children should be handled gracefully

### Editor errors

- The editor does not validate content — it accepts any text

---

## 14. Performance Expectations

- Target document size: up to ~5,000 lines
- Full re-parse + re-render on each edit is acceptable for this size
- No incremental parsing required
- Debounce (50ms) in the demo prevents excessive re-renders during fast typing
- Single-threaded only — no async rendering, no background parsing

---

## 15. CI Configuration

### GitHub Actions

- **Matrix**: Windows (MSVC 2022), macOS (latest Xcode/Clang), Ubuntu (GCC 13)
- **Steps**: configure → build → test
- Triggered on push to `main` and on pull requests

```yaml
# .github/workflows/ci.yml (sketch)
strategy:
  matrix:
    os: [ubuntu-latest, macos-latest, windows-latest]
steps:
  - uses: actions/checkout@v4
  - name: Configure
    run: cmake -S . -B build -DCMAKE_CXX_STANDARD=23
  - name: Build
    run: cmake --build build
  - name: Test
    run: ctest --test-dir build --output-on-failure
```

---

## 16. Final Acceptance Criteria

- Builds on Windows (MSVC), macOS (Clang), Linux (GCC) via CI
- Uses C++23 with documented fallback strategy
- Editor and Viewer are clearly separated components
- cmark-gfm is fully abstracted behind `MarkdownParser` interface
- AST type system (`ASTNode`, `NodeType`) is fully defined and documented
- `DomBuilder` converts AST to FTXUI DOM with documented mapping
- Tests exist for every phase, including edge cases and Unicode
- Editor has lexical highlighting with its own tests
- Demo application runs with side-by-side layout and debounced updates
- Invalid Markdown never crashes the application
- Unicode renders correctly in both editor and viewer
