#!/usr/bin/env python3
"""Unit tests for the Python MCP bridge."""

from __future__ import annotations

import asyncio
import io
import json
import sys
import tempfile
import unittest
from pathlib import Path
from unittest import mock

from mcp import ClientSession, StdioServerParameters
from mcp.client.stdio import stdio_client

import server


class _FakeStdin(io.StringIO):
    def flush(self) -> None:
        pass


class _FakeProcess:
    def __init__(self, stdout: str = "") -> None:
        self.stdin = _FakeStdin()
        self.stdout = io.StringIO(stdout)
        self.stderr = io.StringIO()

    def poll(self):
        return None


class _BinaryInput:
    def __init__(self, data: bytes) -> None:
        self.buffer = io.BytesIO(data)


class _BinaryOutput:
    def __init__(self, data: bytes) -> None:
        self.buffer = io.BytesIO(data)


class ImGuiAppTests(unittest.TestCase):
    def test_events_cannot_steal_command_responses(self) -> None:
        event = {"type": "event", "widget": "button", "event": "clicked"}
        ack = {"type": "ack", "cmd": "get_state"}
        output = "\n".join((json.dumps(event), json.dumps(ack), ""))

        app = server.ImGuiApp()
        app.proc = _FakeProcess(output)
        app._running = True
        app._read_stdout()

        self.assertEqual(ack, app.send_command({"cmd": "get_state"}))
        self.assertEqual([event], app.drain_events())

    def test_same_directory_binary_is_detected(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            expected = Path(temp_dir) / "imgui_mcp_app"
            expected.touch()
            with mock.patch.object(server, "SCRIPT_DIR", Path(temp_dir)):
                self.assertEqual(expected, server._find_binary())

    def test_binary_override_is_honored(self) -> None:
        expected = Path("/tmp/imgui-mcp-windows.exe")
        with mock.patch.dict(
            server.os.environ, {"IMGUI_MCP_APP_BINARY": str(expected)}
        ):
            self.assertEqual(expected, server._find_binary())

    def test_screenshot_waits_for_terminal_completion(self) -> None:
        ack = {"type": "ack", "cmd": "screenshot"}
        screenshot = {"type": "screenshot", "cmd": "screenshot", "path": "shot.bmp"}
        output = "\n".join((json.dumps(ack), json.dumps(screenshot), ""))

        app = server.ImGuiApp()
        app.proc = _FakeProcess(output)
        app._running = True
        app._read_stdout()

        self.assertEqual(screenshot, app.send_command({"cmd": "screenshot"}))

    def test_stale_command_response_is_ignored(self) -> None:
        stale = {"type": "ack", "cmd": "create_window"}
        state = {"type": "state", "cmd": "get_state", "windows": {}}
        output = "\n".join((json.dumps(stale), json.dumps(state), ""))

        app = server.ImGuiApp()
        app.proc = _FakeProcess(output)
        app._running = True
        app._read_stdout()

        self.assertEqual(state, app.send_command({"cmd": "get_state"}))

    def test_request_id_rejects_a_stale_response_for_the_same_command(self) -> None:
        stale = {"type": "state", "cmd": "get_state", "request_id": 999}
        current = {"type": "state", "cmd": "get_state", "request_id": 1, "windows": {}}
        output = "\n".join((json.dumps(stale), json.dumps(current), ""))

        app = server.ImGuiApp()
        app.proc = _FakeProcess(output)
        app._running = True
        app._read_stdout()

        self.assertEqual(current, app.send_command({"cmd": "get_state"}))
        sent = json.loads(app.proc.stdin.getvalue())
        self.assertEqual(1, sent["request_id"])

    def test_oversized_native_command_is_rejected_before_write(self) -> None:
        app = server.ImGuiApp()
        app.proc = _FakeProcess()
        app._running = True

        response = app.send_command(
            {"cmd": "add_widget", "content": "x" * server.MAX_PROTOCOL_BYTES}
        )

        self.assertEqual("error", response["type"])
        self.assertIn("4 MiB", response["message"])
        self.assertEqual("", app.proc.stdin.getvalue())

    def test_event_backlog_is_bounded_to_the_most_recent_events(self) -> None:
        events = [
            {"type": "event", "widget": f"button-{index}", "event": "clicked"}
            for index in range(server.MAX_PENDING_EVENTS + 2)
        ]
        output = "\n".join((*map(json.dumps, events), ""))

        app = server.ImGuiApp()
        app.proc = _FakeProcess(output)
        app._running = True
        app._read_stdout()

        batch = app.drain_event_batch(server.MAX_PENDING_EVENTS)
        self.assertEqual(server.MAX_PENDING_EVENTS, batch["returned"])
        self.assertEqual(2, batch["dropped"])
        self.assertEqual("button-2", batch["events"][0]["widget"])
        self.assertEqual(0, app.drain_event_batch(1)["dropped"])

    def test_event_backlog_rejects_an_event_larger_than_its_byte_budget(self) -> None:
        oversized = {
            "type": "event",
            "widget": "too-large",
            "event": "changed",
            "payload": "x" * server.MAX_PENDING_EVENT_BYTES,
        }
        retained = {"type": "event", "widget": "kept", "event": "changed"}

        app = server.ImGuiApp()
        app.proc = _FakeProcess("\n".join((json.dumps(oversized), json.dumps(retained), "")))
        app._running = True
        app._read_stdout()

        batch = app.drain_event_batch(server.MAX_PENDING_EVENTS)
        self.assertEqual([retained], batch["events"])
        self.assertEqual(1, batch["dropped"])
        self.assertEqual(0, batch["pending"])

    def test_oversized_native_frame_is_bounded_and_reader_recovers(self) -> None:
        valid = json.dumps({"type": "ack", "cmd": "get_state"})
        output = (
            b"x" * (server.MAX_NATIVE_RESPONSE_BYTES + 1)
            + b"\n"
            + valid.encode()
            + b"\n"
        )

        frames = list(server._iter_native_output(_BinaryOutput(output)))

        self.assertEqual((None, server.NATIVE_FRAME_LIMIT_ERROR), frames[0])
        self.assertEqual((valid, None), frames[1])

    def test_native_frame_above_command_limit_is_accepted(self) -> None:
        payload = b"x" * (server.MAX_PROTOCOL_BYTES + 1)

        frames = list(server._iter_native_output(_BinaryOutput(payload + b"\r\n")))

        self.assertEqual(1, len(frames))
        self.assertIsNone(frames[0][1])
        self.assertEqual(len(payload), len(frames[0][0]))

    def test_native_response_backlog_is_bounded_and_fails_current_command(self) -> None:
        responses = [
            {"type": "ack", "cmd": "one"},
            {"type": "ack", "cmd": "two"},
            {"type": "ack", "cmd": "three"},
        ]
        output = "\n".join((*map(json.dumps, responses), ""))

        app = server.ImGuiApp()
        app.proc = _FakeProcess(output)
        app._running = True
        app._read_stdout()

        self.assertLessEqual(app.responses.qsize(), server.MAX_PENDING_RESPONSES)
        response = app.send_command({"cmd": "get_state"})
        self.assertEqual("error", response["type"])
        self.assertEqual("get_state", response["cmd"])
        self.assertEqual(server.NATIVE_RESPONSE_QUEUE_ERROR, response["message"])
        self.assertEqual(0, app.responses.qsize())

    def test_malformed_or_deep_native_json_becomes_a_local_error(self) -> None:
        deeply_nested = '{"type":"ack","value":' + ("[" * 10_000) + "0" + ("]" * 10_000) + "}"
        for payload in ("{not-json", deeply_nested):
            with self.subTest(payload="deep" if payload == deeply_nested else "malformed"):
                app = server.ImGuiApp()
                app.proc = _FakeProcess(payload + "\n")
                app._running = True
                app._read_stdout()

                response = app.send_command({"cmd": "get_state"})
                self.assertEqual("error", response["type"])
                self.assertEqual("get_state", response["cmd"])
                self.assertEqual(server.NATIVE_JSON_ERROR, response["message"])

    def test_non_finite_native_command_is_rejected_before_write(self) -> None:
        app = server.ImGuiApp()
        app.proc = _FakeProcess()
        app._running = True

        response = app.send_command({"cmd": "add_widget", "value": float("nan")})

        self.assertEqual("error", response["type"])
        self.assertIn("JSON compliant", response["message"])
        self.assertEqual("", app.proc.stdin.getvalue())


class MCPServerTests(unittest.TestCase):
    def test_tool_catalog_is_unique_and_complete(self) -> None:
        names = [tool["name"] for tool in server.TOOLS]
        self.assertEqual(84, len(names))
        self.assertEqual(len(names), len(set(names)))

    def test_widget_color_schema_accepts_rgb_and_rgba(self) -> None:
        schema = server.TOOLS_BY_NAME["imgui_add_widget"]["inputSchema"]
        base = {"window": "w", "id": "color", "widget_type": "color_edit3"}

        server._validate_schema({**base, "color": [0.1, 0.2, 0.3]}, schema)
        server._validate_schema({**base, "color": [0.1, 0.2, 0.3, 0.4]}, schema)
        with self.assertRaisesRegex(ValueError, "too few items"):
            server._validate_schema({**base, "color": [0.1, 0.2]}, schema)

    def _test_legacy_invalid_messages_return_errors_and_server_keeps_running(self) -> None:
        requests = "\n".join(
            (
                "[]",
                json.dumps({"jsonrpc": "2.0", "id": 1, "method": "tools/call", "params": []}),
                json.dumps({"jsonrpc": "2.0", "id": 2, "method": "ping"}),
                "",
            )
        )
        output = io.StringIO()

        with mock.patch("sys.stdin", io.StringIO(requests)), mock.patch("sys.stdout", output):
            server.MCPServer().run()

        messages = [json.loads(line) for line in output.getvalue().splitlines()]
        self.assertEqual(-32600, messages[0]["error"]["code"])
        self.assertEqual(-32602, messages[1]["error"]["code"])
        self.assertEqual({"jsonrpc": "2.0", "id": 2, "result": {}}, messages[2])

    def _test_legacy_mcp_input_is_byte_bounded_and_recovers_on_the_next_line(self) -> None:
        ping = json.dumps({"jsonrpc": "2.0", "id": 3, "method": "ping"}).encode()
        requests = (
            b"x" * (server.MAX_PROTOCOL_BYTES + 1)
            + b"\n"
            + b"\xff\n"
            + ping
            + b"\n"
        )
        output = io.StringIO()

        with mock.patch("sys.stdin", _BinaryInput(requests)), mock.patch(
            "sys.stdout", output
        ):
            server.MCPServer().run()

        messages = [json.loads(line) for line in output.getvalue().splitlines()]
        self.assertEqual(-32600, messages[0]["error"]["code"])
        self.assertIn("4 MiB", messages[0]["error"]["message"])
        self.assertEqual(-32700, messages[1]["error"]["code"])
        self.assertEqual({"jsonrpc": "2.0", "id": 3, "result": {}}, messages[2])

    def _test_legacy_non_finite_mcp_json_is_rejected_and_next_request_succeeds(self) -> None:
        requests = "\n".join(
            (
                '{"jsonrpc":"2.0","id":3,"method":"ping","params":{"x":NaN}}',
                json.dumps({"jsonrpc": "2.0", "id": 4, "method": "ping"}),
                "",
            )
        )
        output = io.StringIO()

        with mock.patch("sys.stdin", io.StringIO(requests)), mock.patch(
            "sys.stdout", output
        ):
            server.MCPServer().run()

        messages = [json.loads(line) for line in output.getvalue().splitlines()]
        self.assertEqual(-32700, messages[0]["error"]["code"])
        self.assertIn("non-finite", messages[0]["error"]["message"])
        self.assertEqual({"jsonrpc": "2.0", "id": 4, "result": {}}, messages[1])

    def _test_legacy_pathological_mcp_json_is_rejected_and_ping_still_succeeds(self) -> None:
        giant_integer = '{"jsonrpc":"2.0","id":' + ("9" * 10_000) + ',"method":"ping"}'
        deeply_nested = (
            '{"jsonrpc":"2.0","id":2,"method":"ping","params":{"value":'
            + ("[" * 10_000)
            + "0"
            + ("]" * 10_000)
            + "}}"
        )
        ping = json.dumps({"jsonrpc": "2.0", "id": 3, "method": "ping"})
        output = io.StringIO()

        with mock.patch("sys.stdin", io.StringIO("\n".join((giant_integer, deeply_nested, ping, "")))), mock.patch(
            "sys.stdout", output
        ):
            server.MCPServer().run()

        messages = [json.loads(line) for line in output.getvalue().splitlines()]
        self.assertEqual([-32700, -32700], [message["error"]["code"] for message in messages[:2]])
        self.assertEqual({"jsonrpc": "2.0", "id": 3, "result": {}}, messages[2])

    def test_native_errors_are_mcp_tool_errors(self) -> None:
        mcp = server.MCPServer()
        mcp.app = mock.Mock()
        mcp.app.is_running.return_value = True
        mcp.app.send_command.return_value = {
            "type": "error",
            "cmd": "create_window",
            "message": "invalid flags",
        }
        response = asyncio.run(
            mcp._handle_sdk_tool(
                "imgui_create_window", {"id": "x", "title": "X"}
            )
        )

        self.assertTrue(response.isError)
        content = json.loads(response.content[0].text)
        self.assertEqual("invalid flags", content["message"])

    def test_tool_arguments_are_validated_before_native_dispatch(self) -> None:
        mcp = server.MCPServer()
        mcp.app = mock.Mock()
        response = asyncio.run(
            mcp._handle_sdk_tool(
                "imgui_create_window", {"id": "missing-title"}
            )
        )

        self.assertTrue(response.isError)
        self.assertIn("title is required", response.content[0].text)
        mcp.app.send_command.assert_not_called()

    def test_undeclared_opcode_argument_cannot_override_native_command(self) -> None:
        mcp = server.MCPServer()
        mcp.app = mock.Mock()
        response = asyncio.run(
            mcp._handle_sdk_tool(
                "imgui_add_widget",
                {
                    "window": "w",
                    "id": "label",
                    "widget_type": "text",
                    "cmd": "quit",
                },
            )
        )

        self.assertTrue(response.isError)
        self.assertIn("cmd is not allowed", response.content[0].text)
        mcp.app.send_command.assert_not_called()

        mcp.app.is_running.return_value = True
        mcp.app.send_command.return_value = {"type": "ack"}
        mcp._call_tool(
            "imgui_add_widget",
            {"window": "w", "id": "label", "widget_type": "text", "cmd": "quit"},
        )
        self.assertEqual("add_widget", mcp.app.send_command.call_args.args[0]["cmd"])

    def test_event_drain_is_available_as_an_mcp_tool(self) -> None:
        mcp = server.MCPServer()
        mcp.app = mock.Mock()
        expected = {
            "events": [{"type": "event", "event": "clicked"}],
            "returned": 1,
            "pending": 2,
            "dropped": 3,
        }
        mcp.app.drain_event_batch.return_value = expected

        self.assertEqual(expected, mcp._call_tool("imgui_drain_events", {"max_events": 7}))
        mcp.app.drain_event_batch.assert_called_once_with(7)

    def test_bridge_translates_documented_contracts(self) -> None:
        mcp = server.MCPServer()
        mcp.app = mock.Mock()
        mcp.app.is_running.return_value = True
        mcp.app.send_command.return_value = {"type": "ack"}

        mcp._call_tool(
            "imgui_create_window",
            {
                "id": "w",
                "title": "Window",
                "flags": ["menubar", "no_resize"],
            },
        )
        self.assertEqual(
            {
                "cmd": "create_window",
                "id": "w",
                "title": "Window",
                "flags": (1 << 10) | (1 << 1),
            },
            mcp.app.send_command.call_args.args[0],
        )

        mcp._call_tool(
            "imgui_window_control",
            {"id": "w", "action": "set_pos", "value": [25, 50]},
        )
        self.assertEqual(
            {"cmd": "window_control", "id": "w", "action": "set_pos", "values": [25, 50]},
            mcp.app.send_command.call_args.args[0],
        )

        mcp._call_tool("imgui_clipboard", {"action": "set", "text": "hello"})
        self.assertEqual(
            {"cmd": "clipboard_set", "text": "hello"},
            mcp.app.send_command.call_args.args[0],
        )

        mcp._call_tool(
            "imgui_load_font",
            {
                "id": "ui",
                "path": "/fonts/ui.ttf",
                "size_pixels": 18,
                "glyph_ranges": "symbols",
                "merge_into": "body",
            },
        )
        self.assertEqual(
            {
                "cmd": "load_font",
                "id": "ui",
                "path": "/fonts/ui.ttf",
                "size_pixels": 18,
                "glyph_ranges": "symbols",
                "merge_into": "body",
            },
            mcp.app.send_command.call_args.args[0],
        )

        mcp._call_tool("imgui_set_font", {"id": "ui"})
        self.assertEqual(
            {"cmd": "set_font", "id": "ui"},
            mcp.app.send_command.call_args.args[0],
        )

        mcp._call_tool(
            "imgui_draw",
            {
                "window": "scene",
                "id": "foreground",
                "mode": "replace",
                "commands": [{"type": "ellipse", "p1": [10, 10], "p2": [5, 3]}],
            },
        )
        self.assertEqual(
            {
                "cmd": "draw",
                "window": "scene",
                "id": "foreground",
                "mode": "replace",
                "commands": [{"type": "ellipse", "p1": [10, 10], "p2": [5, 3]}],
            },
            mcp.app.send_command.call_args.args[0],
        )

        mcp._call_tool("imgui_list_fonts", {})
        self.assertEqual(
            {"cmd": "list_fonts"}, mcp.app.send_command.call_args.args[0]
        )


class OfficialSDKIntegrationTests(unittest.IsolatedAsyncioTestCase):
    async def test_stdio_handshake_lists_tools_and_calls_status(self) -> None:
        params = StdioServerParameters(
            command=sys.executable,
            args=[str(server.SCRIPT_DIR / "server.py")],
            cwd=str(server.SCRIPT_DIR),
        )
        async with stdio_client(params) as (read_stream, write_stream):
            async with ClientSession(read_stream, write_stream) as session:
                initialized = await session.initialize()
                tools = await session.list_tools()
                status = await session.call_tool("imgui_app_status", {})

        self.assertEqual("imgui-mcp", initialized.serverInfo.name)
        self.assertEqual(84, len(tools.tools))
        self.assertEqual("imgui_create_window", tools.tools[0].name)
        self.assertFalse(status.isError)
        self.assertEqual(server.SERVER_VERSION, status.structuredContent["version"])


if __name__ == "__main__":
    unittest.main()
