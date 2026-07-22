#!/usr/bin/env python3
"""
ImGui MCP Server - Model Context Protocol server for controlling Dear ImGui v1.92.8

Implements MCP 2025-06-18 over stdio transport (JSON-RPC 2.0, newline-delimited).
Spawns the compiled imgui_mcp_app and bridges MCP tool calls to the C++ application.

Usage with omp (~/.omp/agent/mcp.json or project .omp/mcp.json):
{
  "mcpServers": {
    "imgui": {
      "command": "python3",
      "args": ["/path/to/server.py"],
      "env": {}
    }
  }
}
"""

import json
import os
import subprocess
import sys
import threading
import time
import queue
from pathlib import Path
from typing import Any, Optional

# ─── Configuration ───────────────────────────────────────────────────────────

SERVER_NAME = "imgui-mcp"
SERVER_VERSION = "1.0.0"
PROTOCOL_VERSION = "2025-06-18"

# Find the imgui_mcp_app binary relative to this script
SCRIPT_DIR = Path(__file__).parent.resolve()
APP_BINARY = SCRIPT_DIR / "build" / "imgui_mcp_app"

# ─── Logging (stderr only) ──────────────────────────────────────────────────

def log(msg: str):
    """Write diagnostic message to stderr (never stdout)."""
    print(f"[imgui-mcp] {msg}", file=sys.stderr, flush=True)


# ─── ImGui App Process Manager ──────────────────────────────────────────────

class ImGuiApp:
    """Manages the C++ imgui_mcp_app subprocess."""

    def __init__(self):
        self.proc: Optional[subprocess.Popen] = None
        self.responses: queue.Queue = queue.Queue()
        self._reader_thread: Optional[threading.Thread] = None
        self._running = False

    def start(self) -> bool:
        """Start the imgui application."""
        binary = str(APP_BINARY)
        if not os.path.isfile(binary):
            log(f"ERROR: Binary not found at {binary}")
            log("Run: cd <project> && mkdir -p build && cd build && cmake .. && make")
            return False

        log(f"Starting imgui app: {binary}")
        try:
            self.proc = subprocess.Popen(
                [binary, "--width", "1280", "--height", "720"],
                stdin=subprocess.PIPE,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                bufsize=1,
            )
        except Exception as e:
            log(f"Failed to start app: {e}")
            return False

        self._running = True
        self._reader_thread = threading.Thread(target=self._read_stdout, daemon=True)
        self._reader_thread.start()

        # Wait for ready signal
        try:
            msg = self.responses.get(timeout=10)
            if msg.get("type") == "ready":
                log(f"ImGui app ready (version {msg.get('version', '?')}, backend: {msg.get('backend', '?')})")
                return True
            else:
                log(f"Unexpected first message: {msg}")
                return True  # Still running, might be ok
        except queue.Empty:
            log("Timeout waiting for app ready signal")
            return False

    def _read_stdout(self):
        """Read JSON lines from the app's stdout."""
        while self._running and self.proc and self.proc.stdout:
            try:
                line = self.proc.stdout.readline()
                if not line:
                    break
                line = line.strip()
                if not line:
                    continue
                try:
                    msg = json.loads(line)
                    self.responses.put(msg)
                except json.JSONDecodeError:
                    log(f"Non-JSON from app: {line[:100]}")
            except Exception as e:
                if self._running:
                    log(f"Reader error: {e}")
                break

    def send_command(self, cmd: dict) -> Optional[dict]:
        """Send a command to the app and wait for acknowledgment."""
        if not self.proc or self.proc.poll() is not None:
            return {"type": "error", "message": "imgui app is not running"}

        cmd_json = json.dumps(cmd) + "\n"
        try:
            self.proc.stdin.write(cmd_json)
            self.proc.stdin.flush()
        except (BrokenPipeError, OSError) as e:
            return {"type": "error", "message": f"Failed to send command: {e}"}

        # Wait for response (ack or state)
        try:
            msg = self.responses.get(timeout=5)
            return msg
        except queue.Empty:
            return {"type": "error", "message": "Timeout waiting for app response"}

    def send_command_no_wait(self, cmd: dict):
        """Send a command without waiting for response."""
        if not self.proc or self.proc.poll() is not None:
            return
        cmd_json = json.dumps(cmd) + "\n"
        try:
            self.proc.stdin.write(cmd_json)
            self.proc.stdin.flush()
        except (BrokenPipeError, OSError):
            pass

    def drain_events(self, timeout: float = 0.1) -> list:
        """Collect any pending events."""
        events = []
        deadline = time.time() + timeout
        while time.time() < deadline:
            try:
                msg = self.responses.get(timeout=0.01)
                events.append(msg)
            except queue.Empty:
                break
        return events

    def is_running(self) -> bool:
        return self.proc is not None and self.proc.poll() is None

    def stop(self):
        """Stop the imgui application."""
        self._running = False
        if self.proc and self.proc.poll() is None:
            self.send_command_no_wait({"cmd": "quit"})
            try:
                self.proc.wait(timeout=3)
            except subprocess.TimeoutExpired:
                self.proc.kill()
        self.proc = None


# ─── Widget Type Enum ───────────────────────────────────────────────────────

WIDGET_TYPES = [
    # Buttons
    "button", "small_button", "invisible_button", "arrow_button",
    # Toggles
    "checkbox", "checkbox_flags", "radio_button",
    # Indicators
    "progress_bar", "bullet",
    # Links
    "text_link", "text_link_open_url",
    # Text
    "text", "text_colored", "text_disabled", "text_wrapped",
    "label_text", "bullet_text", "separator_text",
    # Layout
    "separator", "same_line", "new_line", "spacing", "dummy",
    "indent", "unindent", "align_text_to_frame_padding",
    # Input
    "input_text", "input_text_multiline", "input_text_with_hint",
    "input_float", "input_float2", "input_float3", "input_float4",
    "input_int", "input_int2", "input_int3", "input_int4",
    "input_double",
    # Sliders
    "slider_float", "slider_float2", "slider_float3", "slider_float4",
    "slider_int", "slider_int2", "slider_int3", "slider_int4",
    "slider_angle", "vslider_float", "vslider_int",
    # Drag
    "drag_float", "drag_float2", "drag_float3", "drag_float4",
    "drag_float_range2",
    "drag_int", "drag_int2", "drag_int3", "drag_int4",
    "drag_int_range2",
    # Color
    "color_edit3", "color_edit4", "color_picker3", "color_picker4",
    "color_button",
    # Selection
    "combo", "listbox",
    # Tree / Hierarchy
    "tree_node", "tree_node_ex", "collapsing_header", "selectable",
    # Table
    "table",
    # Menu
    "menu_bar", "main_menu_bar", "menu", "menu_item",
    # Popup
    "popup", "popup_modal",
    # Plots
    "plot_lines", "plot_histogram",
    # Containers
    "child_window", "tab_bar", "tab_item", "group",
    # Value display
    "value_bool", "value_int", "value_float",
    # Tooltip
    "tooltip", "set_item_tooltip",
    # Game UI patterns
    "health_bar", "mana_bar", "inventory_grid", "dialogue_box", "minimap",
    "tooltip_card", "cooldown_radial", "notification_toast", "skill_bar",
    "quest_tracker", "character_sheet",
    # Image / Texture
    "image", "image_button",
]

# ─── Draw Command Types ─────────────────────────────────────────────────────

DRAW_COMMAND_TYPES = [
    "line", "rect", "rect_filled", "circle", "circle_filled",
    "triangle", "triangle_filled", "text", "polyline",
    "convex_poly", "bezier_cubic", "bezier_quadratic",
]

# ─── MCP Tool Definitions ───────────────────────────────────────────────────

TOOLS = [
    # ── 1. imgui_create_window ───────────────────────────────────────────────
    {
        "name": "imgui_create_window",
        "description": (
            "Create a new ImGui window. Windows are containers for widgets. "
            "Supports positioning, sizing, and window flags."
        ),
        "inputSchema": {
            "type": "object",
            "properties": {
                "id": {"type": "string", "description": "Unique window identifier"},
                "title": {"type": "string", "description": "Window title bar text"},
                "x": {"type": "number", "description": "Initial X position (-1 for default)"},
                "y": {"type": "number", "description": "Initial Y position (-1 for default)"},
                "w": {"type": "number", "description": "Initial width (default 400)"},
                "h": {"type": "number", "description": "Initial height (default 300)"},
                "flags": {
                    "type": "array",
                    "items": {
                        "type": "string",
                        "enum": [
                            "menubar", "no_resize", "no_move", "no_collapse",
                            "no_title_bar", "always_auto_resize",
                            "horizontal_scrollbar",
                        ],
                    },
                    "description": "Window flags to apply",
                },
            },
            "required": ["id", "title"],
        },
    },
    # ── 2. imgui_add_widget ──────────────────────────────────────────────────
    {
        "name": "imgui_add_widget",
        "description": (
            "Add a widget to an ImGui window. Supports 80+ widget types including "
            "buttons, text, inputs, sliders, drags, color pickers, combos, trees, "
            "tables, menus, popups, plots, containers, and tooltips."
        ),
        "inputSchema": {
            "type": "object",
            "properties": {
                "window": {"type": "string", "description": "Target window id"},
                "id": {"type": "string", "description": "Unique widget identifier"},
                "widget_type": {
                    "type": "string",
                    "description": "Type of widget to create",
                    "enum": WIDGET_TYPES,
                },
                "label": {"type": "string", "description": "Widget label/display text"},
                "content": {"type": "string", "description": "Text content (for text widgets)"},
                "value": {
                    "type": "number",
                    "description": "Float value (sliders, progress, drag, single-component)",
                },
                "values": {
                    "type": "array",
                    "items": {"type": "number"},
                    "description": (
                        "Array of floats for multi-component widgets "
                        "(float2/3/4, drag_range2) or plot data"
                    ),
                },
                "int_value": {
                    "type": "integer",
                    "description": "Integer value (int sliders, drag, single-component)",
                },
                "int_values": {
                    "type": "array",
                    "items": {"type": "integer"},
                    "description": "Array of ints for multi-component int widgets (int2/3/4)",
                },
                "min": {"type": "number", "description": "Minimum value for float sliders/drag"},
                "max": {"type": "number", "description": "Maximum value for float sliders/drag"},
                "int_min": {"type": "integer", "description": "Minimum for int sliders/drag"},
                "int_max": {"type": "integer", "description": "Maximum for int sliders/drag"},
                "checked": {"type": "boolean", "description": "Checkbox/radio button state"},
                "text": {"type": "string", "description": "Initial text for input widgets"},
                "hint": {"type": "string", "description": "Hint/placeholder text for input_text_with_hint"},
                "color": {
                    "type": "array",
                    "items": {"type": "number"},
                    "description": "RGBA color [r,g,b,a] each 0.0-1.0",
                },
                "items": {
                    "type": "array",
                    "items": {"type": "string"},
                    "description": "Items for combo/listbox/menu",
                },
                "selected": {
                    "type": "integer",
                    "description": "Selected index for combo/listbox/selectable",
                },
                "enabled": {"type": "boolean", "description": "Whether widget is enabled/interactable"},
                "flags": {
                    "type": "integer",
                    "description": "ImGui flags for the widget (e.g. checkbox_flags, tree_node_ex)",
                },
                "tooltip": {"type": "string", "description": "Tooltip text shown on hover"},
                "children": {
                    "type": "array",
                    "items": {"type": "object"},
                    "description": "Nested child widgets (for tree_node, collapsing_header, menu, tab_item, group, child_window)",
                },
                "columns": {"type": "integer", "description": "Number of columns for table widget"},
                "headers": {
                    "type": "array",
                    "items": {"type": "string"},
                    "description": "Column headers for table widget",
                },
                "rows": {
                    "type": "array",
                    "items": {"type": "array"},
                    "description": "Row data for table widget (array of arrays of strings)",
                },
                "format": {
                    "type": "string",
                    "description": "Format string for sliders/drag (e.g. '%.2f', '%d')",
                },
                "speed": {
                    "type": "number",
                    "description": "Speed/sensitivity for drag widgets",
                },
                "overlay": {
                    "type": "string",
                    "description": "Overlay text for progress_bar",
                },
                "size": {
                    "type": "array",
                    "items": {"type": "number"},
                    "description": "Size [w, h] for dummy, button, child_window, invisible_button",
                },
                "dir": {
                    "type": "integer",
                    "description": "Direction for arrow_button (0=left, 1=right, 2=up, 3=down)",
                },
                "step": {
                    "type": "number",
                    "description": "Step increment for input widgets",
                },
                "step_fast": {
                    "type": "number",
                    "description": "Fast step increment for input widgets (ctrl+click)",
                },
                "shortcut": {
                    "type": "string",
                    "description": "Keyboard shortcut string for menu_item (e.g. 'Ctrl+S')",
                },
                "open": {"type": "boolean", "description": "Open state for tree_node/collapsing_header/popup"},
            },
            "required": ["window", "id", "widget_type"],
        },
    },
    # ── 3. imgui_update_widget ───────────────────────────────────────────────
    {
        "name": "imgui_update_widget",
        "description": (
            "Update an existing widget's properties. Can update value, text, checked "
            "state, label, content, color, items, selected index, enabled state, "
            "and plot data."
        ),
        "inputSchema": {
            "type": "object",
            "properties": {
                "window": {"type": "string", "description": "Window containing the widget"},
                "id": {"type": "string", "description": "Widget identifier"},
                "value": {"type": "number", "description": "New float value"},
                "int_value": {"type": "integer", "description": "New integer value"},
                "checked": {"type": "boolean", "description": "New checkbox/radio state"},
                "text": {"type": "string", "description": "New text content for input widgets"},
                "content": {"type": "string", "description": "New display content for text widgets"},
                "label": {"type": "string", "description": "New label"},
                "enabled": {"type": "boolean", "description": "Enable/disable widget"},
                "selected": {"type": "integer", "description": "New selected index"},
                "color": {
                    "type": "array",
                    "items": {"type": "number"},
                    "description": "New RGBA color [r,g,b,a]",
                },
                "values": {
                    "type": "array",
                    "items": {"type": "number"},
                    "description": "New multi-component values or plot data",
                },
                "int_values": {
                    "type": "array",
                    "items": {"type": "integer"},
                    "description": "New multi-component integer values",
                },
                "items": {
                    "type": "array",
                    "items": {"type": "string"},
                    "description": "New items for combo/listbox",
                },
                "min": {"type": "number", "description": "New minimum value"},
                "max": {"type": "number", "description": "New maximum value"},
                "int_min": {"type": "integer", "description": "New int minimum"},
                "int_max": {"type": "integer", "description": "New int maximum"},
                "hint": {"type": "string", "description": "New hint/placeholder text"},
                "tooltip": {"type": "string", "description": "New tooltip text"},
                "format": {"type": "string", "description": "New format string"},
                "speed": {"type": "number", "description": "New drag speed"},
                "overlay": {"type": "string", "description": "New overlay text for progress_bar"},
            },
            "required": ["window", "id"],
        },
    },
    # ── 4. imgui_remove_widget ───────────────────────────────────────────────
    {
        "name": "imgui_remove_widget",
        "description": "Remove a widget from a window",
        "inputSchema": {
            "type": "object",
            "properties": {
                "window": {"type": "string", "description": "Window containing the widget"},
                "id": {"type": "string", "description": "Widget identifier to remove"},
            },
            "required": ["window", "id"],
        },
    },
    # ── 5. imgui_remove_window ───────────────────────────────────────────────
    {
        "name": "imgui_remove_window",
        "description": "Remove a window and all its widgets",
        "inputSchema": {
            "type": "object",
            "properties": {
                "id": {"type": "string", "description": "Window identifier to remove"},
            },
            "required": ["id"],
        },
    },
    # ── 6. imgui_get_state ───────────────────────────────────────────────────
    {
        "name": "imgui_get_state",
        "description": (
            "Get the current state of all windows and widgets. Returns values, "
            "checked states, text content, selected indices, and interaction "
            "flags (clicked, changed, hovered, active, focused)."
        ),
        "inputSchema": {
            "type": "object",
            "properties": {},
        },
    },
    # ── 7. imgui_clear_all ───────────────────────────────────────────────────
    {
        "name": "imgui_clear_all",
        "description": "Remove all windows and widgets, resetting to empty state",
        "inputSchema": {
            "type": "object",
            "properties": {},
        },
    },
    # ── 8. imgui_set_style ───────────────────────────────────────────────────
    {
        "name": "imgui_set_style",
        "description": (
            "Set the ImGui style theme. Choose from built-in themes (dark, light, "
            "classic) or provide custom color overrides."
        ),
        "inputSchema": {
            "type": "object",
            "properties": {
                "theme": {
                    "type": "string",
                    "enum": ["dark", "light", "classic"],
                    "description": "Built-in theme to apply",
                },
                "colors": {
                    "type": "object",
                    "description": (
                        "Custom color overrides as a map of color index/name to "
                        "[r,g,b,a] array (each 0.0-1.0)"
                    ),
                    "additionalProperties": {
                        "type": "array",
                        "items": {"type": "number"},
                    },
                },
            },
            "required": ["theme"],
        },
    },
    # ── 9. imgui_push_style_color ────────────────────────────────────────────
    {
        "name": "imgui_push_style_color",
        "description": (
            "Push a style color override onto the style stack for a specific "
            "window or widget. Must be balanced with imgui_pop_style_color."
        ),
        "inputSchema": {
            "type": "object",
            "properties": {
                "target": {
                    "type": "string",
                    "description": "Target window or widget id to apply the style to",
                },
                "col_idx": {
                    "type": "integer",
                    "description": "ImGuiCol_ index (0=Text, 1=TextDisabled, 2=WindowBg, etc.)",
                },
                "color": {
                    "type": "array",
                    "items": {"type": "number"},
                    "description": "RGBA color [r,g,b,a] each 0.0-1.0",
                },
            },
            "required": ["target", "col_idx", "color"],
        },
    },
    # ── 10. imgui_pop_style_color ────────────────────────────────────────────
    {
        "name": "imgui_pop_style_color",
        "description": (
            "Pop style color overrides from the style stack for a specific "
            "window or widget."
        ),
        "inputSchema": {
            "type": "object",
            "properties": {
                "target": {
                    "type": "string",
                    "description": "Target window or widget id",
                },
                "count": {
                    "type": "integer",
                    "description": "Number of style colors to pop (default 1)",
                    "default": 1,
                },
            },
            "required": ["target"],
        },
    },
    # ── 11. imgui_draw ───────────────────────────────────────────────────────
    {
        "name": "imgui_draw",
        "description": (
            "Issue draw commands to a window's draw list. Supports lines, rects, "
            "circles, triangles, text, polylines, convex polygons, and bezier curves."
        ),
        "inputSchema": {
            "type": "object",
            "properties": {
                "window": {"type": "string", "description": "Target window id"},
                "commands": {
                    "type": "array",
                    "items": {
                        "type": "object",
                        "properties": {
                            "type": {
                                "type": "string",
                                "enum": DRAW_COMMAND_TYPES,
                                "description": "Draw command type",
                            },
                            "p1": {
                                "type": "array",
                                "items": {"type": "number"},
                                "description": "First point [x, y]",
                            },
                            "p2": {
                                "type": "array",
                                "items": {"type": "number"},
                                "description": "Second point [x, y]",
                            },
                            "p3": {
                                "type": "array",
                                "items": {"type": "number"},
                                "description": "Third point [x, y] (triangle, bezier)",
                            },
                            "p4": {
                                "type": "array",
                                "items": {"type": "number"},
                                "description": "Fourth point [x, y] (bezier_cubic)",
                            },
                            "color": {
                                "type": "array",
                                "items": {"type": "number"},
                                "description": "RGBA color [r,g,b,a] each 0.0-1.0",
                            },
                            "thickness": {
                                "type": "number",
                                "description": "Line thickness (default 1.0)",
                            },
                            "filled": {
                                "type": "boolean",
                                "description": "Whether shape is filled",
                            },
                            "num_segments": {
                                "type": "integer",
                                "description": "Number of segments for circle/polyline curves",
                            },
                            "text": {
                                "type": "string",
                                "description": "Text string for text draw command",
                            },
                        },
                        "required": ["type"],
                    },
                    "description": "Array of draw commands to execute",
                },
            },
            "required": ["window", "commands"],
        },
    },
    # ── 12. imgui_set_background ─────────────────────────────────────────────
    {
        "name": "imgui_set_background",
        "description": "Set the background clear color of the viewport",
        "inputSchema": {
            "type": "object",
            "properties": {
                "color": {
                    "type": "array",
                    "items": {"type": "number"},
                    "description": "RGB color [r,g,b] each 0.0-1.0",
                },
            },
            "required": ["color"],
        },
    },
    # ── 13. imgui_show_demo ──────────────────────────────────────────────────
    {
        "name": "imgui_show_demo",
        "description": "Show or hide the ImGui demo window (showcases all available widgets)",
        "inputSchema": {
            "type": "object",
            "properties": {
                "show": {"type": "boolean", "description": "Whether to show the demo window"},
            },
            "required": ["show"],
        },
    },
    # ── 14. imgui_show_metrics ───────────────────────────────────────────────
    {
        "name": "imgui_show_metrics",
        "description": "Show or hide the ImGui metrics/debug window (draw calls, vertices, etc.)",
        "inputSchema": {
            "type": "object",
            "properties": {
                "show": {"type": "boolean", "description": "Whether to show the metrics window"},
            },
            "required": ["show"],
        },
    },
    # ── 15. imgui_show_about ─────────────────────────────────────────────────
    {
        "name": "imgui_show_about",
        "description": "Show or hide the ImGui about window (version info, credits)",
        "inputSchema": {
            "type": "object",
            "properties": {
                "show": {"type": "boolean", "description": "Whether to show the about window"},
            },
            "required": ["show"],
        },
    },
    # ── 16. imgui_show_style_editor ──────────────────────────────────────────
    {
        "name": "imgui_show_style_editor",
        "description": "Show or hide the ImGui style editor window (edit colors and sizes interactively)",
        "inputSchema": {
            "type": "object",
            "properties": {
                "show": {"type": "boolean", "description": "Whether to show the style editor window"},
            },
            "required": ["show"],
        },
    },
    # ── 17. imgui_window_control ─────────────────────────────────────────────
    {
        "name": "imgui_window_control",
        "description": (
            "Control a window's properties: position, size, collapsed state, "
            "focus, background alpha, or scroll position."
        ),
        "inputSchema": {
            "type": "object",
            "properties": {
                "id": {"type": "string", "description": "Window identifier"},
                "action": {
                    "type": "string",
                    "enum": [
                        "set_pos", "set_size", "set_collapsed",
                        "set_focus", "set_bg_alpha", "set_scroll",
                    ],
                    "description": "Control action to perform",
                },
                "value": {
                    "description": (
                        "Value for the action: [x,y] for set_pos/set_size, "
                        "bool for set_collapsed/set_focus, "
                        "float for set_bg_alpha, [x,y] for set_scroll"
                    ),
                },
            },
            "required": ["id", "action"],
        },
    },
    # ── 18. imgui_get_input_state ────────────────────────────────────────────
    {
        "name": "imgui_get_input_state",
        "description": (
            "Query input state: mouse position, mouse buttons, keyboard keys, "
            "and item interaction states."
        ),
        "inputSchema": {
            "type": "object",
            "properties": {
                "queries": {
                    "type": "array",
                    "items": {
                        "type": "string",
                        "enum": [
                            "mouse_pos", "mouse_down", "mouse_clicked",
                            "key_down", "key_pressed",
                            "is_any_item_hovered", "is_any_item_active",
                        ],
                    },
                    "description": "List of input state queries to perform",
                },
            },
            "required": ["queries"],
        },
    },
    # ── 19. imgui_clipboard ──────────────────────────────────────────────────
    {
        "name": "imgui_clipboard",
        "description": "Get or set the system clipboard text via ImGui",
        "inputSchema": {
            "type": "object",
            "properties": {
                "action": {
                    "type": "string",
                    "enum": ["get", "set"],
                    "description": "Whether to get or set clipboard content",
                },
                "text": {
                    "type": "string",
                    "description": "Text to set on the clipboard (required for 'set' action)",
                },
            },
            "required": ["action"],
        },
    },
    # ── 20. imgui_app_status ─────────────────────────────────────────────────
    {
        "name": "imgui_app_status",
        "description": (
            "Check if the ImGui application is running and get its status "
            "including binary path and availability."
        ),
        "inputSchema": {
            "type": "object",
            "properties": {},
        },
    },
    # ── 21. imgui_screenshot ────────────────────────────────────────────────
    {
        "name": "imgui_screenshot",
        "description": (
            "Capture a screenshot of the current ImGui frame and save it as a BMP file."
        ),
        "inputSchema": {
            "type": "object",
            "properties": {
                "path": {
                    "type": "string",
                    "description": "File path to save the screenshot (default: /tmp/imgui_screenshot.bmp)",
                },
                "format": {
                    "type": "string",
                    "enum": ["bmp", "ppm"],
                    "description": "Image format (currently only bmp is supported natively)",
                },
            },
        },
    },
    # ── 22. imgui_screenshot_widget ─────────────────────────────────────────
    {
        "name": "imgui_screenshot_widget",
        "description": (
            "Capture a screenshot with information about a specific widget's bounds. "
            "Returns the screenshot path and widget location data."
        ),
        "inputSchema": {
            "type": "object",
            "properties": {
                "window": {
                    "type": "string",
                    "description": "Window ID containing the widget",
                },
                "widget": {
                    "type": "string",
                    "description": "Widget ID to capture bounds for",
                },
                "path": {
                    "type": "string",
                    "description": "File path to save the screenshot (default: /tmp/imgui_screenshot.bmp)",
                },
            },
            "required": ["window", "widget"],
        },
    },
    # ── 23. imgui_screenshot_annotated ──────────────────────────────────────
    {
        "name": "imgui_screenshot_annotated",
        "description": (
            "Capture a screenshot with widget ID overlays for debugging and inspection."
        ),
        "inputSchema": {
            "type": "object",
            "properties": {
                "path": {
                    "type": "string",
                    "description": "File path to save the annotated screenshot (default: /tmp/imgui_screenshot.bmp)",
                },
            },
        },
    },
    # ── 24. imgui_click_widget ──────────────────────────────────────────────
    {
        "name": "imgui_click_widget",
        "description": "Simulate a mouse click on a widget by its ID",
        "inputSchema": {
            "type": "object",
            "properties": {
                "window": {"type": "string", "description": "Window ID containing the widget"},
                "id": {"type": "string", "description": "Widget ID to click"},
                "button": {"type": "integer", "enum": [0, 1, 2], "description": "Mouse button (0=left, 1=right, 2=middle)", "default": 0},
            },
            "required": ["window", "id"],
        },
    },
    # ── 25. imgui_type_text ─────────────────────────────────────────────────
    {
        "name": "imgui_type_text",
        "description": "Type text into a widget (focuses it first, then injects characters)",
        "inputSchema": {
            "type": "object",
            "properties": {
                "window": {"type": "string", "description": "Window ID containing the widget"},
                "id": {"type": "string", "description": "Widget ID to type into"},
                "text": {"type": "string", "description": "Text to type"},
            },
            "required": ["window", "id", "text"],
        },
    },
    # ── 26. imgui_hover_widget ──────────────────────────────────────────────
    {
        "name": "imgui_hover_widget",
        "description": "Move the mouse cursor to hover over a widget",
        "inputSchema": {
            "type": "object",
            "properties": {
                "window": {"type": "string", "description": "Window ID containing the widget"},
                "id": {"type": "string", "description": "Widget ID to hover"},
            },
            "required": ["window", "id"],
        },
    },
    # ── 27. imgui_scroll_window ─────────────────────────────────────────────
    {
        "name": "imgui_scroll_window",
        "description": "Scroll the mouse wheel over a window",
        "inputSchema": {
            "type": "object",
            "properties": {
                "window": {"type": "string", "description": "Window ID to scroll"},
                "delta_y": {"type": "number", "description": "Vertical scroll amount (positive=up, negative=down)", "default": 1.0},
            },
            "required": ["window"],
        },
    },
    # ── 28. imgui_press_key ─────────────────────────────────────────────────
    {
        "name": "imgui_press_key",
        "description": (
            "Simulate a key press and release. Supports single keys and modifier combos. "
            "Examples: 'Enter', 'Tab', 'Escape', 'Ctrl+S', 'Shift+A', 'Ctrl+Shift+Z'"
        ),
        "inputSchema": {
            "type": "object",
            "properties": {
                "key": {"type": "string", "description": "Key name or combo (e.g. 'Enter', 'Tab', 'Ctrl+S')"},
            },
            "required": ["key"],
        },
    },
    # ── 29. imgui_set_mouse_pos ─────────────────────────────────────────────
    {
        "name": "imgui_set_mouse_pos",
        "description": "Set the mouse cursor position in screen coordinates",
        "inputSchema": {
            "type": "object",
            "properties": {
                "x": {"type": "number", "description": "X coordinate"},
                "y": {"type": "number", "description": "Y coordinate"},
            },
            "required": ["x", "y"],
        },
    },
    # ── 30. imgui_focus_widget ──────────────────────────────────────────────
    {
        "name": "imgui_focus_widget",
        "description": "Set keyboard focus to a widget (it will receive focus on the next frame)",
        "inputSchema": {
            "type": "object",
            "properties": {
                "window": {"type": "string", "description": "Window ID containing the widget"},
                "id": {"type": "string", "description": "Widget ID to focus"},
            },
            "required": ["window", "id"],
        },
    },
    # ── 31. imgui_animate ──────────────────────────────────────────────────
    {
        "name": "imgui_animate",
        "description": (
            "Animate a widget property over time using a tween/animation. "
            "Supports multiple easing functions and looping."
        ),
        "inputSchema": {
            "type": "object",
            "properties": {
                "window": {"type": "string", "description": "Window ID containing the widget"},
                "widget": {"type": "string", "description": "Widget ID to animate"},
                "property": {
                    "type": "string",
                    "enum": ["value", "opacity", "pos_x", "size_x", "size_y", "int_value"],
                    "description": "Widget property to animate",
                },
                "from": {"type": "number", "description": "Start value", "default": 0},
                "to": {"type": "number", "description": "End value", "default": 1},
                "duration": {"type": "number", "description": "Duration in seconds", "default": 1.0},
                "ease": {
                    "type": "string",
                    "enum": ["linear", "ease_in", "ease_out", "ease_in_out", "bounce", "elastic", "back"],
                    "description": "Easing function",
                    "default": "linear",
                },
                "loop": {"type": "boolean", "description": "Whether to loop the animation (ping-pong)", "default": False},
            },
            "required": ["window", "widget", "property"],
        },
    },
    # ── 32. imgui_stop_animation ────────────────────────────────────────────
    {
        "name": "imgui_stop_animation",
        "description": (
            "Stop animations for a specific widget, or all animations in a window "
            "if widget is omitted."
        ),
        "inputSchema": {
            "type": "object",
            "properties": {
                "window": {"type": "string", "description": "Window ID"},
                "widget": {"type": "string", "description": "Widget ID (omit to stop all animations in the window)"},
            },
            "required": ["window"],
        },
    },
    # ── 33. imgui_stop_all_animations ───────────────────────────────────────
    {
        "name": "imgui_stop_all_animations",
        "description": "Stop and remove all active animations across all windows",
        "inputSchema": {
            "type": "object",
            "properties": {},
        },
    },
    # ── Window/Widget Manipulation Tools ────────────────────────────────────
    {
        "name": "imgui_move_window",
        "description": "Move a window to a new position",
        "inputSchema": {
            "type": "object",
            "properties": {
                "id": {"type": "string", "description": "Window identifier"},
                "x": {"type": "number", "description": "New X position"},
                "y": {"type": "number", "description": "New Y position"},
            },
            "required": ["id", "x", "y"],
        },
    },
    {
        "name": "imgui_resize_window",
        "description": "Resize a window to new dimensions",
        "inputSchema": {
            "type": "object",
            "properties": {
                "id": {"type": "string", "description": "Window identifier"},
                "w": {"type": "number", "description": "New width"},
                "h": {"type": "number", "description": "New height"},
            },
            "required": ["id", "w", "h"],
        },
    },
    {
        "name": "imgui_move_widget",
        "description": "Move a widget by applying a cursor offset before rendering",
        "inputSchema": {
            "type": "object",
            "properties": {
                "window": {"type": "string", "description": "Window containing the widget"},
                "id": {"type": "string", "description": "Widget identifier"},
                "offset_x": {"type": "number", "description": "Horizontal offset in pixels"},
                "offset_y": {"type": "number", "description": "Vertical offset in pixels"},
            },
            "required": ["window", "id", "offset_x", "offset_y"],
        },
    },
    {
        "name": "imgui_get_widget_rect",
        "description": "Get the actual rendered bounding box of a widget in pixel coordinates",
        "inputSchema": {
            "type": "object",
            "properties": {
                "window": {"type": "string", "description": "Window containing the widget"},
                "id": {"type": "string", "description": "Widget identifier"},
            },
            "required": ["window", "id"],
        },
    },
    {
        "name": "imgui_get_window_rect",
        "description": "Get the position and size of a window",
        "inputSchema": {
            "type": "object",
            "properties": {
                "id": {"type": "string", "description": "Window identifier"},
            },
            "required": ["id"],
        },
    },
    {
        "name": "imgui_bring_to_front",
        "description": "Bring a window to the front and give it focus",
        "inputSchema": {
            "type": "object",
            "properties": {
                "id": {"type": "string", "description": "Window identifier"},
            },
            "required": ["id"],
        },
    },
    {
        "name": "imgui_set_widget_size",
        "description": "Set the size of a widget",
        "inputSchema": {
            "type": "object",
            "properties": {
                "window": {"type": "string", "description": "Window containing the widget"},
                "id": {"type": "string", "description": "Widget identifier"},
                "width": {"type": "number", "description": "New width"},
                "height": {"type": "number", "description": "New height"},
            },
            "required": ["window", "id", "width", "height"],
        },
    },
    {
        "name": "imgui_get_layout",
        "description": "Get the layout of all widgets in a window with their bounding boxes",
        "inputSchema": {
            "type": "object",
            "properties": {
                "window": {"type": "string", "description": "Window identifier"},
            },
            "required": ["window"],
        },
    },
    # ── Layout / Responsive ─────────────────────────────────────────────────
    {
        "name": "imgui_set_anchor",
        "description": "Set a window's anchor position relative to the viewport (e.g. top_left, center, bottom_right)",
        "inputSchema": {
            "type": "object",
            "properties": {
                "window": {"type": "string", "description": "Window identifier"},
                "anchor": {
                    "type": "string",
                    "enum": ["none", "top_left", "top_center", "top_right", "center_left", "center", "center_right", "bottom_left", "bottom_center", "bottom_right"],
                    "description": "Anchor position relative to viewport",
                },
                "offset_x": {"type": "number", "description": "X offset from anchor point (default 0)"},
                "offset_y": {"type": "number", "description": "Y offset from anchor point (default 0)"},
            },
            "required": ["window", "anchor"],
        },
    },
    {
        "name": "imgui_set_widget_anchor",
        "description": "Set a widget's anchor position for responsive layout within its window",
        "inputSchema": {
            "type": "object",
            "properties": {
                "window": {"type": "string", "description": "Window containing the widget"},
                "widget": {"type": "string", "description": "Widget identifier"},
                "anchor": {
                    "type": "string",
                    "enum": ["none", "top_left", "top_center", "top_right", "center_left", "center", "center_right", "bottom_left", "bottom_center", "bottom_right"],
                    "description": "Anchor position",
                },
                "offset_x": {"type": "number", "description": "X offset from anchor point (default 0)"},
                "offset_y": {"type": "number", "description": "Y offset from anchor point (default 0)"},
            },
            "required": ["window", "widget", "anchor"],
        },
    },
    {
        "name": "imgui_set_viewport_size",
        "description": "Resize the application viewport (SDL window) to specific pixel dimensions",
        "inputSchema": {
            "type": "object",
            "properties": {
                "width": {"type": "integer", "description": "Viewport width in pixels"},
                "height": {"type": "integer", "description": "Viewport height in pixels"},
            },
            "required": ["width", "height"],
        },
    },
    {
        "name": "imgui_get_viewport_size",
        "description": "Get the current viewport (SDL window) dimensions",
        "inputSchema": {
            "type": "object",
            "properties": {},
        },
    },
    {
        "name": "imgui_set_resolution_preset",
        "description": "Resize the viewport to a common resolution preset",
        "inputSchema": {
            "type": "object",
            "properties": {
                "preset": {
                    "type": "string",
                    "enum": ["720p", "1080p", "1440p", "4k", "ultrawide", "mobile", "tablet"],
                    "description": "Resolution preset name",
                },
            },
            "required": ["preset"],
        },
    },
    {
        "name": "imgui_set_dpi_scale",
        "description": "Set the DPI/UI scale factor for all ImGui sizes",
        "inputSchema": {
            "type": "object",
            "properties": {
                "scale": {"type": "number", "description": "Scale factor (e.g. 1.0, 1.5, 2.0)"},
            },
            "required": ["scale"],
        },
    },
    {
        "name": "imgui_set_safe_area",
        "description": "Set safe area insets (in pixels) that anchored windows will respect",
        "inputSchema": {
            "type": "object",
            "properties": {
                "top": {"type": "number", "description": "Top inset in pixels"},
                "bottom": {"type": "number", "description": "Bottom inset in pixels"},
                "left": {"type": "number", "description": "Left inset in pixels"},
                "right": {"type": "number", "description": "Right inset in pixels"},
            },
            "required": ["top", "bottom", "left", "right"],
        },
    },
    # ── Scene / Multi-window ─────────────────────────────────────────────────
    {
        "name": "imgui_create_scene",
        "description": "Create a named scene that controls which windows are visible together",
        "inputSchema": {
            "type": "object",
            "properties": {
                "name": {"type": "string", "description": "Scene name"},
                "windows": {
                    "type": "array",
                    "items": {"type": "string"},
                    "description": "List of window IDs visible in this scene",
                },
            },
            "required": ["name", "windows"],
        },
    },
    {
        "name": "imgui_delete_scene",
        "description": "Delete a scene by name",
        "inputSchema": {
            "type": "object",
            "properties": {
                "name": {"type": "string", "description": "Scene name to delete"},
            },
            "required": ["name"],
        },
    },
    {
        "name": "imgui_switch_scene",
        "description": "Switch to a named scene, showing only its windows",
        "inputSchema": {
            "type": "object",
            "properties": {
                "name": {"type": "string", "description": "Scene name to activate"},
            },
            "required": ["name"],
        },
    },
    {
        "name": "imgui_list_scenes",
        "description": "List all scenes and their window lists, plus the active scene",
        "inputSchema": {
            "type": "object",
            "properties": {},
        },
    },
    {
        "name": "imgui_set_window_layer",
        "description": "Set the render layer (z-order) of a window. Higher layers render on top.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "window": {"type": "string", "description": "Window identifier"},
                "layer": {"type": "integer", "description": "Layer value (higher = on top)"},
            },
            "required": ["window", "layer"],
        },
    },
    {
        "name": "imgui_set_widget_visibility",
        "description": (
            "Set conditional visibility on a widget. Conditions: 'always', 'never', "
            "'widget_id.checked==true', 'widget_id.value>0.5'"
        ),
        "inputSchema": {
            "type": "object",
            "properties": {
                "window": {"type": "string", "description": "Window identifier"},
                "widget": {"type": "string", "description": "Widget identifier"},
                "condition": {
                    "type": "string",
                    "description": "Visibility condition expression",
                },
            },
            "required": ["window", "widget", "condition"],
        },
    },
    {
        "name": "imgui_set_window_visibility",
        "description": "Show or hide a window by setting its open state",
        "inputSchema": {
            "type": "object",
            "properties": {
                "window": {"type": "string", "description": "Window identifier"},
                "visible": {"type": "boolean", "description": "Whether the window should be visible"},
            },
            "required": ["window", "visible"],
        },
    },
    # ── imgui_save_snapshot ──────────────────────────────────────────────────
    {
        "name": "imgui_save_snapshot",
        "description": (
            "Save the current layout state as a named snapshot. "
            "Snapshots can be restored later with imgui_load_snapshot."
        ),
        "inputSchema": {
            "type": "object",
            "properties": {
                "name": {
                    "type": "string",
                    "description": "Snapshot name (default: snapshot_N)",
                },
            },
        },
    },
    # ── imgui_load_snapshot ──────────────────────────────────────────────────
    {
        "name": "imgui_load_snapshot",
        "description": (
            "Restore a previously saved snapshot by name. "
            "The current state is pushed to the undo stack before restoring."
        ),
        "inputSchema": {
            "type": "object",
            "properties": {
                "name": {
                    "type": "string",
                    "description": "Name of the snapshot to restore",
                },
            },
            "required": ["name"],
        },
    },
    # ── imgui_list_snapshots ─────────────────────────────────────────────────
    {
        "name": "imgui_list_snapshots",
        "description": "List all saved snapshots with their names, frame numbers, and widget counts",
        "inputSchema": {
            "type": "object",
            "properties": {},
        },
    },
    # ── imgui_delete_snapshot ────────────────────────────────────────────────
    {
        "name": "imgui_delete_snapshot",
        "description": "Delete a saved snapshot by name",
        "inputSchema": {
            "type": "object",
            "properties": {
                "name": {
                    "type": "string",
                    "description": "Name of the snapshot to delete",
                },
            },
            "required": ["name"],
        },
    },
    # ── imgui_undo ───────────────────────────────────────────────────────────
    {
        "name": "imgui_undo",
        "description": (
            "Undo the last destructive operation (add/remove/update widget, "
            "remove window, clear all). The undone state is pushed to the redo stack."
        ),
        "inputSchema": {
            "type": "object",
            "properties": {},
        },
    },
    # ── imgui_redo ───────────────────────────────────────────────────────────
    {
        "name": "imgui_redo",
        "description": (
            "Redo the last undone operation. Re-applies the state that was "
            "reversed by imgui_undo."
        ),
        "inputSchema": {
            "type": "object",
            "properties": {},
        },
    },
    # ── imgui_export_cpp ────────────────────────────────────────────────────
    {
        "name": "imgui_export_cpp",
        "description": (
            "Export the current UI layout as C++ ImGui code that recreates "
            "all windows and widgets with their current values."
        ),
        "inputSchema": {
            "type": "object",
            "properties": {
                "path": {
                    "type": "string",
                    "description": "Optional file path to write the C++ code to. If omitted, code is returned inline.",
                },
            },
        },
    },
    # ── imgui_export_lua ────────────────────────────────────────────────────
    {
        "name": "imgui_export_lua",
        "description": (
            "Export the current UI layout as Lua ImGui bindings code that "
            "recreates all windows and widgets with their current values."
        ),
        "inputSchema": {
            "type": "object",
            "properties": {
                "path": {
                    "type": "string",
                    "description": "Optional file path to write the Lua code to. If omitted, code is returned inline.",
                },
            },
        },
    },
    # ── imgui_export_json ───────────────────────────────────────────────────
    {
        "name": "imgui_export_json",
        "description": (
            "Export the current UI layout as structured JSON with version info, "
            "window properties, and all widget definitions."
        ),
        "inputSchema": {
            "type": "object",
            "properties": {
                "path": {
                    "type": "string",
                    "description": "Optional file path to write the JSON to. If omitted, JSON is returned inline.",
                },
            },
        },
    },
    # ── imgui_import_json ───────────────────────────────────────────────────
    {
        "name": "imgui_import_json",
        "description": (
            "Import a UI layout from JSON (same format as imgui_export_json). "
            "Clears the current layout and recreates all windows and widgets."
        ),
        "inputSchema": {
            "type": "object",
            "properties": {
                "json": {
                    "type": "object",
                    "description": "The layout JSON object to import (same format as export_json output).",
                },
                "path": {
                    "type": "string",
                    "description": "Path to a JSON file to import. Use this OR the 'json' field.",
                },
            },
        },
    },
    # ── Asset/Theming Tools ─────────────────────────────────────────────────
    {
        "name": "imgui_load_texture",
        "description": (
            "Load a texture from a BMP or PPM file. If the file doesn't exist or "
            "is unsupported, creates a 64x64 checkerboard placeholder texture."
        ),
        "inputSchema": {
            "type": "object",
            "properties": {
                "id": {"type": "string", "description": "Unique texture identifier"},
                "path": {"type": "string", "description": "Path to BMP or PPM image file"},
            },
            "required": ["id", "path"],
        },
    },
    {
        "name": "imgui_unload_texture",
        "description": "Unload a previously loaded texture and free its GPU resources.",
        "inputSchema": {
            "type": "object",
            "properties": {
                "id": {"type": "string", "description": "Texture identifier to unload"},
            },
            "required": ["id"],
        },
    },
    {
        "name": "imgui_set_theme",
        "description": (
            "Apply a theme preset to the ImGui interface. Available themes: "
            "dark, light, classic, fantasy (gold/amber accents), "
            "scifi (cyan/teal accents), retro (green terminal style), "
            "minimal (white, thin borders, no rounding)."
        ),
        "inputSchema": {
            "type": "object",
            "properties": {
                "theme": {
                    "type": "string",
                    "enum": ["dark", "light", "classic", "fantasy", "scifi", "retro", "minimal"],
                    "description": "Theme preset name",
                },
            },
            "required": ["theme"],
        },
    },
    {
        "name": "imgui_set_theme_color",
        "description": (
            "Set a specific ImGui style color by name. Color names include: "
            "Text, WindowBg, FrameBg, Button, ButtonHovered, ButtonActive, "
            "Header, HeaderHovered, HeaderActive, Border, CheckMark, SliderGrab, etc."
        ),
        "inputSchema": {
            "type": "object",
            "properties": {
                "col_name": {"type": "string", "description": "ImGui color name (e.g. 'WindowBg', 'Button', 'Text')"},
                "color": {
                    "type": "array",
                    "items": {"type": "number"},
                    "minItems": 4,
                    "maxItems": 4,
                    "description": "RGBA color values [r, g, b, a] each 0.0-1.0",
                },
            },
            "required": ["col_name", "color"],
        },
    },
    {
        "name": "imgui_set_style_var",
        "description": (
            "Set an ImGui style variable. Float vars: FrameRounding, WindowRounding, "
            "IndentSpacing, ScrollbarSize, GrabMinSize, etc. "
            "Vec2 vars: FramePadding, ItemSpacing, WindowPadding, etc."
        ),
        "inputSchema": {
            "type": "object",
            "properties": {
                "var_name": {"type": "string", "description": "Style variable name (e.g. 'FrameRounding', 'ItemSpacing')"},
                "value": {
                    "description": "Float value for scalar vars, or [x, y] array for vec2 vars",
                },
            },
            "required": ["var_name", "value"],
        },
    },
]


# ─── MCP Server ─────────────────────────────────────────────────────────────

class MCPServer:
    """MCP server implementing JSON-RPC 2.0 over stdio."""

    def __init__(self):
        self.app = ImGuiApp()
        self._initialized = False

    def run(self):
        """Main loop: read JSON-RPC from stdin, dispatch, write responses to stdout."""
        log(f"{SERVER_NAME} v{SERVER_VERSION} starting (MCP {PROTOCOL_VERSION})")
        log(f"App binary: {APP_BINARY}")

        for line in sys.stdin:
            line = line.strip()
            if not line:
                continue
            try:
                msg = json.loads(line)
            except json.JSONDecodeError as e:
                self._send_error(None, -32700, f"Parse error: {e}")
                continue

            self._dispatch(msg)

        # stdin closed, shutdown
        self.app.stop()
        log("Server shutting down (stdin closed)")

    def _dispatch(self, msg: dict):
        """Route a JSON-RPC message."""
        method = msg.get("method", "")
        msg_id = msg.get("id")
        params = msg.get("params", {})

        # Notifications (no id)
        if msg_id is None:
            if method == "notifications/initialized":
                log("Client initialized")
            elif method == "notifications/cancelled":
                pass  # Ignore cancellations
            else:
                log(f"Unknown notification: {method}")
            return

        # Requests (have id)
        if method == "initialize":
            self._handle_initialize(msg_id, params)
        elif method == "tools/list":
            self._handle_tools_list(msg_id)
        elif method == "tools/call":
            self._handle_tools_call(msg_id, params)
        elif method == "ping":
            self._send_result(msg_id, {})
        else:
            self._send_error(msg_id, -32601, f"Method not found: {method}")

    def _handle_initialize(self, msg_id: Any, params: dict):
        """Handle initialize request."""
        log(f"Initialize request from {params.get('clientInfo', {}).get('name', 'unknown')}")

        # Start the imgui app
        if not self.app.is_running():
            if not self.app.start():
                log("WARNING: Could not start imgui app (display may not be available)")

        self._initialized = True
        self._send_result(msg_id, {
            "protocolVersion": PROTOCOL_VERSION,
            "capabilities": {
                "tools": {"listChanged": False},
            },
            "serverInfo": {
                "name": SERVER_NAME,
                "version": SERVER_VERSION,
            },
        })

    def _handle_tools_list(self, msg_id: Any):
        """Handle tools/list request."""
        self._send_result(msg_id, {"tools": TOOLS})

    def _handle_tools_call(self, msg_id: Any, params: dict):
        """Handle tools/call request."""
        tool_name = params.get("name", "")
        arguments = params.get("arguments", {})

        try:
            result = self._call_tool(tool_name, arguments)
            self._send_result(msg_id, {
                "content": [{"type": "text", "text": json.dumps(result, indent=2)}],
            })
        except Exception as e:
            log(f"Tool error ({tool_name}): {e}")
            self._send_result(msg_id, {
                "content": [{"type": "text", "text": f"Error: {e}"}],
                "isError": True,
            })

    def _call_tool(self, name: str, args: dict) -> dict:
        """Execute a tool and return the result."""
        # Tools that don't need the app running
        if name == "imgui_app_status":
            return {
                "running": self.app.is_running(),
                "binary": str(APP_BINARY),
                "binary_exists": os.path.isfile(str(APP_BINARY)),
            }

        # All other tools need the app
        if not self.app.is_running():
            if not self.app.start():
                return {"error": "ImGui app is not running and could not be started. "
                        "Ensure a display is available and the binary is built."}

        if name == "imgui_create_window":
            cmd = {"cmd": "create_window"}
            for k in ("id", "title", "x", "y", "w", "h", "flags"):
                if k in args:
                    cmd[k] = args[k]
            return self.app.send_command(cmd) or {"error": "no response"}

        elif name == "imgui_add_widget":
            cmd = {"cmd": "add_widget"}
            cmd.update(args)
            return self.app.send_command(cmd) or {"error": "no response"}

        elif name == "imgui_update_widget":
            cmd = {"cmd": "update_widget"}
            cmd.update(args)
            return self.app.send_command(cmd) or {"error": "no response"}

        elif name == "imgui_remove_widget":
            cmd = {"cmd": "remove_widget"}
            cmd.update(args)
            return self.app.send_command(cmd) or {"error": "no response"}

        elif name == "imgui_remove_window":
            cmd = {"cmd": "remove_window", "id": args.get("id", "")}
            return self.app.send_command(cmd) or {"error": "no response"}

        elif name == "imgui_get_state":
            return self.app.send_command({"cmd": "get_state"}) or {"error": "no response"}

        elif name == "imgui_clear_all":
            return self.app.send_command({"cmd": "clear_all"}) or {"error": "no response"}

        elif name == "imgui_set_style":
            cmd = {"cmd": "set_style", "theme": args.get("theme", "dark")}
            if "colors" in args:
                cmd["colors"] = args["colors"]
            return self.app.send_command(cmd) or {"error": "no response"}

        elif name == "imgui_push_style_color":
            cmd = {
                "cmd": "push_style_color",
                "target": args.get("target", ""),
                "col_idx": args.get("col_idx", 0),
                "color": args.get("color", [1.0, 1.0, 1.0, 1.0]),
            }
            return self.app.send_command(cmd) or {"error": "no response"}

        elif name == "imgui_pop_style_color":
            cmd = {
                "cmd": "pop_style_color",
                "target": args.get("target", ""),
                "count": args.get("count", 1),
            }
            return self.app.send_command(cmd) or {"error": "no response"}

        elif name == "imgui_draw":
            cmd = {
                "cmd": "draw",
                "window": args.get("window", ""),
                "commands": args.get("commands", []),
            }
            return self.app.send_command(cmd) or {"error": "no response"}

        elif name == "imgui_set_background":
            cmd = {"cmd": "set_background", "color": args.get("color", [0.06, 0.06, 0.08])}
            return self.app.send_command(cmd) or {"error": "no response"}

        elif name == "imgui_show_demo":
            cmd = {"cmd": "show_demo", "show": args.get("show", True)}
            return self.app.send_command(cmd) or {"error": "no response"}

        elif name == "imgui_show_metrics":
            cmd = {"cmd": "show_metrics", "show": args.get("show", True)}
            return self.app.send_command(cmd) or {"error": "no response"}

        elif name == "imgui_show_about":
            cmd = {"cmd": "show_about", "show": args.get("show", True)}
            return self.app.send_command(cmd) or {"error": "no response"}

        elif name == "imgui_show_style_editor":
            cmd = {"cmd": "show_style_editor", "show": args.get("show", True)}
            return self.app.send_command(cmd) or {"error": "no response"}

        elif name == "imgui_window_control":
            cmd = {
                "cmd": "window_control",
                "id": args.get("id", ""),
                "action": args.get("action", ""),
            }
            if "value" in args:
                cmd["value"] = args["value"]
            return self.app.send_command(cmd) or {"error": "no response"}

        elif name == "imgui_get_input_state":
            cmd = {"cmd": "get_input_state", "queries": args.get("queries", [])}
            return self.app.send_command(cmd) or {"error": "no response"}

        elif name == "imgui_clipboard":
            cmd = {"cmd": "clipboard", "action": args.get("action", "get")}
            if "text" in args:
                cmd["text"] = args["text"]
            return self.app.send_command(cmd) or {"error": "no response"}

        elif name == "imgui_screenshot":
            cmd = {
                "cmd": "screenshot",
                "path": args.get("path", "/tmp/imgui_screenshot.bmp"),
            }
            return self.app.send_command(cmd) or {"error": "no response"}

        elif name == "imgui_screenshot_widget":
            cmd = {
                "cmd": "screenshot_widget",
                "window": args.get("window", ""),
                "widget": args.get("widget", ""),
                "path": args.get("path", "/tmp/imgui_screenshot.bmp"),
            }
            return self.app.send_command(cmd) or {"error": "no response"}

        elif name == "imgui_screenshot_annotated":
            cmd = {
                "cmd": "screenshot_annotated",
                "path": args.get("path", "/tmp/imgui_screenshot.bmp"),
            }
            return self.app.send_command(cmd) or {"error": "no response"}

        elif name == "imgui_click_widget":
            cmd = {
                "cmd": "click_widget",
                "window": args.get("window", ""),
                "id": args.get("id", ""),
                "button": args.get("button", 0),
            }
            return self.app.send_command(cmd) or {"error": "no response"}

        elif name == "imgui_type_text":
            cmd = {
                "cmd": "type_text",
                "window": args.get("window", ""),
                "id": args.get("id", ""),
                "text": args.get("text", ""),
            }
            return self.app.send_command(cmd) or {"error": "no response"}

        elif name == "imgui_hover_widget":
            cmd = {
                "cmd": "hover_widget",
                "window": args.get("window", ""),
                "id": args.get("id", ""),
            }
            return self.app.send_command(cmd) or {"error": "no response"}

        elif name == "imgui_scroll_window":
            cmd = {
                "cmd": "scroll_window",
                "window": args.get("window", ""),
                "delta_y": args.get("delta_y", 1.0),
            }
            return self.app.send_command(cmd) or {"error": "no response"}

        elif name == "imgui_press_key":
            cmd = {
                "cmd": "press_key",
                "key": args.get("key", ""),
            }
            return self.app.send_command(cmd) or {"error": "no response"}

        elif name == "imgui_set_mouse_pos":
            cmd = {
                "cmd": "set_mouse_pos",
                "x": args.get("x", 0.0),
                "y": args.get("y", 0.0),
            }
            return self.app.send_command(cmd) or {"error": "no response"}

        elif name == "imgui_focus_widget":
            cmd = {
                "cmd": "focus_widget",
                "window": args.get("window", ""),
                "id": args.get("id", ""),
            }
            return self.app.send_command(cmd) or {"error": "no response"}

        elif name == "imgui_animate":
            cmd = {
                "cmd": "animate",
                "window": args.get("window", ""),
                "widget": args.get("widget", ""),
                "property": args.get("property", "value"),
                "from": args.get("from", 0.0),
                "to": args.get("to", 1.0),
                "duration": args.get("duration", 1.0),
                "ease": args.get("ease", "linear"),
                "loop": args.get("loop", False),
            }
            return self.app.send_command(cmd) or {"error": "no response"}

        elif name == "imgui_stop_animation":
            cmd = {
                "cmd": "stop_animation",
                "window": args.get("window", ""),
            }
            if "widget" in args:
                cmd["widget"] = args["widget"]
            return self.app.send_command(cmd) or {"error": "no response"}

        elif name == "imgui_stop_all_animations":
            return self.app.send_command({"cmd": "stop_all_animations"}) or {"error": "no response"}

        # ── Window/Widget manipulation handlers ─────────────────────────────
        elif name == "imgui_move_window":
            cmd = {
                "cmd": "move_window",
                "id": args.get("id", ""),
                "x": args.get("x", 0.0),
                "y": args.get("y", 0.0),
            }
            return self.app.send_command(cmd) or {"error": "no response"}

        elif name == "imgui_resize_window":
            cmd = {
                "cmd": "resize_window",
                "id": args.get("id", ""),
                "w": args.get("w", 400.0),
                "h": args.get("h", 300.0),
            }
            return self.app.send_command(cmd) or {"error": "no response"}

        elif name == "imgui_move_widget":
            cmd = {
                "cmd": "move_widget",
                "window": args.get("window", ""),
                "id": args.get("id", ""),
                "offset_x": args.get("offset_x", 0.0),
                "offset_y": args.get("offset_y", 0.0),
            }
            return self.app.send_command(cmd) or {"error": "no response"}

        elif name == "imgui_get_widget_rect":
            cmd = {
                "cmd": "get_widget_rect",
                "window": args.get("window", ""),
                "id": args.get("id", ""),
            }
            return self.app.send_command(cmd) or {"error": "no response"}

        elif name == "imgui_get_window_rect":
            cmd = {
                "cmd": "get_window_rect",
                "id": args.get("id", ""),
            }
            return self.app.send_command(cmd) or {"error": "no response"}

        elif name == "imgui_bring_to_front":
            cmd = {
                "cmd": "bring_to_front",
                "id": args.get("id", ""),
            }
            return self.app.send_command(cmd) or {"error": "no response"}

        elif name == "imgui_set_widget_size":
            cmd = {
                "cmd": "set_widget_size",
                "window": args.get("window", ""),
                "id": args.get("id", ""),
                "width": args.get("width", 0.0),
                "height": args.get("height", 0.0),
            }
            return self.app.send_command(cmd) or {"error": "no response"}

        elif name == "imgui_get_layout":
            cmd = {
                "cmd": "get_layout",
                "window": args.get("window", ""),
            }
            return self.app.send_command(cmd) or {"error": "no response"}

        elif name == "imgui_set_anchor":
            cmd = {
                "cmd": "set_anchor",
                "window": args.get("window", ""),
                "anchor": args.get("anchor", "none"),
            }
            if "offset_x" in args:
                cmd["offset_x"] = args["offset_x"]
            if "offset_y" in args:
                cmd["offset_y"] = args["offset_y"]
            return self.app.send_command(cmd) or {"error": "no response"}

        elif name == "imgui_set_widget_anchor":
            cmd = {
                "cmd": "set_widget_anchor",
                "window": args.get("window", ""),
                "widget": args.get("widget", ""),
                "anchor": args.get("anchor", "none"),
            }
            if "offset_x" in args:
                cmd["offset_x"] = args["offset_x"]
            if "offset_y" in args:
                cmd["offset_y"] = args["offset_y"]
            return self.app.send_command(cmd) or {"error": "no response"}

        elif name == "imgui_set_viewport_size":
            cmd = {
                "cmd": "set_viewport_size",
                "width": args.get("width", 1280),
                "height": args.get("height", 720),
            }
            return self.app.send_command(cmd) or {"error": "no response"}

        elif name == "imgui_get_viewport_size":
            return self.app.send_command({"cmd": "get_viewport_size"}) or {"error": "no response"}

        elif name == "imgui_set_resolution_preset":
            cmd = {
                "cmd": "set_resolution_preset",
                "preset": args.get("preset", "1080p"),
            }
            return self.app.send_command(cmd) or {"error": "no response"}

        elif name == "imgui_set_dpi_scale":
            cmd = {
                "cmd": "set_dpi_scale",
                "scale": args.get("scale", 1.0),
            }
            return self.app.send_command(cmd) or {"error": "no response"}

        elif name == "imgui_set_safe_area":
            cmd = {
                "cmd": "set_safe_area",
                "top": args.get("top", 0),
                "bottom": args.get("bottom", 0),
                "left": args.get("left", 0),
                "right": args.get("right", 0),
            }
            return self.app.send_command(cmd) or {"error": "no response"}

        elif name == "imgui_create_scene":
            cmd = {"cmd": "create_scene", "name": args.get("name", ""), "windows": args.get("windows", [])}
            return self.app.send_command(cmd) or {"error": "no response"}

        elif name == "imgui_delete_scene":
            cmd = {"cmd": "delete_scene", "name": args.get("name", "")}
            return self.app.send_command(cmd) or {"error": "no response"}

        elif name == "imgui_switch_scene":
            cmd = {"cmd": "switch_scene", "name": args.get("name", "")}
            return self.app.send_command(cmd) or {"error": "no response"}

        elif name == "imgui_list_scenes":
            return self.app.send_command({"cmd": "list_scenes"}) or {"error": "no response"}

        elif name == "imgui_set_window_layer":
            cmd = {"cmd": "set_window_layer", "id": args.get("window", ""), "layer": args.get("layer", 0)}
            return self.app.send_command(cmd) or {"error": "no response"}

        elif name == "imgui_set_widget_visibility":
            cmd = {"cmd": "set_widget_visibility", "window": args.get("window", ""), "id": args.get("widget", ""), "condition": args.get("condition", "always")}
            return self.app.send_command(cmd) or {"error": "no response"}

        elif name == "imgui_set_window_visibility":
            cmd = {"cmd": "set_window_visibility", "id": args.get("window", ""), "visible": args.get("visible", True)}
            return self.app.send_command(cmd) or {"error": "no response"}

        elif name == "imgui_save_snapshot":
            cmd = {"cmd": "save_snapshot"}
            if "name" in args:
                cmd["name"] = args["name"]
            return self.app.send_command(cmd) or {"error": "no response"}

        elif name == "imgui_load_snapshot":
            cmd = {"cmd": "load_snapshot", "name": args.get("name", "")}
            return self.app.send_command(cmd) or {"error": "no response"}

        elif name == "imgui_list_snapshots":
            return self.app.send_command({"cmd": "list_snapshots"}) or {"error": "no response"}

        elif name == "imgui_delete_snapshot":
            cmd = {"cmd": "delete_snapshot", "name": args.get("name", "")}
            return self.app.send_command(cmd) or {"error": "no response"}

        elif name == "imgui_undo":
            return self.app.send_command({"cmd": "undo"}) or {"error": "no response"}

        elif name == "imgui_redo":
            return self.app.send_command({"cmd": "redo"}) or {"error": "no response"}

        elif name == "imgui_export_cpp":
            cmd = {"cmd": "export_cpp"}
            if "path" in args:
                cmd["path"] = args["path"]
            return self.app.send_command(cmd) or {"error": "no response"}

        elif name == "imgui_export_lua":
            cmd = {"cmd": "export_lua"}
            if "path" in args:
                cmd["path"] = args["path"]
            return self.app.send_command(cmd) or {"error": "no response"}

        elif name == "imgui_export_json":
            cmd = {"cmd": "export_json"}
            if "path" in args:
                cmd["path"] = args["path"]
            return self.app.send_command(cmd) or {"error": "no response"}

        elif name == "imgui_import_json":
            cmd = {"cmd": "import_json"}
            if "json" in args:
                cmd["json"] = args["json"]
            if "path" in args:
                cmd["path"] = args["path"]
            return self.app.send_command(cmd) or {"error": "no response"}

        elif name == "imgui_load_texture":
            cmd = {"cmd": "load_texture", "id": args.get("id", ""), "path": args.get("path", "")}
            return self.app.send_command(cmd) or {"error": "no response"}

        elif name == "imgui_unload_texture":
            cmd = {"cmd": "unload_texture", "id": args.get("id", "")}
            return self.app.send_command(cmd) or {"error": "no response"}

        elif name == "imgui_set_theme":
            cmd = {"cmd": "set_theme", "theme": args.get("theme", "dark")}
            return self.app.send_command(cmd) or {"error": "no response"}

        elif name == "imgui_set_theme_color":
            cmd = {"cmd": "set_theme_color", "col_name": args.get("col_name", ""), "color": args.get("color", [0, 0, 0, 1])}
            return self.app.send_command(cmd) or {"error": "no response"}

        elif name == "imgui_set_style_var":
            cmd = {"cmd": "set_style_var", "var_name": args.get("var_name", ""), "value": args.get("value", 0)}
            return self.app.send_command(cmd) or {"error": "no response"}

        else:
            return {"error": f"Unknown tool: {name}"}

    # ─── JSON-RPC Response Helpers ───────────────────────────────────────────

    def _send_result(self, msg_id: Any, result: dict):
        """Send a JSON-RPC success response."""
        self._write({"jsonrpc": "2.0", "id": msg_id, "result": result})

    def _send_error(self, msg_id: Any, code: int, message: str, data: Any = None):
        """Send a JSON-RPC error response."""
        error = {"code": code, "message": message}
        if data is not None:
            error["data"] = data
        self._write({"jsonrpc": "2.0", "id": msg_id, "error": error})

    def _write(self, msg: dict):
        """Write a JSON-RPC message to stdout (newline-delimited)."""
        line = json.dumps(msg, separators=(",", ":"))
        sys.stdout.write(line + "\n")
        sys.stdout.flush()


# ─── Entry Point ─────────────────────────────────────────────────────────────

if __name__ == "__main__":
    server = MCPServer()
    try:
        server.run()
    except KeyboardInterrupt:
        server.app.stop()
        log("Interrupted")
    except Exception as e:
        log(f"Fatal error: {e}")
        server.app.stop()
        sys.exit(1)
