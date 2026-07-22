<p align="center">
  <h1 align="center">🎮 imgui-mcp</h1>
  <p align="center">
    <strong>The MCP server that lets AI agents <em>see</em>, <em>build</em>, and <em>iterate</em> on Dear ImGui game UIs in real-time.</strong>
  </p>
  <p align="center">
    <a href="https://github.com/SaloQT/imgui-mcp/releases"><img src="https://img.shields.io/github/v/release/SaloQT/imgui-mcp?style=flat&colorA=222222&colorB=58A6FF" alt="Release"></a>
    <a href="https://github.com/ocornut/imgui/releases/tag/v1.92.8"><img src="https://img.shields.io/badge/Dear_ImGui-v1.92.8-ff69b4?style=flat&colorA=222222" alt="ImGui"></a>
    <a href="https://modelcontextprotocol.io"><img src="https://img.shields.io/badge/MCP-2025--06--18-orange?style=flat&colorA=222222" alt="MCP"></a>
    <img src="https://img.shields.io/badge/74_tools-8A2BE2?style=flat&colorA=222222" alt="Tools">
    <img src="https://img.shields.io/badge/102_widgets-DC143C?style=flat&colorA=222222" alt="Widgets">
    <img src="https://img.shields.io/badge/13_IDE%2FCLI_tools-3FB950?style=flat&colorA=222222" alt="Supported Tools">
    <a href="https://github.com/SaloQT/imgui-mcp/blob/main/LICENSE"><img src="https://img.shields.io/badge/License-MIT-green?style=flat&colorA=222222" alt="License"></a>
  </p>
  <p align="center">
    <a href="#quick-start">Quick Start</a> ·
    <a href="#all-74-mcp-tools">All Tools</a> ·
    <a href="#all-102-widget-types">All Widgets</a> ·
    <a href="#game-ui-patterns">Game Patterns</a> ·
    <a href="#supported-idecli-tools">Supported Tools</a> ·
    <a href="https://github.com/SaloQT/imgui-mcp/releases">Releases</a>
  </p>
</p>

---

## The Problem

AI coding agents can generate UI code — but they **can't see the result**. They write ImGui calls blind, hope the layout works, and never iterate visually. Game UI development needs a feedback loop: *place → preview → adjust → repeat*.

## The Solution

**imgui-mcp** is a [Model Context Protocol](https://modelcontextprotocol.io) server that gives any AI agent a live, interactive [Dear ImGui](https://github.com/ocornut/imgui) canvas with **74 tools**, **102 widget types**, and a **visual feedback loop**:

```
┌──────────────────────────────────────────────────────────────────┐
│  "Build a game HUD with health bar, inventory, and skill bar"    │
└───────────────────────────────┬──────────────────────────────────┘
                                ▼
┌─────────────┐    MCP     ┌──────────┐   JSON    ┌─────────────┐
│  AI Agent   │◄──────────►│ server.py │◄────────►│  Live ImGui  │
│  (any LLM)  │  74 tools  │ 0 deps   │  stdin/  │  SDL2+GL     │
└─────────────┘            └──────────┘  stdout   └──────┬───────┘
       ▲                                                 │
       │              screenshot (BMP)                   │
       └─────────────────────────────────────────────────┘
                    Agent SEES what it built
```

The agent creates widgets, takes a screenshot, **sees the rendered result**, and iterates — like a developer with eyes.

---

## Features at a Glance

| | |
|---|---|
| 🖼️ **Visual Feedback** | Screenshot capture (full frame, widget-bound metadata, annotated overlays) — the agent sees its work |
| 🎮 **Game UI Patterns** | Health bars, mana bars, inventory grids, dialogue boxes, minimaps, cooldown radials, skill bars, quest trackers, character sheets, notification toasts, tooltip cards |
| 🖱️ **Input Simulation** | Click buttons, type text, hover widgets, scroll, press keys — test UI behavior programmatically |
| ✨ **Animation** | 7 easing functions, nested-widget property tweens, loop/ping-pong, and JSON round-trip persistence |
| 🎨 **Theming** | 7 presets (dark, light, classic, fantasy, sci-fi, retro, minimal) + per-color and per-variable customization |
| 📐 **Layout** | 9-point anchor system, resolution presets (720p → 4K + ultrawide + mobile), DPI scaling, safe areas |
| 🔄 **Workflow** | Undo/redo (50-deep), named snapshots, save/load/delete layout versions |
| 🎬 **Scenes** | Multi-window composition, z-order layers, conditional visibility (`widget.checked==true`) |
| 📦 **Export** | Generate production C++, Lua, or complete JSON designs with fonts and live animations |
| 🖌️ **Drawing** | Lines, rects, circles, triangles, beziers, polylines, text — direct ImDrawList access |
| 🧩 **102 Widgets** | Every Dear ImGui widget + 11 game-specific patterns |
| 🔌 **Zero Dependencies** | Python server needs no pip packages. C++ app vendors ImGui. |
| 🪟 **Native Polish** | Dark themed Windows title bar, branded taskbar/title-bar icon, and HiDPI-aware windowing |
| 🔤 **Font Stacks** | Runtime TTF/OTF loading, Unicode coverage presets, and merged icon/symbol fallback faces |

---

## Quick Start

### Option A: Pre-built release (no compiler needed)

Download from [Releases](https://github.com/SaloQT/imgui-mcp/releases):

**Linux:**
```bash
tar -xzf imgui-mcp-*-linux-x64.tar.gz
cd imgui-mcp-*-linux-x64
./setup.sh    # configures all 13 IDE/CLI tools
```

**Windows:**
```
Extract zip → double-click install.bat
```

### Option B: Build from source

```bash
git clone https://github.com/SaloQT/imgui-mcp.git
cd imgui-mcp
uv sync       # installs the pinned official MCP Python SDK
make build    # requires: cmake, g++, libsdl2-dev, libgl1-mesa-dev
./setup.sh
```

### Verify it works

```bash
uv run python demo.py   # runs a full feature demonstration
```

---

## All 74 MCP Tools

### Windows & Widgets (7)

| Tool | Description |
|------|-------------|
| `imgui_create_window` | Create a window with title, position, size, and flags (menubar, no_resize, no_move, no_collapse, no_title_bar, always_auto_resize) |
| `imgui_add_widget` | Add any of 102 widget types to a window — supports nested children for containers |
| `imgui_update_widget` | Update a widget's value, text, checked state, color, label, or enabled state at runtime |
| `imgui_remove_widget` | Remove a widget by ID |
| `imgui_remove_window` | Remove a window and all its widgets |
| `imgui_get_state` | Get the full state of every window and widget (values, clicked, changed, hovered, focused) |
| `imgui_clear_all` | Remove all windows and widgets, reset to empty canvas |

### Visual Feedback (3)

| Tool | Description |
|------|-------------|
| `imgui_screenshot` | Capture the current frame as BMP — agent can inspect the image to see what it built |
| `imgui_screenshot_widget` | Capture with a specific widget's bounding box info for targeted inspection |
| `imgui_screenshot_annotated` | Capture with widget ID overlays drawn on the image for spatial reference |

### Input Simulation (7)

| Tool | Description |
|------|-------------|
| `imgui_click_widget` | Simulate a mouse click on any widget (finds its rect center, injects click events) |
| `imgui_type_text` | Focus a widget and type text into it character by character |
| `imgui_hover_widget` | Move the virtual mouse to hover over a widget (triggers tooltips, highlights) |
| `imgui_scroll_window` | Inject mouse wheel scroll into a window |
| `imgui_press_key` | Press any key or combo: `Enter`, `Tab`, `Escape`, `Ctrl+S`, `Shift+A`, etc. |
| `imgui_set_mouse_pos` | Set the virtual mouse position in screen coordinates |
| `imgui_focus_widget` | Set keyboard focus to a specific widget for the next frame |

### Window & Widget Manipulation (8)

| Tool | Description |
|------|-------------|
| `imgui_move_window` | Move a window to new (x, y) coordinates |
| `imgui_resize_window` | Resize a window to new (w, h) dimensions |
| `imgui_move_widget` | Offset a widget's position via cursor offset |
| `imgui_get_widget_rect` | Get a widget's actual rendered bounding box in pixels |
| `imgui_get_window_rect` | Get a window's position and size |
| `imgui_bring_to_front` | Focus and raise a window to the top |
| `imgui_set_widget_size` | Override a widget's dimensions |
| `imgui_get_layout` | Get all widget bounding boxes in a window as structured data |

### Drawing — ImDrawList (1)

| Tool | Description |
|------|-------------|
| `imgui_draw` | Draw on named, composable canvas layers. Calls append by default (`mode: replace` resets one layer). Includes lines, rectangles, circles, triangles, text, polygons, Beziers, ellipses, arcs, and four-corner gradients |

### Style & Theming (7)

| Tool | Description |
|------|-------------|
| `imgui_set_style` | Apply a built-in theme: dark, light, or classic |
| `imgui_set_theme` | Apply an extended theme preset: **fantasy** (gold/amber), **scifi** (cyan/teal), **retro** (terminal green), **minimal** (clean white) |
| `imgui_set_theme_color` | Set any ImGui color by name: `WindowBg`, `Button`, `Text`, `FrameBg`, `Header`, `Border`, etc. |
| `imgui_set_style_var` | Set any style variable: `FrameRounding`, `WindowRounding`, `ItemSpacing`, `FramePadding`, `IndentSpacing`, `ScrollbarSize`, `GrabMinSize`, etc. |
| `imgui_push_style_color` | Push a temporary style color override |
| `imgui_pop_style_color` | Pop pushed style colors |
| `imgui_set_background` | Set the viewport background clear color |

### Animation (3)

| Tool | Description |
|------|-------------|
| `imgui_animate` | Tween any widget property (value, opacity, size, position) over time with easing: `linear`, `ease_in`, `ease_out`, `ease_in_out`, `bounce`, `elastic`, `back` — supports loop/ping-pong |
| `imgui_stop_animation` | Stop animations on a specific widget |
| `imgui_stop_all_animations` | Stop all active animations |

### Layout & Responsiveness (7)

| Tool | Description |
|------|-------------|
| `imgui_set_anchor` | Anchor a window to the viewport: `top_left`, `top_center`, `top_right`, `center_left`, `center`, `center_right`, `bottom_left`, `bottom_center`, `bottom_right` — with pixel offset |
| `imgui_set_widget_anchor` | Anchor a widget within its window for responsive layout |
| `imgui_set_viewport_size` | Resize the application window to exact pixel dimensions |
| `imgui_get_viewport_size` | Get current viewport dimensions |
| `imgui_set_resolution_preset` | Resize to a preset: `720p`, `1080p`, `1440p`, `4k`, `ultrawide` (3440×1440), `mobile` (390×844), `tablet` (1024×768) |
| `imgui_set_dpi_scale` | Scale all UI sizes for HiDPI displays (1.0, 1.5, 2.0) |
| `imgui_set_safe_area` | Set safe area insets (top, bottom, left, right) that anchored windows respect |

### Assets, Textures & Fonts (5)

| Tool | Description |
|------|-------------|
| `imgui_load_texture` | Load a BMP/PPM image as a GPU texture for Image/ImageButton widgets. Creates a checkerboard placeholder if file not found. |
| `imgui_unload_texture` | Free a loaded texture's GPU resources |
| `imgui_load_font` | Load a TTF/OTF font with a glyph-coverage preset, or merge it into another face as a symbol/icon fallback. |
| `imgui_set_font` | Switch the global ImGui font by ID, or restore the embedded `default`. |
| `imgui_list_fonts` | List loaded font IDs, paths, sizes, and the active font. |

Font paths are opened by the native renderer, so use a path visible to that
process. For example, a Windows renderer launched from WSL accepts a Windows
path:

```text
imgui_load_font(id="ui", path="C:\\Windows\\Fonts\\segoeui.ttf", size_pixels=18)
imgui_load_font(id="symbols", path="C:\\Windows\\Fonts\\seguisym.ttf",
                size_pixels=18, glyph_ranges="symbols", merge_into="ui")
imgui_set_font(id="ui")
imgui_list_fonts()
imgui_set_font(id="default")
```

`glyph_ranges` accepts `default`, `unicode`, `symbols`, `cyrillic`, `greek`,
`vietnamese`, `thai`, `japanese`, `korean`, `chinese_full`, or
`chinese_simplified`. Dear ImGui 1.92 resolves Unicode dynamically; merging is
the important step when the primary face does not contain icon or symbol
glyphs. On Windows, Segoe UI Symbol is merged into Segoe UI automatically.

Fonts remain loaded for the renderer process lifetime. IDs are unique; choose a
new ID when comparing another face or size.

### Workflow — Undo/Redo & Snapshots (6)

| Tool | Description |
|------|-------------|
| `imgui_undo` | Undo the last add/remove/update/clear operation (50-deep stack) |
| `imgui_redo` | Re-apply an undone operation |
| `imgui_save_snapshot` | Save the current layout as a named snapshot |
| `imgui_load_snapshot` | Restore a previously saved snapshot |
| `imgui_list_snapshots` | List all snapshots with names, frame numbers, and widget counts |
| `imgui_delete_snapshot` | Delete a snapshot by name |

### Scenes & Multi-Window (7)

| Tool | Description |
|------|-------------|
| `imgui_create_scene` | Create a named scene that groups specific windows together |
| `imgui_delete_scene` | Delete a scene |
| `imgui_switch_scene` | Switch to a scene — only its windows are rendered |
| `imgui_list_scenes` | List all scenes and the active scene |
| `imgui_set_window_layer` | Set z-order layer for a window (higher = renders on top) |
| `imgui_set_widget_visibility` | Set conditional visibility: `always`, `never`, `widget_id.checked==true`, `widget_id.value>0.5` |
| `imgui_set_window_visibility` | Show or hide a window |

### Export & Import (4)

| Tool | Description |
|------|-------------|
| `imgui_export_cpp` | Generate standalone C++ ImGui code that recreates the current layout (with static variables for state) |
| `imgui_export_lua` | Generate Lua imgui binding code that recreates the layout |
| `imgui_export_json` | Serialize the complete live design: windows, widgets, font stack, active font, and tweens |
| `imgui_import_json` | Restore the complete design, including merged fonts and in-progress/looping animations |

### Debug Windows (4)

| Tool | Description |
|------|-------------|
| `imgui_show_demo` | Toggle the ImGui demo window (showcases every widget interactively) |
| `imgui_show_metrics` | Toggle the metrics/debug window (draw calls, vertices, internal state) |
| `imgui_show_about` | Toggle the about window (version, credits, build info) |
| `imgui_show_style_editor` | Toggle the style editor (edit colors and sizes interactively) |

### System, Events & Input State (5)

| Tool | Description |
|------|-------------|
| `imgui_app_status` | Check if the ImGui app is running, binary path, and version |
| `imgui_drain_events` | Retrieve pending native UI events with bounded batching and dropped-event accounting |
| `imgui_get_input_state` | Query mouse position, button states, keyboard state, hovered/active/focused items |
| `imgui_clipboard` | Get or set the system clipboard text |
| `imgui_window_control` | Low-level window ops: set_pos, set_size, set_collapsed, set_focus, set_bg_alpha, set_scroll |

---

## All 102 Widget Types

### Standard ImGui Widgets (91)

| Category | Widget Types |
|----------|-------------|
| **Buttons** | `button`, `small_button`, `invisible_button`, `arrow_button`, `text_link`, `text_link_open_url` |
| **Text** | `text`, `text_colored`, `text_disabled`, `text_wrapped`, `label_text`, `bullet_text`, `separator_text` |
| **Input** | `input_text`, `input_text_multiline`, `input_text_with_hint`, `input_float`, `input_float2`, `input_float3`, `input_float4`, `input_int`, `input_int2`, `input_int3`, `input_int4`, `input_double` |
| **Sliders** | `slider_float`, `slider_float2`, `slider_float3`, `slider_float4`, `slider_int`, `slider_int2`, `slider_int3`, `slider_int4`, `slider_angle`, `vslider_float`, `vslider_int` |
| **Drag** | `drag_float`, `drag_float2`, `drag_float3`, `drag_float4`, `drag_float_range2`, `drag_int`, `drag_int2`, `drag_int3`, `drag_int4`, `drag_int_range2` |
| **Color** | `color_edit3`, `color_edit4`, `color_picker3`, `color_picker4`, `color_button` |
| **Combo & List** | `combo`, `listbox` |
| **Trees** | `tree_node`, `tree_node_ex`, `collapsing_header`, `selectable` |
| **Tables** | `table` — with headers, rows, sorting, column resize, reordering |
| **Menus** | `menu_bar`, `main_menu_bar`, `menu`, `menu_item` — with shortcuts and checkmarks |
| **Popups** | `popup`, `popup_modal` |
| **Tabs** | `tab_bar`, `tab_item` — closable tabs with nested content |
| **Containers** | `child_window` — scrollable sub-regions, `group` — layout grouping |
| **Plots** | `plot_lines`, `plot_histogram` — with overlay text and scale control |
| **Toggles** | `checkbox`, `checkbox_flags`, `radio_button` |
| **Progress** | `progress_bar` — with custom overlay text |
| **Layout** | `separator`, `same_line`, `new_line`, `spacing`, `dummy`, `indent`, `unindent`, `align_text_to_frame_padding`, `bullet` |
| **Values** | `value_bool`, `value_int`, `value_float` |
| **Tooltips** | `tooltip`, `set_item_tooltip` |
| **Images** | `image`, `image_button` — with loaded textures |

### Game UI Patterns (11)

| Widget | What it renders |
|--------|----------------|
| `health_bar` | Labeled HP bar with green→yellow→red color gradient based on percentage, "HP: X/Y" overlay |
| `mana_bar` | Blue/purple MP bar with "MP: X/Y" overlay |
| `inventory_grid` | N×M grid of 48px slots with item labels, selected slot highlight, click events |
| `dialogue_box` | Bordered panel with speaker name, typewriter text reveal, numbered choice buttons |
| `minimap` | Dark bordered square with green player triangle at center, red entity dots from coordinate data |
| `tooltip_card` | Bordered card with rarity-colored title, wrapped description, stat lines |
| `cooldown_radial` | Circle with radial sweep showing remaining cooldown, centered label |
| `notification_toast` | Small rounded rect with opacity fade, positioned at top-right |
| `skill_bar` | Horizontal row of icon squares with keybind labels and cooldown overlays |
| `quest_tracker` | Panel with colored header, quest entries with individual progress bars |
| `character_sheet` | Tabbed panel with headers as tab names, key-value stat pairs from row data |

---

## Supported IDE/CLI Tools

imgui-mcp ships **project-level config files** for every major AI coding tool. Clone the repo, open it in your tool, and the MCP server is already configured.

| Tool | Config File | Format |
|------|------------|--------|
| **Claude Code** | `.mcp.json` | JSON — `mcpServers` with `${CLAUDE_PROJECT_DIR}` |
| **Claude Desktop** | Global config | JSON — `mcpServers` (absolute path required) |
| **Cursor** | `.cursor/mcp.json` | JSON — `mcpServers` with `${workspaceFolder}` |
| **VS Code / Copilot** | `.vscode/mcp.json` | JSON — `servers` key with `type: "stdio"` |
| **Windsurf** | `.windsurf/mcp.json` | JSON — `mcpServers` (relative path) |
| **omp (Oh My Pi)** | `.omp/mcp.json` | JSON — `mcpServers` (relative path) |
| **Cline** | `.cline/mcp.json` | JSON — `mcpServers` with `cwd` + `autoApprove` |
| **Roo Code** | `.roo/mcp.json` | JSON — `mcpServers` with `cwd` + `alwaysAllow` |
| **Zed** | `.zed/settings.json` | JSON — `context_servers` with flat command |
| **Gemini CLI** | `.gemini/settings.json` | JSON — `mcpServers` (relative path) |
| **Codex CLI** | `.codex/config.toml` | TOML — `[mcp_servers.imgui]` |
| **Continue** | `.continue/config.yaml` | YAML — `mcpServers` as list |
| **Augment** | Global config | JSON — `mcpServers` |

Run `./setup.sh` (Linux/macOS) or `.\setup.ps1` (Windows) to also install global configs for tools that need them.

---

## Usage Examples

### Game HUD

> "Create a fantasy game HUD with health at 75%, mana at 60%, a 4×3 inventory grid, and a 5-slot skill bar."

```
imgui_create_window(id="hud", title="Game HUD", w=800, h=600, flags=["menubar"])
imgui_add_widget(window="hud", id="hp", widget_type="health_bar", value=0.75)
imgui_add_widget(window="hud", id="mp", widget_type="mana_bar", value=0.6)
imgui_add_widget(window="hud", id="inv", widget_type="inventory_grid",
                 int_value=4, int_values=[4,3],
                 items=["Sword","Shield","Potion","Bow","Arrow","Ring",...])
imgui_add_widget(window="hud", id="skills", widget_type="skill_bar",
                 int_value=5, items=["Q","W","E","R","F"],
                 values=[0, 0.3, 0, 0.7, 1.0])
imgui_set_theme(theme="fantasy")
imgui_screenshot()  ← agent sees the result and iterates
```

### Animated Damage Flash

> "Animate the health bar dropping from 75% to 20% with a bounce effect over 2 seconds."

```
imgui_animate(window="hud", widget="hp", property="value",
              from=0.75, to=0.2, duration=2.0, ease="bounce")
```

### Export to Game Code

> "Export this layout as C++ code I can drop into my game."

```
imgui_export_cpp(path="src/game_hud.cpp")
→ Generates a standalone render_game_hud() function with static variables
```

### Responsive Testing

> "Test the HUD at 4K resolution and mobile."

```
imgui_set_resolution_preset(preset="4k")
imgui_screenshot()
imgui_set_resolution_preset(preset="mobile")
imgui_screenshot()
```

---

## Architecture

```
imgui-mcp/
├── server.py              Official Python SDK server — 74 tools
├── pyproject.toml         Python package and MCP SDK dependency
├── uv.lock                Reproducible Python dependency lock
├── src/
│   ├── types.h            Shared enums (102 widget types), structs, globals
│   ├── main.cpp           SDL2+OpenGL3 lifecycle, stdin reader, emit helpers
│   ├── render.cpp         render_widget() — 102-case switch, nested containers
│   └── commands.cpp       process_command() — all command handlers
├── vendor/                Dear ImGui v1.92.8 (vendored, MIT)
├── cmake/                 MinGW cross-compilation toolchain
├── assets/windows/        Branded PNG/ICO and Windows resource manifest
├── tests/                 Python bridge + native protocol regression suite
├── VERSION                Authoritative release version
├── CMakeLists.txt         Cross-platform build (Linux, macOS, Windows)
├── setup.sh               Linux/macOS installer for all 13 tools
├── setup.ps1              Windows installer for all 13 tools
├── demo.py                Full feature demonstration script
├── .mcp.json              Claude Code config
├── .cursor/mcp.json       Cursor config
├── .vscode/mcp.json       VS Code / Copilot config
├── .windsurf/mcp.json     Windsurf config
├── .omp/mcp.json          omp config
├── .cline/mcp.json        Cline config
├── .roo/mcp.json          Roo Code config
├── .zed/settings.json     Zed config
├── .gemini/settings.json  Gemini CLI config
├── .codex/config.toml     Codex CLI config
└── .continue/config.yaml  Continue config
```

**Wire protocol:** MCP via the official Python SDK · stdio transport. Local stdio
servers do not use OAuth; clients may display authentication as “Unsupported” or
“N/A”, which is expected.

---

## Building from Source

```bash
# Linux / macOS
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure

# Windows (MSVC + vcpkg)
cmake -B build -DCMAKE_TOOLCHAIN_FILE=[vcpkg]/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release

# Windows (cross-compile from Linux)
cmake -B build-win -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64-x86_64.cmake
cmake --build build-win
```

Requirements: CMake 3.16+, C++17 compiler, SDL2, OpenGL

---

## Trust Model

imgui-mcp is a local development tool. Several tools intentionally read or write
paths available to the server process (screenshots, textures, layouts, and code
exports), so connect only trusted MCP clients and run it with the least filesystem
privileges your project needs.

---

## License

MIT — see [LICENSE](LICENSE). Dear ImGui is vendored under its own [MIT license](vendor/LICENSE.txt).

---

<p align="center">
  <sub>Built for AI agents that need eyes. Powered by Dear ImGui v1.92.8 + SDL2 + OpenGL.</sub><br>
  <sub>Works with Claude · Cursor · VS Code · Windsurf · omp · Zed · Gemini CLI · Codex · Cline · Roo Code · Continue · Augment</sub>
</p>
