<p align="center">
  <h1 align="center">imgui-mcp</h1>
  <p align="center">
    <strong>Let AI agents design, control, and debug Dear ImGui interfaces in real-time.</strong>
  </p>
  <p align="center">
    <a href="#quick-start"><img src="https://img.shields.io/badge/Quick_Start-↓-blue" alt="Quick Start"></a>
    <a href="https://github.com/ocornut/imgui/releases/tag/v1.92.8"><img src="https://img.shields.io/badge/Dear_ImGui-v1.92.8-ff69b4" alt="ImGui Version"></a>
    <a href="https://modelcontextprotocol.io"><img src="https://img.shields.io/badge/MCP-2025--06--18-orange" alt="MCP Protocol"></a>
    <a href="https://github.com/SaloQT/imgui-mcp/blob/main/LICENSE"><img src="https://img.shields.io/badge/License-MIT-green" alt="License"></a>
    <img src="https://img.shields.io/badge/Tools-70+-purple" alt="70+ MCP Tools">
    <img src="https://img.shields.io/badge/Widgets-102-red" alt="102 Widget Types">
  </p>
</p>

---

**imgui-mcp** is a [Model Context Protocol](https://modelcontextprotocol.io) server that gives AI coding agents full control over a live [Dear ImGui](https://github.com/ocornut/imgui) application. Design game HUDs, debug UI layouts, prototype interfaces, and iterate visually — all through natural language.

Works with **Claude Desktop**, **Claude Code**, **Cursor**, **VS Code (Copilot)**, **Windsurf**, **omp**, **Zed**, **Gemini CLI**, **Codex CLI**, **Cline**, **Roo Code**, **Continue**, and **Augment**.

## Why?

AI agents can write code, but they **can't see** the UI they're building. imgui-mcp closes the loop:

```
Agent creates widget → Agent takes screenshot → Agent sees result → Agent iterates
```

No more blind UI development. The agent designs, previews, adjusts, and exports — like a developer with eyes.

## Features

| Category | What you get |
|----------|-------------|
| **70+ MCP Tools** | Create windows, add widgets, draw shapes, animate, theme, export |
| **102 Widget Types** | Every ImGui widget + game patterns (health bars, inventory grids, dialogue boxes, minimaps, skill bars) |
| **Visual Feedback** | Screenshot capture → agent sees what it built |
| **Input Simulation** | Click buttons, type text, hover, scroll — test UI behavior |
| **Animation** | 7 easing functions, property tweens, loop/ping-pong |
| **Theming** | 7 presets (dark, light, classic, fantasy, sci-fi, retro, minimal) + full customization |
| **Layout** | 9-point anchor system, resolution presets (720p→4K+ultrawide), DPI scaling |
| **Workflow** | Undo/redo, named snapshots, version history |
| **Scenes** | Multi-window composition, layer ordering, conditional visibility |
| **Export** | Generate C++, Lua, or JSON from any layout |
| **Game Patterns** | Health/mana bars, inventory grids, dialogue boxes, minimaps, cooldown radials, quest trackers, character sheets |

## Quick Start

### Prerequisites

- **Python 3.8+** (no pip packages needed)
- **SDL2** + **OpenGL** (for the rendering backend)
- **CMake** + **C++17 compiler** (to build from source)

### Install

```bash
git clone https://github.com/SaloQT/imgui-mcp.git
cd imgui-mcp
make build    # or: mkdir build && cd build && cmake .. && make -j$(nproc)
./setup.sh    # installs MCP config for all supported tools
```

**Windows:**
```powershell
# Extract release zip, then:
powershell -ExecutionPolicy Bypass -File .\setup.ps1
```

### Configure your tool

<details>
<summary><strong>Claude Desktop / Claude Code</strong></summary>

Add to `claude_desktop_config.json` or `.mcp.json`:
```json
{
  "mcpServers": {
    "imgui": {
      "command": "python3",
      "args": ["/path/to/imgui-mcp/server.py"]
    }
  }
}
```
</details>

<details>
<summary><strong>Cursor</strong></summary>

Add to `.cursor/mcp.json`:
```json
{
  "mcpServers": {
    "imgui": {
      "command": "python3",
      "args": ["/path/to/imgui-mcp/server.py"]
    }
  }
}
```
</details>

<details>
<summary><strong>VS Code (GitHub Copilot)</strong></summary>

Add to `.vscode/mcp.json`:
```json
{
  "servers": {
    "imgui": {
      "type": "stdio",
      "command": "python3",
      "args": ["/path/to/imgui-mcp/server.py"]
    }
  }
}
```
</details>

<details>
<summary><strong>omp (Oh My Pi)</strong></summary>

Add to `.omp/mcp.json`:
```json
{
  "mcpServers": {
    "imgui": {
      "command": "python3",
      "args": ["/path/to/imgui-mcp/server.py"]
    }
  }
}
```
</details>

<details>
<summary><strong>All other tools</strong></summary>

Run `./setup.sh` (Linux/macOS) or `.\setup.ps1` (Windows) — it auto-configures:
Windsurf, Gemini CLI, Codex CLI, Cline, Roo Code, Zed, Continue, Augment.
</details>

## Usage Example

Once configured, just talk to your AI agent:

> "Create a game HUD with a health bar at 75%, a mana bar at 60%, an inventory grid with 12 slots, and a skill bar with 5 abilities. Use the fantasy theme."

The agent will call:
```
imgui_create_window(id="hud", title="Game HUD", w=800, h=600)
imgui_add_widget(window="hud", id="hp", widget_type="health_bar", value=0.75)
imgui_add_widget(window="hud", id="mp", widget_type="mana_bar", value=0.6)
imgui_add_widget(window="hud", id="inv", widget_type="inventory_grid", ...)
imgui_add_widget(window="hud", id="skills", widget_type="skill_bar", ...)
imgui_set_theme(theme="fantasy")
imgui_screenshot(path="/tmp/hud.bmp")
```

Then it sees the screenshot and iterates.

## Architecture

```
┌─────────────────────────────────────────────────────┐
│  AI Agent (Claude, GPT, Gemini, any MCP client)     │
└────────────────────────┬────────────────────────────┘
                         │ MCP 2025-06-18 (JSON-RPC 2.0 / stdio)
┌────────────────────────▼────────────────────────────┐
│  server.py — 70 tools, zero Python dependencies     │
└────────────────────────┬────────────────────────────┘
                         │ JSON IPC (stdin/stdout)
┌────────────────────────▼────────────────────────────┐
│  imgui_mcp_app — C++17, SDL2 + OpenGL3             │
│  Dear ImGui v1.92.8 · 102 widget types · 60 FPS    │
└─────────────────────────────────────────────────────┘
```

## Tool Reference

<details>
<summary><strong>All 70 MCP Tools</strong></summary>

**Windows & Widgets:** `imgui_create_window` · `imgui_add_widget` · `imgui_update_widget` · `imgui_remove_widget` · `imgui_remove_window` · `imgui_get_state` · `imgui_clear_all`

**Visual Feedback:** `imgui_screenshot` · `imgui_screenshot_widget` · `imgui_screenshot_annotated`

**Manipulation:** `imgui_move_window` · `imgui_resize_window` · `imgui_move_widget` · `imgui_get_widget_rect` · `imgui_get_window_rect` · `imgui_bring_to_front` · `imgui_set_widget_size` · `imgui_get_layout`

**Input Simulation:** `imgui_click_widget` · `imgui_type_text` · `imgui_hover_widget` · `imgui_scroll_window` · `imgui_press_key` · `imgui_set_mouse_pos` · `imgui_focus_widget`

**Drawing:** `imgui_draw` (line, rect, circle, triangle, bezier, polyline, text)

**Style & Theming:** `imgui_set_style` · `imgui_set_theme` · `imgui_set_theme_color` · `imgui_set_style_var` · `imgui_push_style_color` · `imgui_pop_style_color` · `imgui_set_background`

**Animation:** `imgui_animate` · `imgui_stop_animation` · `imgui_stop_all_animations`

**Layout:** `imgui_set_anchor` · `imgui_set_widget_anchor` · `imgui_set_viewport_size` · `imgui_get_viewport_size` · `imgui_set_resolution_preset` · `imgui_set_dpi_scale` · `imgui_set_safe_area`

**Assets:** `imgui_load_texture` · `imgui_unload_texture`

**Workflow:** `imgui_undo` · `imgui_redo` · `imgui_save_snapshot` · `imgui_load_snapshot` · `imgui_list_snapshots` · `imgui_delete_snapshot`

**Scenes:** `imgui_create_scene` · `imgui_delete_scene` · `imgui_switch_scene` · `imgui_list_scenes` · `imgui_set_window_layer` · `imgui_set_widget_visibility` · `imgui_set_window_visibility`

**Export:** `imgui_export_cpp` · `imgui_export_lua` · `imgui_export_json` · `imgui_import_json`

**Debug:** `imgui_show_demo` · `imgui_show_metrics` · `imgui_show_about` · `imgui_show_style_editor`

**System:** `imgui_app_status` · `imgui_get_input_state` · `imgui_clipboard` · `imgui_window_control`

</details>

## Widget Types (102)

<details>
<summary><strong>Full list</strong></summary>

**Buttons:** button, small_button, invisible_button, arrow_button, text_link, text_link_open_url

**Text:** text, text_colored, text_disabled, text_wrapped, label_text, bullet_text, separator_text

**Input:** input_text, input_text_multiline, input_text_with_hint, input_float/2/3/4, input_int/2/3/4, input_double

**Sliders:** slider_float/2/3/4, slider_int/2/3/4, slider_angle, vslider_float, vslider_int

**Drag:** drag_float/2/3/4, drag_float_range2, drag_int/2/3/4, drag_int_range2

**Color:** color_edit3/4, color_picker3/4, color_button

**Combo/List:** combo, listbox

**Trees:** tree_node, tree_node_ex, collapsing_header, selectable

**Tables:** table (with headers, rows, sorting, resize, reorder)

**Menus:** menu_bar, main_menu_bar, menu, menu_item

**Popups:** popup, popup_modal

**Tabs:** tab_bar, tab_item

**Containers:** child_window, group

**Plots:** plot_lines, plot_histogram

**Layout:** separator, same_line, new_line, spacing, dummy, indent, unindent, align_text_to_frame_padding, bullet

**Game Patterns:** health_bar, mana_bar, inventory_grid, dialogue_box, minimap, tooltip_card, cooldown_radial, notification_toast, skill_bar, quest_tracker, character_sheet

**Misc:** progress_bar, checkbox, checkbox_flags, radio_button, value_bool, value_int, value_float, tooltip, image, image_button, draw_list

</details>

## Building from Source

```bash
# Linux / macOS
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)

# Windows (MSVC + vcpkg)
cmake -B build -DCMAKE_TOOLCHAIN_FILE=[vcpkg]/scripts/buildsystems/vcpkg.cmake
cmake --build build --config Release

# Windows (cross-compile from Linux with MinGW)
cmake -B build-win -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64-x86_64.cmake
cmake --build build-win
```

## Releases

Pre-built binaries for **Linux x64** and **Windows x64** are available in [Releases](https://github.com/SaloQT/imgui-mcp/releases).

## License

MIT. Dear ImGui is vendored under its own MIT license.

---

<p align="center">
  <sub>Built for AI agents that need eyes. Made with Dear ImGui v1.92.8.</sub>
</p>
