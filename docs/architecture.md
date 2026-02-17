# Architecture

## High-Level Overview

MarkdownFTXUI is a library that provides two main components -- an **Editor** and a **Viewer** -- built on top of FTXUI's terminal rendering engine. The Viewer pipeline transforms Markdown text through three stages: parsing, AST construction, and DOM building.

```
 Application Code
 ────────────────────────────────────────────────────────
        │                        │
        ▼                        ▼
 ┌─────────────┐         ┌──────────────────────────────┐
 │   Editor     │         │          Viewer              │
 │              │ content │                              │
 │  highlight   │────────►│  ┌────────┐   ┌──────────┐  │
 │  _markdown   │         │  │ Parser │──►│DomBuilder│  │
 │  _syntax()   │         │  │ (cmark │   │          │  │
 │              │         │  │  -gfm) │   │          │  │
 │  Lexical     │         │  └────────┘   └──────────┘  │
 │  highlighting│         │   MarkdownAST    Element     │
 └─────────────┘         └──────────────────────────────┘
        │                        │
        ▼                        ▼
 ┌──────────────────────────────────────────────────────┐
 │           FTXUI  (screen, dom, component)            │
 └──────────────────────────────────────────────────────┘
        │
        ▼
    Terminal Output
```

## Data Flow

The Viewer's rendering pipeline has three distinct stages:

```
  Markdown string
       │
       ▼
  MarkdownParser::parse()        ← cmark-gfm (hidden)
       │
       ▼
  MarkdownAST (tree of ASTNode)
       │
       ▼
  DomBuilder::build()            ← applies Theme decorators
       │
       ▼
  ftxui::Element                 ← FTXUI virtual DOM tree
       │
       ▼
  ftxui::Screen::Render()        ← terminal output
```

The Editor takes a different path. It does **not** parse Markdown into an AST. Instead, it performs lexical (character-level) highlighting directly on the raw text, coloring syntax markers like `#`, `*`, `` ` ``, and `>` without understanding document structure. This is intentional -- the editor text changes every keystroke, and a full parse-render cycle would be too expensive.

## Component Responsibilities

### MarkdownParser (`parser.hpp`)

Abstract interface for Markdown parsing. The single concrete implementation (`parser_cmark.cpp`) wraps cmark-gfm and is created via the `make_cmark_parser()` factory function. This design completely hides cmark-gfm's types and headers from library consumers.

```
  MarkdownParser (abstract)
       │
       └── CmarkParser (internal, parser_cmark.cpp)
               │
               └── cmark-gfm C library
```

The parser provides two overloads:
- `bool parse(string_view, MarkdownAST&)` -- output parameter, returns success
- `MarkdownAST parse(string_view)` -- convenience, returns AST directly

### ASTNode / MarkdownAST (`ast.hpp`)

A recursive tree structure representing parsed Markdown. Each node has a `NodeType` enum and optional fields (`text`, `url`, `level`, `list_start`). The tree mirrors Markdown's block/inline structure:

```
  Document
  ├── Heading (level=1)
  │   └── Text ("Hello")
  ├── Paragraph
  │   ├── Text ("This is ")
  │   ├── Strong
  │   │   └── Text ("bold")
  │   └── Text (" text")
  └── BulletList
      ├── ListItem
      │   └── Paragraph
      │       └── Text ("item one")
      └── ListItem
          └── Paragraph
              └── Text ("item two")
```

Supported node types (17 total): Document, Heading, Paragraph, Text, Emphasis, Strong, Link, ListItem, BulletList, OrderedList, CodeInline, CodeBlock, BlockQuote, SoftBreak, HardBreak, ThematicBreak, Image.

### DomBuilder (`dom_builder.hpp`)

Transforms a `MarkdownAST` into an `ftxui::Element` tree. The `build()` method walks the AST recursively, dispatching each node type to a named helper function:

```
  build()
    └── build_node()
          ├── build_document()      → vbox of children
          ├── build_heading()       → text with heading1/2/3 decorator
          ├── build_link()          → underlined text with reflect() boxes
          ├── build_bullet_list()   → "  " indent + "* " prefix per item
          ├── build_ordered_list()  → "  " indent + "N. " prefix per item
          ├── build_blockquote()    → "  | " prefix + blockquote decorator
          ├── build_code_block()    → bordered box with code_block decorator
          └── build_image()         → "[img: alt]" placeholder text
```

The builder also tracks **link targets** -- each link's bounding `Box` on screen (via `ftxui::reflect()`) and its URL. These are used by the Viewer for mouse click detection and keyboard navigation.

### Viewer (`viewer.hpp`, `viewer.cpp`)

The highest-level component. It owns a parser and DomBuilder internally, exposes an `ftxui::Component` for embedding in FTXUI layouts, and handles:

- **Content management**: `set_content()` updates the Markdown text
- **Scroll control**: `set_scroll(ratio)` for linear offset, yframe for link focus
- **Link navigation**: configurable next/prev keys cycle through links, activate key presses
- **Tab integration**: `on_tab_exit`/`enter_focus` for parent↔viewer focus cycling
- **Configurable keys**: `set_keys(ViewerKeys)` overrides activate, deactivate, next, prev
- **Theming**: `set_theme()` changes visual style

Internally, the Viewer wraps its renderer in a `ViewerWrap` component that handles all keyboard/mouse events:

```
  ViewerWrap (keys are configurable via ViewerKeys)
  ├── keys.activate → activate (enable link navigation) [default: Enter]
  ├── keys.deactivate → deactivate                      [default: Esc]
  ├── keys.next/prev → cycle links (only when active)   [default: Tab/Shift+Tab]
  │   ├── on_tab_exit set → exit instead of wrapping
  │   └── no callback → wrap around (default)
  ├── keys.activate (when link focused) → press link
  └── Arrows → scroll
```

The parent can also activate the viewer via `enter_focus(direction)`, which sets the viewer active and focuses the first or last link. When Tab goes past the bounds and `on_tab_exit` is set, the viewer clears focus, deactivates, and calls the callback so the parent can move focus to its own elements.

### Editor (`editor.hpp`, `editor.cpp`)

Wraps FTXUI's `Input` component with lexical syntax highlighting. The editor uses `InputOption::transform` to replace the default rendering with `highlight_markdown_with_cursor()`, which colors Markdown syntax markers while preserving cursor position.

Features:
- Line numbers gutter (configurable)
- Status bar cursor position (line/col)
- Mouse click support (maps screen coordinates to byte offsets)
- Batched cursor rendering (inverted character, no terminal cursor blink)

### DirectScrollFrame (`scroll_frame.hpp`)

A custom FTXUI `Node` that implements ratio-based vertical scrolling. Unlike FTXUI's built-in `yframe` (which centers the focused element), DirectScrollFrame computes:

```
  scroll_offset = ratio * scrollable_height
```

This gives predictable linear scrolling controlled by arrow keys or a scroll ratio (0.0 = top, 1.0 = bottom). The Viewer uses both approaches:

```
  Scroll strategy selection:
  ├── Link focused → yframe (centers the focused link)
  └── No link focused → DirectScrollFrame (ratio-based offset)
```

### Theme (`theme.hpp`)

A struct of `ftxui::Decorator` values that style different Markdown elements. Each decorator is a `std::function<Element(Element)>` that can compose FTXUI attributes like `bold`, `underlined`, `color()`, and `inverted`.

Three built-in themes:
- **Default** -- minimal styling (bold headings, yellow syntax markers, inverted code)
- **Contrast** -- high contrast (white + bold syntax, underlined headings)
- **Colorful** -- color-coded (cyan H1, green H2, yellow H3, magenta links)

### Text Utilities (`text_utils.hpp`)

Header-only UTF-8 utilities shared between Editor and DomBuilder:
- Character counting and byte offset conversion
- CJK/fullwidth character width detection (1 vs 2 terminal columns)
- Visual column to byte offset mapping
- Line splitting
- Gutter width calculation for line numbers

## Caching Strategy

The Viewer uses **generation counters** to skip redundant work. Four counters track different aspects of state:

```
  _content_gen    ← incremented on set_content()
  _parsed_gen     ← set to _content_gen after parsing
  _built_gen      ← set to _parsed_gen after building
  _theme_gen      ← incremented on set_theme() (only if name changed)
  _built_theme_gen← set to _theme_gen after building
```

On each render frame:
1. If `_content_gen != _parsed_gen` → re-parse (call `MarkdownParser::parse()`)
2. If `_parsed_gen != _built_gen` OR focused link changed OR `_theme_gen != _built_theme_gen` → re-build (call `DomBuilder::build()`)
3. Otherwise → reuse `_cached_element`

This means:
- Typing in the editor (which calls `set_content()` each frame) triggers a re-parse + re-build
- Scrolling with arrow keys triggers neither (only the scroll ratio changes)
- Changing themes triggers a re-build but not a re-parse
- Tab-cycling links triggers a re-build (focused link changes highlight)

Why counters instead of hashing: Counters are O(1) to compare and increment. Content hashing would be O(n) on every frame, which defeats the purpose for large documents.

## Module Dependency Graph

```
  Consumer code
       │
       ▼
  ┌─────────────────────────────────────┐
  │            markdown-ui               │
  │                                      │
  │  viewer.hpp ──► parser.hpp           │  PUBLIC headers
  │       │              │               │  (consumer includes these)
  │       ▼              ▼               │
  │  dom_builder.hpp  ast.hpp            │
  │       │                              │
  │       ▼                              │
  │  theme.hpp                           │
  │                                      │
  │  editor.hpp ──► highlight.hpp        │
  │                     │                │
  │                     ▼                │
  │               text_utils.hpp         │
  │                                      │
  │  scroll_frame.hpp (standalone)       │
  ├──────────────────────────────────────┤
  │  parser_cmark.cpp ──► cmark-gfm     │  PRIVATE (hidden)
  └──────────────────────────────────────┘
       │
       ▼
  ftxui::screen, ftxui::dom, ftxui::component  (PUBLIC dependency)
```

Consumers link against `markdown-ui` and get FTXUI as a transitive dependency. cmark-gfm is completely invisible -- no headers, no link flags, no defines.
