# Design Decisions

This document explains the reasoning behind key architectural and scope decisions in MarkdownFTXUI. Understanding these helps contributors and consumers predict behavior and avoid pitfalls.

---

## 1. Why No Tables

Tables are explicitly out of scope. The original project plan lists them under "Out of Scope" alongside images, HTML blocks, and footnotes.

**Rationale:**

Terminal table rendering is deceptively complex. A correct implementation requires:
- **Column width calculation** -- measuring the widest cell per column, handling CJK characters that occupy 2 terminal columns, and deciding minimum/maximum widths
- **Word wrapping within cells** -- cells with long text need to wrap, which changes row heights and requires re-measuring
- **Border drawing** -- consistent Unicode box-drawing characters (`+`, `-`, `|` or `┌`, `─`, `┐`) aligned across variable-width columns
- **Terminal width sensitivity** -- tables that fit in an 80-column terminal overflow in a 60-column one, requiring either horizontal scrolling or column truncation

All of this is fragile across terminal sizes and font configurations. The library targets **simple Markdown for planning documents and emails** -- use cases where bullet lists and headings are sufficient. Full GitHub Flavored Markdown table support would roughly double the DomBuilder complexity for a feature most users don't need in a terminal context.

If table support is needed, a dedicated table-rendering library should be used alongside MarkdownFTXUI.

## 2. Why No GFM Extensions

The library uses cmark-gfm as its parser but does **not** enable any GFM extensions. Specifically:
- **Strikethrough** (`~~text~~`) -- not supported
- **Task lists** (`- [x] item`) -- rendered as regular list items
- **Autolinks** (`https://...` without `[]()`syntax) -- not linked
- **Tables** -- see above

**Rationale:**

Scope control. Each extension adds AST node types, rendering rules, and edge cases. The library links cmark-gfm statically but never calls `cmark_gfm_core_extensions_ensure_registered()` or attaches any extension to the parser. This keeps the rendering surface small and predictable.

The `MarkdownParser` interface makes it possible to add extension support in the future without changing consumer code -- a new parser implementation could enable extensions and produce the same AST tree with additional node types.

## 3. Parser Abstraction

The `MarkdownParser` abstract class hides cmark-gfm completely from library consumers.

**Rationale:**

cmark-gfm is a C library with several integration challenges:
- On MSVC, static targets are named `libcmark-gfm_static` (not `libcmark-gfm`)
- Static linking requires the `CMARK_GFM_STATIC_DEFINE` preprocessor definition
- Its CMakeLists uses an older `cmake_minimum_required`, requiring `CMAKE_POLICY_VERSION_MINIMUM=3.5`
- Include directories for static targets are not exported and must be added manually: `${cmark-gfm_SOURCE_DIR}/src` and `${cmark-gfm_BINARY_DIR}/src`

None of these details leak to consumers. They link against `markdown-ui` and see only the `MarkdownParser` interface and `make_cmark_parser()` factory function. The parser could be swapped for a different Markdown library (or a hand-written parser) without changing any public API.

## 4. Lexical vs Semantic Highlighting

The Editor and Viewer use fundamentally different highlighting strategies:

| | Editor | Viewer |
|---|--------|--------|
| **Method** | Lexical (character scanning) | Semantic (AST-driven) |
| **Input** | Raw text | Parsed AST |
| **Cost** | O(n) scan per frame | Parse + build (skipped when cached) |
| **Accuracy** | Approximate (colors markers, not meaning) | Exact (matches parsed structure) |

**Rationale:**

The editor text changes on every keystroke. Running a full cmark-gfm parse + DomBuilder build on every character would be expensive and unnecessary. The lexical highlighter simply scans for known marker characters (`#`, `*`, `` ` ``, `>`, `-`, `[`, `]`, etc.) and applies a dim/colored decorator. It does not understand nesting, so `**bold *italic***` will color all asterisks the same way regardless of their semantic role. This is acceptable for an editor -- users are looking at their own raw Markdown syntax.

The viewer, by contrast, parses into a full AST and builds a styled DOM tree. This gives precise rendering (bold text is actually bold, links are underlined) but is too expensive to run on every keystroke. The viewer's caching system (see below) ensures it only re-parses when the content actually changes.

## 5. Generation-Counter Caching

The Viewer tracks state changes using integer generation counters rather than content hashing or dirty flags.

Four counters are maintained:
- `_content_gen` -- incremented by `set_content()`
- `_parsed_gen` -- set to `_content_gen` after parsing completes
- `_built_gen` -- set to `_parsed_gen` after DOM building completes
- `_theme_gen` -- incremented by `set_theme()` when the theme name changes
- `_built_theme_gen` -- set to `_theme_gen` after DOM building completes

**Rationale:**

The renderer lambda is called every frame (60+ times per second during interaction). Comparing counters is O(1). Content hashing would be O(n) on the full document text, which defeats the purpose of caching for large documents.

The counter approach also naturally handles the cascade: if content changes, both parse and build are triggered. If only the theme changes, only build is triggered. If only the scroll ratio changes, neither is triggered.

An earlier version of `set_theme()` incremented `_theme_gen` unconditionally, causing a full DOM rebuild on every frame even when the theme hadn't changed. This was fixed by comparing `_theme.name != theme.name` before incrementing.

## 6. DirectScrollFrame vs yframe

The library uses two different scroll strategies depending on context:

**FTXUI's yframe**: Scrolls to center the element that has `ftxui::focus`. Used when a link is Tab-focused, so the focused link is always visible on screen.

**DirectScrollFrame**: Scrolls by a direct ratio (0.0 = top, 1.0 = bottom). Used for arrow-key scrolling, where the user controls position linearly.

```
  if (focused_link >= 0)
      element | ftxui::yframe       // Center on focused link
  else
      direct_scroll(element, ratio) // Linear ratio-based offset
```

**Rationale:**

`yframe` is the right choice for focus-driven scrolling -- it automatically keeps the focused element visible. But it provides no way to scroll to an arbitrary position by ratio. When the user presses Arrow Down, they expect smooth incremental scrolling, not centering on some element.

DirectScrollFrame was created to fill this gap. It overrides `SetBox()` to compute `offset = ratio * scrollable_height` and shifts the child element accordingly. It also sets the stencil to clip content outside the viewport.

## 7. FTXUI Decorator Composition on MSVC

A subtle compatibility issue affects MSVC builds:

```cpp
// FAILS on MSVC:
auto deco = ftxui::bold | ftxui::underlined;

// WORKS on all platforms:
auto deco = ftxui::Decorator(ftxui::bold) | ftxui::underlined;
```

**Explanation:**

`ftxui::bold` and `ftxui::underlined` are raw function pointers (`Element(*)(Element)`), not `std::function<Element(Element)>`. The `operator|` for decorator composition is defined for `Decorator` (which is `std::function<Element(Element)>`), not for function pointers. On GCC/Clang, implicit conversions make this work. On MSVC, the overload resolution fails.

The fix is to explicitly wrap the first operand in `Decorator(...)`. Decorators returned by `ftxui::color()` are already `std::function` and compose without wrapping.

All three built-in themes use this pattern in their initializers.

## 8. vector<Box> and reflect() Pitfall

A crash pattern was discovered during DomBuilder development:

```cpp
std::vector<ftxui::Box> boxes;
// BAD: push_back can reallocate the vector, invalidating references
for (auto& link : links) {
    boxes.push_back({});
    elements.push_back(element | ftxui::reflect(boxes.back()));
    // If push_back triggers reallocation, reflect() holds a dangling Box&
}
```

`ftxui::reflect(box)` stores a **reference** to the `Box`, not a copy. If the vector reallocates (because it runs out of capacity), all previously stored references become dangling. On MSVC, this manifests as `_Orphan_all_unlocked_v3` during `Node::~Node()` destruction.

**Fix:** Pre-allocate the vector before creating any `reflect()` references:

```cpp
boxes.resize(link_count);  // Allocate all at once
for (int i = 0; i < link_count; ++i) {
    elements.push_back(element | ftxui::reflect(boxes[i]));
}
```

This is documented here because anyone extending DomBuilder with new reflectable elements must follow the same pattern.

## 9. Tab Focus Integration

The Viewer supports integration with parent component Tab cycling via two callbacks:

- **`on_tab_exit(cb)`** -- called when Tab would wrap past the last link (`cb(+1)`) or Shift+Tab before the first (`cb(-1)`). The viewer clears its focus and deactivates.
- **`enter_focus(direction)`** -- the parent calls this to give the viewer link focus. Returns `true` if there are links to focus, `false` otherwise.

If `on_tab_exit` is not set, Tab wraps through links as before (backward compatible).

**Rationale:**

An earlier design embedded parent items directly into the viewer's Tab ring via an `ExternalFocusable` API (`add_focusable("From", "alice@example.com")`). This was limiting because the "external" items were just data strings, not real FTXUI components. The parent had to poll the viewer's focus state to render them.

The callback approach inverts control: the parent owns its own focusable elements and manages them with real component logic. The viewer is just one element in the parent's focus chain. This is more composable -- any parent with any number of components can integrate with the viewer without the viewer knowing anything about the parent's structure.

The `cycle_focus()` function handles the starting case specially: when `_focus_index == -1` (nothing focused), Tab goes to index 0 and Shift+Tab goes to the last item. When `on_tab_exit` is set and Tab would advance past the bounds, the viewer exits instead of wrapping.

## 10. Configurable Key Bindings

The Viewer's key bindings (activate, deactivate, next, prev) are configurable via the `ViewerKeys` struct rather than hardcoded to Enter/Escape/Tab/Shift+Tab.

```cpp
markdown::ViewerKeys keys;
keys.activate   = ftxui::Event::Return;      // default
keys.deactivate = ftxui::Event::Escape;       // default
keys.next       = ftxui::Event::Tab;          // default
keys.prev       = ftxui::Event::TabReverse;   // default
viewer->set_keys(keys);
```

**Rationale:**

Different host applications have different key conventions. An email client might use `j`/`k` for navigation instead of Tab. A Vim-style application might use `i`/`Esc` for mode switching. Hardcoded keys force consumers to wrap the viewer in a key-remapping layer, which is fragile and error-prone.

The `keys()` accessor serves a second purpose: parent components that intercept events for Tab integration (`on_tab_exit`/`enter_focus`) should reference `viewer->keys().next` and `viewer->keys().deactivate` instead of `ftxui::Event::Tab` and `ftxui::Event::Escape`. This way, if the consumer customizes the keys, parent event handlers automatically stay in sync.

## 11. Theme System Design

Themes are `ftxui::Decorator` values (which are `std::function<Element(Element)>`), not enum-based style selectors.

**Rationale:**

FTXUI's decorator system is composable -- `bold | underlined | color(Cyan)` combines three transformations into one. By storing decorators directly in the Theme struct, theme authors have full access to any FTXUI attribute combination without the library defining a fixed set of "styles."

The three built-in themes demonstrate different approaches:
- **Default**: minimal (bold headings, inverted code, dim blockquotes)
- **Contrast**: high visibility (white + bold everywhere)
- **Colorful**: one color per element type (cyan headings, magenta links, green code)

Custom themes can use any FTXUI decorator, including `bgcolor()`, `dim`, or custom decorators.

## 12. Features Beyond the Original Plan

The original project plan specified a focused scope: headings, bold, italic, links, unordered lists, inline code, blockquotes, and an editor with lexical highlighting. During development, several additional features were added:

- **Ordered lists** (1. 2. 3.) with `list_start` support
- **Thematic breaks** (---) rendered as `ftxui::separator()`
- **Images** (![alt](url)) rendered as `[img: alt]` placeholder text
- **Code blocks** (```) rendered in bordered boxes
- **Nested list indentation** (sub-items indented under parent items)
- **H1 underline** (heading level 1 gets underlined in addition to bold)
- **Editor line numbers** gutter with configurable display
- **Editor status bar** (Ln/Col tracking)
- **Three built-in themes** (originally only one was planned)
- **Tab focus integration** via `on_tab_exit`/`enter_focus` callbacks
- **Configurable key bindings** via `ViewerKeys` struct
- **Embed mode** for combining viewer content with other UI elements
- **CMake install targets** with `find_package` support

These additions were motivated by the demo application's needs. The email screen in particular drove the tab focus integration and embed mode features.
