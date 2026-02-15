# Demo Application

The demo application showcases all three usage patterns of MarkdownFTXUI: a side-by-side editor with live preview, a standalone scrollable viewer, and an email viewer with external focusable headers.

## Building and Running

```bash
cmake -B build
cmake --build build --config Release

# Windows (MSVC)
build\demo\Release\markdown-demo.exe

# Linux / macOS
./build/demo/markdown-demo
```

## Main Menu (Screen 0)

The demo starts with a centered menu showing three options:

```
         MarkdownFTXUI Demo

  1. Editor + Viewer
     Side-by-side editing with live preview

  2. Viewer with Scroll
     Scrollable markdown viewer with scrollbar

  3. Email Viewer
     Simulated email with combined scroll

  Theme: [Default] Contrast  Colorful

  Enter to select
```

- **Arrow Up/Down** to navigate menu items
- **Enter** to select a screen
- **Tab** on the theme toggle to cycle themes
- **Esc** to quit the application

The theme selection is shared across all screens.

---

## Screen 1: Editor + Viewer

A split-pane layout with an editable Markdown source on the left and a live-rendered preview on the right.

```
  Theme: Default  Contrast  Colorful
 ╔═══════════ Editor ══════════╗┌─────────── Viewer ───────────┐
 ║ # Hello Markdown             │ Hello Markdown                │
 ║                              │ ════════════════              │
 ║ > Focus on **important** ..  │ │ Focus on important tasks    │
 ║                              │                               │
 ║ ## Section One               │ Section One                   │
 ║                              │                               │
 ║ This is **bold**, *italic*   │ This is bold, italic, and     │
 ║                              │ bold italic.                  │
 ║ - Write *code*               │                               │
 ║ - Review `tests` carefully   │  * Write code                 │
 ║ - Read [docs](https://...)   │  * Review tests carefully     │
 ╚══════════════════════════════╝  * Read docs                  │
                                 └──────────────────────────────┘
  Enter:select  Esc:back
```

### Features

- **Live preview**: The viewer updates instantly as you type in the editor
- **Cursor-synced scroll**: The viewer scroll position tracks the editor cursor line. If the cursor is at line 50 of 100, the viewer scrolls to 50%.
- **Border indicators**:
  - Dim border: component is not focused
  - Single white border: component is focused but not in edit mode
  - Double white border: component is focused and in edit mode (accepting input)
- **Theme toggle**: The top bar shows a Toggle component for switching themes

### Key Code Pattern

```cpp
auto editor = std::make_shared<markdown::Editor>();
auto viewer = std::make_shared<markdown::Viewer>(
    markdown::make_cmark_parser());

// In the Renderer lambda:
viewer->set_content(editor->content());   // Live sync

float ratio = static_cast<float>(editor->cursor_line() - 1) /
              static_cast<float>(editor->total_lines() - 1);
viewer->set_scroll(ratio);                // Cursor-synced scroll
```

### Navigation

| Key | Action |
|-----|--------|
| Tab / Shift+Tab | Move focus between theme toggle, editor, viewer |
| Enter | Activate focused component (enter edit mode) |
| Esc | Deactivate component; if already inactive, return to menu |
| Arrow keys | Edit text (in editor) or scroll (in viewer, when active) |
| Page Up/Down | Move cursor by 20 lines (editor) or scroll viewport (viewer) |
| Mouse Wheel | Move cursor by 3 lines (editor) or scroll 5% (viewer) |

---

## Screen 2: Standalone Viewer

A full-window Markdown viewer with a scrollbar, link navigation, and theme cycling.

```
  Theme: Default
 ┌────────────── Markdown Viewer ──────────────┐
 │ Markdown Viewer Demo                        ▓│
 │ ════════════════════                        ▓│
 │                                             ░│
 │ This screen demonstrates a standalone       ░│
 │ viewer with scrollbar and link navigation.  ░│
 │                                             ░│
 │ Text Formatting                             ░│
 │                                             ░│
 │ You can write in bold, italic, bold italic, ░│
 │ and inline code.                            ░│
 └─────────────────────────────────────────────┘
  https://example.com/bullet    Scroll:Arrows/PgUp/PgDn/Home/End  ...
```

### Features

- **Scrollbar**: Vertical scroll indicator on the right edge
- **Link navigation**: Tab cycles through links (highlighted with inverted style when focused), Enter activates
- **Link URL display**: The status bar at the bottom shows the URL of the focused/clicked link
- **Keyboard scrolling**: Up/Down arrows (5%), PageUp/PageDown (one viewport), Home/End (jump to top/bottom)
- **Mouse wheel**: Scrolls content by 5% per tick
- **Theme cycling**: Left/Right arrows switch between the three themes
- **Auto-activation**: Tab automatically activates the viewer for link navigation

### Key Code Pattern

```cpp
auto viewer = std::make_shared<markdown::Viewer>(
    markdown::make_cmark_parser());
viewer->set_content(long_document);
viewer->show_scrollbar(true);

viewer->on_link_click(
    [&](std::string const& url, markdown::LinkEvent ev) {
        status_text = url;
    });
```

### Navigation

| Key | Action |
|-----|--------|
| Arrow Up/Down | Scroll content (5% per press) |
| Page Up/Down | Scroll by one viewport height |
| Home / End | Jump to top / bottom |
| Mouse Wheel | Scroll content (5% per tick) |
| Arrow Left/Right | Cycle themes |
| Tab / Shift+Tab | Cycle through links |
| Enter | Activate viewer (if inactive) or press focused link |
| Esc | Deactivate viewer; if inactive, return to menu |

---

## Screen 3: Email Viewer

A simulated email view combining header fields and a Markdown body in a single scrollable frame.

```
  Theme: Default
 ┌──────────────────────────────────────────────┐
 │[From: Alice <alice@example.com>]             ▓│
 │ To: team@example.com                         ▓│
 │ Subject: Sprint Review Notes - Week 42       ░│
 │ Date: Fri, 7 Feb 2026 15:30:00 +0200        ░│
 │──────────────────────────────────────────────░│
 │ Sprint Review Notes                          ░│
 │ ════════════════════                         ░│
 │                                              ░│
 │ Hi team, here are the notes from today's     ░│
 │ sprint review. Please review and add any     ░│
 │ comments by end of day.                      ░│
 └──────────────────────────────────────────────┘
  Alice <alice@example.com>    Tab:cycle  Scroll:Arrows/PgUp/PgDn/Home/End  ...
```

### Features

- **External focusable headers**: Four header fields (From, To, Subject, Date) are registered as external focusable items. They appear before links in the Tab ring.
- **Bracket highlighting**: The currently focused header is wrapped in `[brackets]`, others have padding spaces for alignment.
- **Embed mode**: The viewer operates in embed mode (`set_embed(true)`), meaning it does not create its own scroll frame. Instead, headers and body are combined in a single `vbox` wrapped in one `direct_scroll`/`yframe`.
- **Unified scroll**: A single scrollbar covers both headers and body content.
- **Link support**: Links in the Markdown body are part of the same Tab ring as headers.

### Key Code Pattern

```cpp
auto viewer = std::make_shared<markdown::Viewer>(
    markdown::make_cmark_parser());
viewer->set_content(email_body);
viewer->set_embed(true);  // Skip internal framing

// Register email headers as external focusables
viewer->add_focusable("From", "Alice <alice@example.com>");
viewer->add_focusable("To", "team@example.com");
viewer->add_focusable("Subject", "Sprint Review Notes - Week 42");
viewer->add_focusable("Date", "Fri, 7 Feb 2026 15:30:00 +0200");

// In the Renderer:
for (int i = 0; i < viewer->externals().size(); ++i) {
    if (viewer->is_external_focused(i)) {
        // Render with [brackets]
    }
}

// Combine headers + viewer body in one scrollable frame
auto combined = ftxui::vbox({headers..., separator(), viewer_comp->Render()})
    | ftxui::vscroll_indicator;
if (!viewer->is_link_focused())
    combined = markdown::direct_scroll(combined, viewer->scroll());
else
    combined = combined | ftxui::yframe;
```

### Navigation

| Key | Action |
|-----|--------|
| Tab / Shift+Tab | Cycle through headers then links |
| Arrow Up/Down | Scroll content (5% per press) |
| Page Up/Down | Scroll by one viewport height |
| Home / End | Jump to top / bottom |
| Mouse Wheel | Scroll content (5% per tick) |
| Arrow Left/Right | Cycle themes |
| Enter | Press focused item (shows value in status bar) |
| Esc | Return to menu |

---

## Keyboard Reference (All Screens)

| Key | Menu | Editor+Viewer | Standalone Viewer | Email Viewer |
|-----|------|---------------|-------------------|--------------|
| Enter | Select screen | Activate component | Activate / press link | Press focused item |
| Esc | Quit app | Deactivate / back to menu | Deactivate / back to menu | Back to menu |
| Tab | Theme toggle | Cycle components | Cycle links | Cycle headers + links |
| Shift+Tab | Theme toggle | Cycle components (reverse) | Cycle links (reverse) | Cycle (reverse) |
| Arrow Up | Menu navigate | Edit / Scroll | Scroll (5%) | Scroll (5%) |
| Arrow Down | Menu navigate | Edit / Scroll | Scroll (5%) | Scroll (5%) |
| Arrow Left | -- | Edit | Theme cycle | Theme cycle |
| Arrow Right | -- | Edit | Theme cycle | Theme cycle |
| Page Up | -- | Cursor -20 lines | Scroll viewport | Scroll viewport |
| Page Down | -- | Cursor +20 lines | Scroll viewport | Scroll viewport |
| Home | -- | Line start (FTXUI) | Jump to top | Jump to top |
| End | -- | Line end (FTXUI) | Jump to bottom | Jump to bottom |
| Mouse Wheel | -- | Cursor ±3 lines | Scroll (5%) | Scroll (5%) |

## Demo Source Files

| File | Purpose |
|------|---------|
| `demo/main.cpp` | Entry point, main menu, Container::Tab routing |
| `demo/screens.hpp` | Function declarations for screen factories |
| `demo/common.hpp` | Shared utilities (ZeroMinWidth, get_theme, theme_bar) |
| `demo/screen_editor.cpp` | Screen 1: Editor + Viewer side-by-side |
| `demo/screen_viewer.cpp` | Screen 2: Standalone viewer with scrollbar |
| `demo/screen_email.cpp` | Screen 3: Email viewer with external focusables |
