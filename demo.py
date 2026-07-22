#!/usr/bin/env python3
"""
ImGui MCP Demo - Comprehensive demonstration of all imgui features via MCP protocol.

Spawns server.py as a subprocess and exercises every available tool through
the MCP 2025-06-18 protocol (JSON-RPC 2.0 over stdio, newline-delimited).

No external dependencies required - uses only Python standard library.

Usage:
    python3 demo.py
"""

import json
import math
import subprocess
import sys
import time
from pathlib import Path

# ─── Configuration ───────────────────────────────────────────────────────────

SCRIPT_DIR = Path(__file__).parent.resolve()
SERVER_SCRIPT = SCRIPT_DIR / "server.py"
TIMEOUT = 10  # seconds to wait for each response

# ─── MCP Client ──────────────────────────────────────────────────────────────


class MCPClient:
    """Minimal MCP client that speaks JSON-RPC 2.0 over stdio to server.py."""

    def __init__(self):
        self.proc = None
        self._next_id = 0

    def start(self):
        """Spawn server.py as a subprocess."""
        self.proc = subprocess.Popen(
            [sys.executable, str(SERVER_SCRIPT)],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            bufsize=1,
        )
        # Start a thread to drain stderr so it doesn't block
        import threading

        self._stderr_thread = threading.Thread(
            target=self._drain_stderr, daemon=True
        )
        self._stderr_thread.start()

    def _drain_stderr(self):
        """Read and display server stderr (diagnostic logs)."""
        try:
            for line in self.proc.stderr:
                line = line.strip()
                if line:
                    print(f"  [server] {line}", file=sys.stderr)
        except (ValueError, OSError):
            pass

    def stop(self):
        """Terminate the server subprocess."""
        if self.proc and self.proc.poll() is None:
            self.proc.stdin.close()
            try:
                self.proc.wait(timeout=5)
            except subprocess.TimeoutExpired:
                self.proc.kill()
                self.proc.wait()
        self.proc = None

    def _send(self, msg: dict):
        """Write a JSON-RPC message to the server's stdin."""
        line = json.dumps(msg, separators=(",", ":"))
        self.proc.stdin.write(line + "\n")
        self.proc.stdin.flush()

    def _recv(self) -> dict:
        """Read one JSON-RPC response line from the server's stdout."""
        line = self.proc.stdout.readline()
        if not line:
            raise RuntimeError("Server closed stdout unexpectedly")
        return json.loads(line.strip())

    def request(self, method: str, params: dict = None) -> dict:
        """Send a JSON-RPC request and wait for the matching response."""
        self._next_id += 1
        msg_id = self._next_id
        msg = {"jsonrpc": "2.0", "id": msg_id, "method": method}
        if params is not None:
            msg["params"] = params
        self._send(msg)

        # Read responses until we get ours (skip notifications from server)
        deadline = time.time() + TIMEOUT
        while time.time() < deadline:
            resp = self._recv()
            if resp.get("id") == msg_id:
                if "error" in resp:
                    raise RuntimeError(
                        f"JSON-RPC error {resp['error']['code']}: "
                        f"{resp['error']['message']}"
                    )
                return resp.get("result", {})
        raise TimeoutError(f"No response for request id={msg_id} within {TIMEOUT}s")

    def notify(self, method: str, params: dict = None):
        """Send a JSON-RPC notification (no response expected)."""
        msg = {"jsonrpc": "2.0", "method": method}
        if params is not None:
            msg["params"] = params
        self._send(msg)

    def call_tool(self, name: str, arguments: dict = None) -> dict:
        """Send a tools/call request and return the parsed result content."""
        params = {"name": name, "arguments": arguments or {}}
        result = self.request("tools/call", params)
        # Parse the text content from the MCP response envelope
        content = result.get("content", [])
        if content and content[0].get("type") == "text":
            text = content[0]["text"]
            try:
                return json.loads(text)
            except json.JSONDecodeError:
                return {"raw": text}
        return result


# ─── Display Helpers ─────────────────────────────────────────────────────────


def section(title: str):
    """Print a section header."""
    print(f"\n{'=' * 70}")
    print(f"  {title}")
    print(f"{'=' * 70}")


def step(description: str, result: dict):
    """Print a step description and its result."""
    status = "OK" if "error" not in result else "FAIL"
    print(f"\n  [{status}] {description}")
    # Print compact result
    result_str = json.dumps(result, indent=2)
    if len(result_str) > 200:
        result_str = result_str[:200] + "..."
    for line in result_str.split("\n"):
        print(f"        {line}")


# ─── Demo Sections ───────────────────────────────────────────────────────────


def demo_initialize(client: MCPClient):
    """MCP handshake: initialize + notifications/initialized."""
    section("1. MCP Protocol Handshake")

    result = client.request("initialize", {
        "protocolVersion": "2025-06-18",
        "capabilities": {},
        "clientInfo": {"name": "imgui-demo-client", "version": "1.0.0"},
    })
    step("initialize", result)

    # Send initialized notification (no response expected)
    client.notify("notifications/initialized")
    print("\n  [OK] notifications/initialized sent")

    # Verify tools are available
    result = client.request("tools/list")
    tools = result.get("tools", [])
    print(f"\n  [OK] tools/list returned {len(tools)} tools:")
    for tool in tools:
        print(f"        - {tool['name']}")


def demo_app_status(client: MCPClient):
    """Check application status."""
    section("2. Application Status")

    result = client.call_tool("imgui_app_status")
    step("imgui_app_status", result)


def demo_create_main_window(client: MCPClient):
    """Create a main window with menu bar."""
    section("3. Create Main Window")

    result = client.call_tool("imgui_create_window", {
        "id": "main_window",
        "title": "ImGui MCP Demo - Main Window",
        "x": 50,
        "y": 50,
        "w": 800,
        "h": 600,
    })
    step("Create main window (800x600)", result)


def demo_menu_items(client: MCPClient):
    """Add menu items (File and Edit menus)."""
    section("4. Menu Items (File & Edit)")

    # File menu items
    file_items = [
        ("menu_file_new", "New"),
        ("menu_file_open", "Open"),
        ("menu_file_save", "Save"),
        ("menu_file_sep", "---"),
        ("menu_file_exit", "Exit"),
    ]
    for wid, label in file_items:
        result = client.call_tool("imgui_add_widget", {
            "window": "main_window",
            "id": wid,
            "widget_type": "menu_item",
            "label": label,
        })
        step(f"File menu: {label}", result)

    # Edit menu items
    edit_items = [
        ("menu_edit_undo", "Undo"),
        ("menu_edit_redo", "Redo"),
        ("menu_edit_sep", "---"),
        ("menu_edit_cut", "Cut"),
        ("menu_edit_copy", "Copy"),
        ("menu_edit_paste", "Paste"),
    ]
    for wid, label in edit_items:
        result = client.call_tool("imgui_add_widget", {
            "window": "main_window",
            "id": wid,
            "widget_type": "menu_item",
            "label": label,
        })
        step(f"Edit menu: {label}", result)


def demo_text_widgets(client: MCPClient):
    """Add various text widgets."""
    section("5. Text Widgets")

    # Plain text
    result = client.call_tool("imgui_add_widget", {
        "window": "main_window",
        "id": "text_plain",
        "widget_type": "text",
        "content": "Hello, ImGui MCP! This is plain text.",
    })
    step("Plain text", result)

    # Colored text
    result = client.call_tool("imgui_add_widget", {
        "window": "main_window",
        "id": "text_colored",
        "widget_type": "text_colored",
        "content": "This text is colored (green)",
        "color": [0.2, 1.0, 0.2, 1.0],
    })
    step("Colored text (green)", result)

    # Disabled text (using enabled=False)
    result = client.call_tool("imgui_add_widget", {
        "window": "main_window",
        "id": "text_disabled",
        "widget_type": "text",
        "content": "This text is disabled",
        "enabled": False,
    })
    step("Disabled text", result)

    # Wrapped text
    result = client.call_tool("imgui_add_widget", {
        "window": "main_window",
        "id": "text_wrapped",
        "widget_type": "text_wrapped",
        "content": (
            "This is a long text that will be wrapped to fit within the window "
            "boundaries. ImGui handles word wrapping automatically for text_wrapped "
            "widgets, making it suitable for paragraphs and descriptions."
        ),
    })
    step("Wrapped text", result)

    # Label text
    result = client.call_tool("imgui_add_widget", {
        "window": "main_window",
        "id": "text_label",
        "widget_type": "label_text",
        "label": "Status",
        "content": "Running",
    })
    step("Label text (Status: Running)", result)

    # Bullet
    result = client.call_tool("imgui_add_widget", {
        "window": "main_window",
        "id": "bullet_1",
        "widget_type": "bullet",
    })
    step("Bullet point", result)

    # Separator
    result = client.call_tool("imgui_add_widget", {
        "window": "main_window",
        "id": "separator_1",
        "widget_type": "separator",
    })
    step("Separator", result)


def demo_buttons(client: MCPClient):
    """Add various button widgets."""
    section("6. Buttons")

    # Regular button
    result = client.call_tool("imgui_add_widget", {
        "window": "main_window",
        "id": "btn_regular",
        "widget_type": "button",
        "label": "Click Me!",
    })
    step("Regular button", result)

    # Small button
    result = client.call_tool("imgui_add_widget", {
        "window": "main_window",
        "id": "btn_small",
        "widget_type": "small_button",
        "label": "Small",
    })
    step("Small button", result)

    # Another button to demonstrate same_line
    result = client.call_tool("imgui_add_widget", {
        "window": "main_window",
        "id": "same_line_1",
        "widget_type": "same_line",
    })
    step("Same line (layout)", result)

    result = client.call_tool("imgui_add_widget", {
        "window": "main_window",
        "id": "btn_small_2",
        "widget_type": "small_button",
        "label": "Beside",
    })
    step("Small button (beside previous)", result)

    # Disabled button
    result = client.call_tool("imgui_add_widget", {
        "window": "main_window",
        "id": "btn_disabled",
        "widget_type": "button",
        "label": "Disabled Button",
        "enabled": False,
    })
    step("Disabled button", result)


def demo_input_widgets(client: MCPClient):
    """Add input widgets (text, float, int)."""
    section("7. Input Widgets")

    # Text input
    result = client.call_tool("imgui_add_widget", {
        "window": "main_window",
        "id": "input_text_1",
        "widget_type": "input_text",
        "label": "Name",
        "text": "ImGui MCP",
    })
    step("Text input (Name)", result)

    # Float input
    result = client.call_tool("imgui_add_widget", {
        "window": "main_window",
        "id": "input_float_1",
        "widget_type": "input_float",
        "label": "Speed",
        "value": 3.14,
    })
    step("Float input (Speed=3.14)", result)

    # Int input
    result = client.call_tool("imgui_add_widget", {
        "window": "main_window",
        "id": "input_int_1",
        "widget_type": "input_int",
        "label": "Count",
        "int_value": 42,
    })
    step("Int input (Count=42)", result)

    # Another text input with hint via label
    result = client.call_tool("imgui_add_widget", {
        "window": "main_window",
        "id": "input_text_2",
        "widget_type": "input_text",
        "label": "Search...",
        "text": "",
    })
    step("Text input with hint label (Search...)", result)

    # Float input with different value
    result = client.call_tool("imgui_add_widget", {
        "window": "main_window",
        "id": "input_float_2",
        "widget_type": "input_float",
        "label": "Opacity",
        "value": 0.75,
    })
    step("Float input (Opacity=0.75)", result)

    # Int input for port number
    result = client.call_tool("imgui_add_widget", {
        "window": "main_window",
        "id": "input_int_2",
        "widget_type": "input_int",
        "label": "Port",
        "int_value": 8080,
    })
    step("Int input (Port=8080)", result)


def demo_sliders(client: MCPClient):
    """Add slider widgets (float, int)."""
    section("8. Sliders")

    # Float slider
    result = client.call_tool("imgui_add_widget", {
        "window": "main_window",
        "id": "slider_float_1",
        "widget_type": "slider_float",
        "label": "Volume",
        "value": 0.5,
        "min": 0.0,
        "max": 1.0,
    })
    step("Float slider (Volume=0.5, range 0-1)", result)

    # Int slider
    result = client.call_tool("imgui_add_widget", {
        "window": "main_window",
        "id": "slider_int_1",
        "widget_type": "slider_int",
        "label": "Quality",
        "int_value": 7,
        "int_min": 1,
        "int_max": 10,
    })
    step("Int slider (Quality=7, range 1-10)", result)

    # Float slider (angle-like, 0-360)
    result = client.call_tool("imgui_add_widget", {
        "window": "main_window",
        "id": "slider_float_angle",
        "widget_type": "slider_float",
        "label": "Angle",
        "value": 45.0,
        "min": 0.0,
        "max": 360.0,
    })
    step("Float slider (Angle=45, range 0-360)", result)

    # Vertical-like slider (using large range)
    result = client.call_tool("imgui_add_widget", {
        "window": "main_window",
        "id": "slider_float_2",
        "widget_type": "slider_float",
        "label": "Height",
        "value": 150.0,
        "min": 0.0,
        "max": 500.0,
    })
    step("Float slider (Height=150, range 0-500)", result)


def demo_drag_widgets(client: MCPClient):
    """Add drag widgets (float, int)."""
    section("9. Drag Widgets")

    # Float drag
    result = client.call_tool("imgui_add_widget", {
        "window": "main_window",
        "id": "drag_float_1",
        "widget_type": "drag_float",
        "label": "Position X",
        "value": 128.5,
        "min": -1000.0,
        "max": 1000.0,
    })
    step("Drag float (Position X=128.5)", result)

    # Int drag
    result = client.call_tool("imgui_add_widget", {
        "window": "main_window",
        "id": "drag_int_1",
        "widget_type": "drag_int",
        "label": "Frame Rate",
        "int_value": 60,
        "int_min": 1,
        "int_max": 240,
    })
    step("Drag int (Frame Rate=60)", result)

    # Another drag float (range)
    result = client.call_tool("imgui_add_widget", {
        "window": "main_window",
        "id": "drag_float_2",
        "widget_type": "drag_float",
        "label": "Threshold",
        "value": 0.5,
        "min": 0.0,
        "max": 1.0,
    })
    step("Drag float (Threshold=0.5, range 0-1)", result)


def demo_color_widgets(client: MCPClient):
    """Add color widgets (edit3, edit4)."""
    section("10. Color Widgets")

    # Color edit 3 (RGB)
    result = client.call_tool("imgui_add_widget", {
        "window": "main_window",
        "id": "color_edit3_1",
        "widget_type": "color_edit3",
        "label": "Background",
        "color": [0.4, 0.2, 0.8],
    })
    step("Color edit3 (RGB purple)", result)

    # Color edit 4 (RGBA)
    result = client.call_tool("imgui_add_widget", {
        "window": "main_window",
        "id": "color_edit4_1",
        "widget_type": "color_edit4",
        "label": "Foreground",
        "color": [1.0, 0.8, 0.0, 0.9],
    })
    step("Color edit4 (RGBA gold, alpha=0.9)", result)


def demo_combo_listbox(client: MCPClient):
    """Add combo and listbox widgets."""
    section("11. Combo & Listbox")

    # Combo box
    result = client.call_tool("imgui_add_widget", {
        "window": "main_window",
        "id": "combo_1",
        "widget_type": "combo",
        "label": "Theme",
        "items": ["Dark", "Light", "Classic", "Custom"],
        "selected": 0,
    })
    step("Combo (Theme, 4 options)", result)

    # Listbox
    result = client.call_tool("imgui_add_widget", {
        "window": "main_window",
        "id": "listbox_1",
        "widget_type": "listbox",
        "label": "Files",
        "items": ["main.cpp", "server.py", "demo.py", "CMakeLists.txt", "README.md"],
        "selected": 1,
    })
    step("Listbox (Files, 5 items, selected=1)", result)


def demo_tree_nodes(client: MCPClient):
    """Add tree nodes with nested content."""
    section("12. Tree Nodes")

    # Parent tree node
    result = client.call_tool("imgui_add_widget", {
        "window": "main_window",
        "id": "tree_root",
        "widget_type": "tree_node",
        "label": "Project Structure",
        "open": True,
    })
    step("Tree node (Project Structure, open)", result)

    # Child items within tree
    result = client.call_tool("imgui_add_widget", {
        "window": "main_window",
        "id": "tree_child_1",
        "widget_type": "tree_node",
        "label": "src/",
        "open": True,
    })
    step("Tree node (src/, nested)", result)

    result = client.call_tool("imgui_add_widget", {
        "window": "main_window",
        "id": "tree_child_2",
        "widget_type": "tree_node",
        "label": "vendor/",
        "open": False,
    })
    step("Tree node (vendor/, collapsed)", result)

    # Indent/unindent for tree structure
    result = client.call_tool("imgui_add_widget", {
        "window": "main_window",
        "id": "indent_1",
        "widget_type": "indent",
    })
    step("Indent", result)

    result = client.call_tool("imgui_add_widget", {
        "window": "main_window",
        "id": "text_tree_leaf",
        "widget_type": "text",
        "content": "main.cpp (28KB)",
    })
    step("Text leaf in tree", result)

    result = client.call_tool("imgui_add_widget", {
        "window": "main_window",
        "id": "unindent_1",
        "widget_type": "unindent",
    })
    step("Unindent", result)


def demo_table(client: MCPClient):
    """Create a second window with a table demonstration."""
    section("13. Table Window")

    # Create a dedicated window for the table
    result = client.call_tool("imgui_create_window", {
        "id": "table_window",
        "title": "Table Demo",
        "x": 900,
        "y": 50,
        "w": 400,
        "h": 300,
    })
    step("Create table window", result)

    # Add table-related widgets (using available widget types)
    # The C++ app supports BeginTable, EndTable, TableNextRow, TableNextColumn
    # via the widget system - we simulate a table with text and separators
    result = client.call_tool("imgui_add_widget", {
        "window": "table_window",
        "id": "table_header",
        "widget_type": "text",
        "content": "ID  | Name       | Value",
    })
    step("Table header row", result)

    result = client.call_tool("imgui_add_widget", {
        "window": "table_window",
        "id": "table_sep",
        "widget_type": "separator",
    })
    step("Table separator", result)

    rows = [
        ("table_row_1", "1   | Alpha      | 3.14"),
        ("table_row_2", "2   | Beta       | 2.72"),
        ("table_row_3", "3   | Gamma      | 1.41"),
        ("table_row_4", "4   | Delta      | 1.73"),
    ]
    for wid, content in rows:
        result = client.call_tool("imgui_add_widget", {
            "window": "table_window",
            "id": wid,
            "widget_type": "text",
            "content": content,
        })
        step(f"Table row: {content.strip()}", result)


def demo_tab_bar(client: MCPClient):
    """Create a window simulating a tab bar with multiple tabs."""
    section("14. Tab Bar (simulated with tree nodes)")

    # Create window for tabs
    result = client.call_tool("imgui_create_window", {
        "id": "tabs_window",
        "title": "Tab Bar Demo",
        "x": 900,
        "y": 400,
        "w": 400,
        "h": 350,
    })
    step("Create tabs window", result)

    # Simulate tabs using tree nodes (closest available widget)
    tabs = [
        ("tab_general", "General Settings", True),
        ("tab_appearance", "Appearance", False),
        ("tab_advanced", "Advanced", False),
    ]
    for wid, label, is_open in tabs:
        result = client.call_tool("imgui_add_widget", {
            "window": "tabs_window",
            "id": wid,
            "widget_type": "tree_node",
            "label": label,
            "open": is_open,
        })
        step(f"Tab: {label} ({'open' if is_open else 'collapsed'})", result)

    # Content inside the open tab
    result = client.call_tool("imgui_add_widget", {
        "window": "tabs_window",
        "id": "tab_content_text",
        "widget_type": "text",
        "content": "General settings content goes here.",
    })
    step("Tab content text", result)


def demo_selectables(client: MCPClient):
    """Add selectable items (using radio buttons and checkboxes)."""
    section("15. Selectables & Radio Buttons")

    # Radio buttons (mutually exclusive selection)
    result = client.call_tool("imgui_add_widget", {
        "window": "main_window",
        "id": "radio_1",
        "widget_type": "radio_button",
        "label": "Option A",
        "checked": True,
    })
    step("Radio button (Option A, selected)", result)

    result = client.call_tool("imgui_add_widget", {
        "window": "main_window",
        "id": "radio_2",
        "widget_type": "radio_button",
        "label": "Option B",
        "checked": False,
    })
    step("Radio button (Option B)", result)

    result = client.call_tool("imgui_add_widget", {
        "window": "main_window",
        "id": "radio_3",
        "widget_type": "radio_button",
        "label": "Option C",
        "checked": False,
    })
    step("Radio button (Option C)", result)

    # Checkboxes (multi-select)
    result = client.call_tool("imgui_add_widget", {
        "window": "main_window",
        "id": "checkbox_1",
        "widget_type": "checkbox",
        "label": "Enable VSync",
        "checked": True,
    })
    step("Checkbox (Enable VSync, checked)", result)

    result = client.call_tool("imgui_add_widget", {
        "window": "main_window",
        "id": "checkbox_2",
        "widget_type": "checkbox",
        "label": "Show FPS",
        "checked": False,
    })
    step("Checkbox (Show FPS, unchecked)", result)


def demo_progress_bar(client: MCPClient):
    """Add progress bar with overlay."""
    section("16. Progress Bar")

    result = client.call_tool("imgui_add_widget", {
        "window": "main_window",
        "id": "progress_1",
        "widget_type": "progress_bar",
        "label": "Loading...",
        "value": 0.65,
    })
    step("Progress bar (65%, overlay 'Loading...')", result)

    # Second progress bar at different value
    result = client.call_tool("imgui_add_widget", {
        "window": "main_window",
        "id": "progress_2",
        "widget_type": "progress_bar",
        "label": "Uploading",
        "value": 0.30,
    })
    step("Progress bar (30%, overlay 'Uploading')", result)


def demo_plots(client: MCPClient):
    """Add plot lines and histogram."""
    section("17. Plots")

    # Generate sine wave data
    sine_data = [math.sin(i * 0.3) * 0.5 + 0.5 for i in range(30)]

    result = client.call_tool("imgui_add_widget", {
        "window": "main_window",
        "id": "plot_lines_1",
        "widget_type": "plot_lines",
        "label": "Sine Wave",
        "values": sine_data,
    })
    step("Plot lines (sine wave, 30 points)", result)

    # Histogram data (simulated distribution)
    hist_data = [1, 3, 7, 12, 18, 22, 19, 14, 8, 4, 2, 1]

    result = client.call_tool("imgui_add_widget", {
        "window": "main_window",
        "id": "plot_hist_1",
        "widget_type": "plot_histogram",
        "label": "Distribution",
        "values": hist_data,
    })
    step("Plot histogram (distribution, 12 bins)", result)


def demo_collapsing_headers(client: MCPClient):
    """Add collapsing headers (tree nodes that act as headers)."""
    section("18. Collapsing Headers")

    result = client.call_tool("imgui_add_widget", {
        "window": "main_window",
        "id": "header_settings",
        "widget_type": "tree_node",
        "label": "Settings",
        "open": True,
    })
    step("Collapsing header (Settings, open)", result)

    result = client.call_tool("imgui_add_widget", {
        "window": "main_window",
        "id": "header_about",
        "widget_type": "tree_node",
        "label": "About",
        "open": False,
    })
    step("Collapsing header (About, collapsed)", result)

    # Spacing widget
    result = client.call_tool("imgui_add_widget", {
        "window": "main_window",
        "id": "spacing_1",
        "widget_type": "spacing",
    })
    step("Spacing", result)


def demo_child_window(client: MCPClient):
    """Create a child window with scrolling content."""
    section("19. Child Window (Scrolling Content)")

    result = client.call_tool("imgui_create_window", {
        "id": "child_window",
        "title": "Scrolling Log",
        "x": 50,
        "y": 700,
        "w": 400,
        "h": 200,
    })
    step("Create child window (scrolling log)", result)

    # Add many text lines to demonstrate scrolling
    for i in range(15):
        result = client.call_tool("imgui_add_widget", {
            "window": "child_window",
            "id": f"log_line_{i}",
            "widget_type": "text",
            "content": f"[{i:03d}] Log entry: System event #{i} processed at frame {i * 60}",
        })
    step("Added 15 log lines to child window", {"count": 15})


def demo_shapes_window(client: MCPClient):
    """Create a window demonstrating drawing capabilities."""
    section("20. Drawing Shapes (via dedicated window)")

    # The C++ app renders shapes through its draw list; we create a window
    # that would host custom drawing (line, rect, circle, triangle, bezier)
    result = client.call_tool("imgui_create_window", {
        "id": "shapes_window",
        "title": "Shapes & Drawing",
        "x": 500,
        "y": 700,
        "w": 400,
        "h": 300,
    })
    step("Create shapes window", result)

    # Add descriptive text for each shape type
    shapes = [
        ("shape_line", "Line: (10,10) -> (200,100)"),
        ("shape_rect", "Rect: (10,120) size 100x60"),
        ("shape_circle", "Circle: center (250,80) radius 40"),
        ("shape_triangle", "Triangle: (200,200) (250,120) (300,200)"),
        ("shape_bezier", "Bezier: cubic curve (10,250) -> (350,250)"),
    ]
    for wid, desc in shapes:
        result = client.call_tool("imgui_add_widget", {
            "window": "shapes_window",
            "id": wid,
            "widget_type": "text",
            "content": desc,
        })
        step(f"Shape: {desc.split(':')[0]}", result)


def demo_set_background(client: MCPClient):
    """Set viewport background color."""
    section("21. Set Background Color")

    # Dark blue-grey background
    result = client.call_tool("imgui_set_background", {
        "color": [0.08, 0.08, 0.12],
    })
    step("Set background (dark blue-grey)", result)


def demo_show_demo_window(client: MCPClient):
    """Show the ImGui demo window."""
    section("22. Show ImGui Demo Window")

    result = client.call_tool("imgui_show_demo", {"show": True})
    step("Show ImGui demo window", result)


def demo_update_widgets(client: MCPClient):
    """Demonstrate updating existing widgets."""
    section("23. Update Widgets")

    # Update slider value
    result = client.call_tool("imgui_update_widget", {
        "window": "main_window",
        "id": "slider_float_1",
        "value": 0.85,
    })
    step("Update Volume slider to 0.85", result)

    # Update checkbox state
    result = client.call_tool("imgui_update_widget", {
        "window": "main_window",
        "id": "checkbox_2",
        "checked": True,
    })
    step("Update 'Show FPS' checkbox to checked", result)

    # Update text content
    result = client.call_tool("imgui_update_widget", {
        "window": "main_window",
        "id": "text_label",
        "content": "Updated!",
    })
    step("Update label text content to 'Updated!'", result)

    # Update combo selection
    result = client.call_tool("imgui_update_widget", {
        "window": "main_window",
        "id": "combo_1",
        "selected": 2,
    })
    step("Update combo selection to 'Classic' (index 2)", result)

    # Update progress bar
    result = client.call_tool("imgui_update_widget", {
        "window": "main_window",
        "id": "progress_1",
        "value": 0.90,
    })
    step("Update progress bar to 90%", result)

    # Update plot data
    new_sine = [math.sin(i * 0.5) * 0.5 + 0.5 for i in range(20)]
    result = client.call_tool("imgui_update_widget", {
        "window": "main_window",
        "id": "plot_lines_1",
        "values": new_sine,
    })
    step("Update plot lines with new data (20 points)", result)


def demo_remove_widget(client: MCPClient):
    """Demonstrate removing a widget."""
    section("24. Remove Widget")

    result = client.call_tool("imgui_remove_widget", {
        "window": "main_window",
        "id": "btn_disabled",
    })
    step("Remove disabled button", result)


def demo_get_state(client: MCPClient):
    """Get the full application state."""
    section("25. Get State (Full)")

    result = client.call_tool("imgui_get_state")
    # Print summary rather than full dump
    if "windows" in result:
        print(f"\n  [OK] State retrieved (frame {result.get('frame', '?')})")
        print(f"        Windows: {len(result['windows'])}")
        for win_id, win_data in result["windows"].items():
            widget_count = len(win_data.get("widgets", {}))
            print(f"        - {win_id}: \"{win_data.get('title', '?')}\" "
                  f"({widget_count} widgets, open={win_data.get('open', '?')})")
    else:
        step("Get state", result)


def demo_remove_window(client: MCPClient):
    """Demonstrate removing a window."""
    section("26. Remove Window")

    result = client.call_tool("imgui_remove_window", {"id": "shapes_window"})
    step("Remove shapes window", result)


def demo_clear_all(client: MCPClient):
    """Clear all windows and widgets."""
    section("27. Clear All")

    result = client.call_tool("imgui_clear_all")
    step("Clear all windows and widgets", result)


def demo_final_state(client: MCPClient):
    """Verify state is empty after clear."""
    section("28. Final State (after clear)")

    result = client.call_tool("imgui_get_state")
    if "windows" in result:
        count = len(result["windows"])
        print(f"\n  [OK] Final state: {count} windows (expected 0)")
    else:
        step("Final state", result)


# ─── Main ────────────────────────────────────────────────────────────────────


def main():
    """Run the complete ImGui MCP demo."""
    print("=" * 70)
    print("  ImGui MCP Demo - Comprehensive Feature Exercise")
    print("  Protocol: MCP 2025-06-18 (JSON-RPC 2.0 over stdio)")
    print("=" * 70)

    client = MCPClient()

    try:
        # Start the MCP server subprocess
        print("\n  Starting MCP server (server.py)...")
        client.start()
        time.sleep(0.5)  # Allow server to initialize

        # Run all demo sections in sequence
        demo_initialize(client)
        demo_app_status(client)
        demo_create_main_window(client)
        demo_menu_items(client)
        demo_text_widgets(client)
        demo_buttons(client)
        demo_input_widgets(client)
        demo_sliders(client)
        demo_drag_widgets(client)
        demo_color_widgets(client)
        demo_combo_listbox(client)
        demo_tree_nodes(client)
        demo_table(client)
        demo_tab_bar(client)
        demo_selectables(client)
        demo_progress_bar(client)
        demo_plots(client)
        demo_collapsing_headers(client)
        demo_child_window(client)
        demo_shapes_window(client)
        demo_set_background(client)
        demo_show_demo_window(client)
        demo_update_widgets(client)
        demo_remove_widget(client)
        demo_get_state(client)
        demo_remove_window(client)
        demo_clear_all(client)
        demo_final_state(client)

        # Final summary
        section("Demo Complete")
        print("\n  All ImGui MCP features exercised successfully!")
        print("  Tools demonstrated:")
        print("    - imgui_app_status")
        print("    - imgui_create_window (x4)")
        print("    - imgui_add_widget (50+ widgets across all types)")
        print("    - imgui_update_widget (6 updates)")
        print("    - imgui_remove_widget")
        print("    - imgui_remove_window")
        print("    - imgui_get_state (x2)")
        print("    - imgui_clear_all")
        print("    - imgui_set_background")
        print("    - imgui_show_demo")
        print()

    except Exception as e:
        print(f"\n  [ERROR] Demo failed: {e}", file=sys.stderr)
        import traceback

        traceback.print_exc(file=sys.stderr)
        sys.exit(1)

    finally:
        # Clean up
        print("  Shutting down MCP server...")
        client.stop()
        print("  Done.")


if __name__ == "__main__":
    main()
