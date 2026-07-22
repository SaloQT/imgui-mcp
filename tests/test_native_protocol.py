#!/usr/bin/env python3
"""Integration tests for the native newline-delimited JSON protocol."""

from __future__ import annotations

import json
import os
import select
import shutil
import struct
import subprocess
import tempfile
import time
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
APP = Path(os.environ.get("IMGUI_MCP_APP", ROOT / "build" / "imgui_mcp_app"))
REQUIRE_NATIVE = os.environ.get("IMGUI_MCP_REQUIRE_NATIVE") == "1"


def unavailable(reason: str) -> None:
    if REQUIRE_NATIVE:
        raise RuntimeError(reason)
    raise unittest.SkipTest(reason)


class NativeApp:
    def __init__(self) -> None:
        if not APP.is_file():
            unavailable(f"native app is not built: {APP}")

        command = [str(APP), "--width", "320", "--height", "240"]
        if not os.environ.get("DISPLAY"):
            xvfb_run = shutil.which("xvfb-run")
            if not xvfb_run:
                unavailable("DISPLAY and xvfb-run are unavailable")
            command = [xvfb_run, "-a", *command]

        self.proc = subprocess.Popen(
            command,
            cwd=ROOT,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            bufsize=1,
        )

    def send(self, message: dict) -> None:
        assert self.proc.stdin is not None
        self.proc.stdin.write(json.dumps(message, separators=(",", ":")) + "\n")
        self.proc.stdin.flush()

    def close_stdin(self) -> None:
        if self.proc.stdin and not self.proc.stdin.closed:
            self.proc.stdin.close()

    def read_json(self, timeout: float = 5.0) -> dict:
        assert self.proc.stdout is not None
        deadline = time.monotonic() + timeout
        while True:
            remaining = deadline - time.monotonic()
            if remaining <= 0:
                raise TimeoutError("timed out waiting for native JSON output")
            readable, _, _ = select.select([self.proc.stdout], [], [], remaining)
            if not readable:
                raise TimeoutError("timed out waiting for native JSON output")
            line = self.proc.stdout.readline()
            if not line:
                stderr = self._stderr_after_exit()
                raise RuntimeError(
                    f"native app closed stdout (exit={self.proc.poll()}): {stderr}"
                )
            try:
                return json.loads(line)
            except json.JSONDecodeError:
                # xvfb-run/XKB can occasionally emit a diagnostic on stdout.
                continue

    def read_until_exit(self, timeout: float = 5.0) -> list[dict]:
        messages: list[dict] = []
        deadline = time.monotonic() + timeout
        while self.proc.poll() is None and time.monotonic() < deadline:
            try:
                messages.append(self.read_json(min(0.25, deadline - time.monotonic())))
            except TimeoutError:
                continue
            except RuntimeError:
                break
        self.proc.wait(timeout=max(0.1, deadline - time.monotonic()))
        assert self.proc.stdout is not None
        for line in self.proc.stdout:
            try:
                messages.append(json.loads(line))
            except json.JSONDecodeError:
                continue
        return messages

    def _stderr_after_exit(self) -> str:
        if self.proc.poll() is None or self.proc.stderr is None:
            return ""
        return self.proc.stderr.read().strip()

    def cleanup(self) -> None:
        self.close_stdin()
        if self.proc.poll() is None:
            try:
                self.proc.wait(timeout=2)
            except subprocess.TimeoutExpired:
                self.proc.kill()
                self.proc.wait(timeout=2)
        if self.proc.stdout:
            self.proc.stdout.close()
        if self.proc.stderr:
            self.proc.stderr.close()


class NativeProtocolTests(unittest.TestCase):
    def setUp(self) -> None:
        self.app = NativeApp()
        ready = self.app.read_json()
        self.assertEqual("ready", ready.get("type"))

    def tearDown(self) -> None:
        self.app.cleanup()

    def test_drains_queued_commands_before_stdin_eof(self) -> None:
        self.app.send({"cmd": "create_window", "id": "batch", "title": "Batch"})
        self.app.send({"cmd": "get_state"})
        self.app.send({"cmd": "quit"})
        self.app.close_stdin()

        messages = self.app.read_until_exit()
        self.assertTrue(
            any(m.get("type") == "ack" and m.get("cmd") == "create_window" for m in messages),
            messages,
        )
        states = [m for m in messages if m.get("type") == "state"]
        self.assertEqual(1, len(states), messages)
        self.assertIn("batch", states[0]["windows"])
        self.assertTrue(
            any(m.get("type") == "ack" and m.get("cmd") == "quit" for m in messages),
            messages,
        )
        self.assertTrue(any(m.get("type") == "shutdown" for m in messages), messages)

    def test_documented_window_flags_and_graceful_quit(self) -> None:
        self.app.send(
            {
                "cmd": "create_window",
                "id": "flagged",
                "title": "Flagged",
                "flags": ["menubar", "no_resize"],
            }
        )
        response = self.app.read_json()
        self.assertEqual("ack", response.get("type"), response)
        self.assertEqual("create_window", response.get("cmd"), response)

        self.app.send({"cmd": "get_state"})
        state = self.app.read_json()
        self.assertEqual((1 << 10) | (1 << 1), state["windows"]["flagged"]["flags"])

        self.app.send({"cmd": "quit"})
        response = self.app.read_json()
        self.assertEqual("ack", response.get("type"), response)
        self.assertEqual("quit", response.get("cmd"), response)
        self.app.proc.wait(timeout=2)
        self.assertEqual(0, self.app.proc.returncode)

    def test_internal_window_flags_are_rejected_without_killing_app(self) -> None:
        for flags in (-1, 1 << 24):
            self.app.send(
                {
                    "cmd": "create_window",
                    "id": f"invalid-{flags}",
                    "title": "Invalid",
                    "flags": flags,
                }
            )
            response = self.app.read_json()
            self.assertEqual("error", response.get("type"), response)
            self.assertIn("unsupported", response.get("message", ""), response)

        self.app.send({"cmd": "get_state"})
        self.assertEqual("state", self.app.read_json().get("type"))

    def test_bad_command_returns_error_without_killing_app(self) -> None:
        self.app.send({"cmd": "create_window", "id": "bad", "w": "wide"})
        response = self.app.read_json()
        self.assertEqual("error", response.get("type"), response)

        self.app.send({"cmd": "get_state"})
        response = self.app.read_json()
        self.assertEqual("state", response.get("type"), response)

    def test_non_object_command_returns_error_without_killing_app(self) -> None:
        self.app.send([])  # type: ignore[arg-type]
        response = self.app.read_json()
        self.assertEqual("error", response.get("type"), response)
        self.assertIn("object", response.get("message", ""), response)

        self.app.send({"cmd": "get_state"})
        self.assertEqual("state", self.app.read_json().get("type"))

    def test_deep_native_command_is_rejected_and_app_stays_live(self) -> None:
        raw_command = '{"cmd":"get_state","nested":' + ("[" * 10_000) + "0" + ("]" * 10_000) + "}"
        self.assertLess(len(raw_command.encode("utf-8")), 4 * 1024 * 1024)
        assert self.app.proc.stdin is not None
        self.app.proc.stdin.write(raw_command + "\n")
        self.app.proc.stdin.flush()

        response = self.app.read_json(timeout=2.0)
        self.assertEqual("error", response.get("type"), response)

        self.app.send({"cmd": "get_state"})
        self.assertEqual("state", self.app.read_json().get("type"))

    def test_request_ids_are_echoed(self) -> None:
        self.app.send({"cmd": "get_state", "request_id": "request-7"})
        response = self.app.read_json()
        self.assertEqual("request-7", response.get("request_id"), response)

    def test_large_command_is_not_split(self) -> None:
        content = "x" * 100_000
        self.app.send(
            {
                "cmd": "create_window",
                "id": "large",
                "title": "Large",
            }
        )
        self.assertEqual("ack", self.app.read_json().get("type"))
        self.app.send(
            {
                "cmd": "add_widget",
                "window": "large",
                "id": "large_text",
                "widget_type": "text",
                "content": content,
            }
        )
        self.assertEqual("ack", self.app.read_json().get("type"))
        self.app.send({"cmd": "get_state"})
        state = self.app.read_json()
        self.assertIn("large_text", state["windows"]["large"]["widgets"])

    def test_oversized_command_is_rejected_without_exhausting_or_killing_app(self) -> None:
        self.app.send(
            {
                "cmd": "create_window",
                "id": "oversized",
                "title": "x" * (4 * 1024 * 1024),
            }
        )
        response = self.app.read_json(timeout=10)
        self.assertEqual("error", response.get("type"), response)
        self.assertIn("4 MiB", response.get("message", ""), response)

        self.app.send({"cmd": "get_state"})
        self.assertEqual("state", self.app.read_json().get("type"))

    def test_unsafe_numeric_format_and_style_underflow_are_contained(self) -> None:
        self.app.send({"cmd": "create_window", "id": "safe", "title": "Safe"})
        self.assertEqual("ack", self.app.read_json().get("type"))
        for index, unsafe_format in enumerate(
            ("%s", "%.1000000000f", "%1000000000.2f", "%" + "1" * 129 + "f")
        ):
            self.app.send(
                {
                    "cmd": "add_widget",
                    "window": "safe",
                    "id": f"slider-{index}",
                    "widget_type": "slider_float",
                    "format": unsafe_format,
                }
            )
            response = self.app.read_json()
            self.assertEqual("error", response.get("type"), response)

        self.app.send({"cmd": "pop_style_color", "count": 1})
        response = self.app.read_json()
        self.assertEqual("error", response.get("type"), response)

        self.app.send({"cmd": "push_style_color", "col_idx": 999, "color": [1, 1, 1, 1]})
        response = self.app.read_json()
        self.assertEqual("error", response.get("type"), response)

        self.app.send({"cmd": "get_state"})
        self.assertEqual("state", self.app.read_json().get("type"))

    def test_import_window_quota_is_transactional_and_app_survives(self) -> None:
        windows = [
            {"id": f"window-{index}", "title": f"Window {index}"}
            for index in range(257)
        ]
        self.app.send({"cmd": "import_json", "json": {"windows": windows}})
        response = self.app.read_json()
        self.assertEqual("error", response.get("type"), response)
        self.assertIn("256", response.get("message", ""), response)

        self.app.send({"cmd": "get_state"})
        state = self.app.read_json()
        self.assertEqual("state", state.get("type"), state)
        self.assertEqual({}, state["windows"])

    def test_rejected_widget_does_not_consume_undo_history(self) -> None:
        self.app.send({"cmd": "create_window", "id": "undo", "title": "Undo"})
        self.assertEqual("ack", self.app.read_json().get("type"))
        self.app.send(
            {
                "cmd": "add_widget",
                "window": "undo",
                "id": "valid",
                "widget_type": "text",
                "content": "kept until undo",
            }
        )
        self.assertEqual("ack", self.app.read_json().get("type"))
        self.app.send(
            {
                "cmd": "add_widget",
                "window": "undo",
                "id": "invalid",
                "widget_type": "slider_float",
                "format": "%s",
            }
        )
        self.assertEqual("error", self.app.read_json().get("type"))

        self.app.send({"cmd": "undo"})
        self.assertEqual("ack", self.app.read_json().get("type"))
        self.app.send({"cmd": "get_state"})
        state = self.app.read_json()
        self.assertEqual({}, state["windows"]["undo"]["widgets"])

    def test_exported_tooltip_uses_a_fixed_format_string(self) -> None:
        payload = '%n%s%p "quoted" \\ slash\nline'
        self.app.send({"cmd": "create_window", "id": "export", "title": "Export"})
        self.assertEqual("ack", self.app.read_json().get("type"))
        self.app.send(
            {
                "cmd": "add_widget",
                "window": "export",
                "id": "tip",
                "widget_type": "tooltip",
                "content": payload,
            }
        )
        self.assertEqual("ack", self.app.read_json().get("type"))
        self.app.send({"cmd": "export_cpp"})
        exported = self.app.read_json()
        self.assertEqual("export", exported.get("type"), exported)
        self.assertIn('ImGui::SetTooltip("%s", "', exported["code"])
        self.assertNotIn('ImGui::SetTooltip("%n', exported["code"])

    def test_cpp_and_lua_exports_escape_control_character_injection(self) -> None:
        id_marker = "id_escape_breakout"
        title_marker = "title_escape_breakout"
        unsafe_id = f"window\n{id_marker}();\r\x01"
        unsafe_title = f'Title"\n{title_marker}();\r\x02'

        self.app.send({"cmd": "create_window", "id": unsafe_id, "title": unsafe_title})
        self.assertEqual("ack", self.app.read_json().get("type"))

        for command in ("export_cpp", "export_lua"):
            self.app.send({"cmd": command})
            exported = self.app.read_json()
            self.assertEqual("export", exported.get("type"), exported)
            code = exported["code"]
            self.assertNotIn(f"\n{id_marker}", code)
            self.assertNotIn(f"\n{title_marker}", code)
            self.assertNotIn("\r", code)
            self.assertNotIn("\x01", code)
            self.assertNotIn("\x02", code)

    def test_billion_slot_game_widgets_are_rejected_and_app_stays_live(self) -> None:
        self.app.send({"cmd": "create_window", "id": "bounded", "title": "Bounded"})
        self.assertEqual("ack", self.app.read_json().get("type"))

        for widget_type, int_value in (
            ("inventory_grid", [1_000_000_000, 1_000_000_000]),
            ("skill_bar", 1_000_000_000),
        ):
            with self.subTest(widget_type=widget_type):
                started = time.monotonic()
                self.app.send(
                    {
                        "cmd": "add_widget",
                        "window": "bounded",
                        "id": widget_type,
                        "widget_type": widget_type,
                        "int_value": int_value,
                    }
                )
                response = self.app.read_json(timeout=2.0)
                self.assertLess(time.monotonic() - started, 2.0)
                self.assertEqual("error", response.get("type"), response)

        self.app.send({"cmd": "get_state"})
        self.assertEqual("state", self.app.read_json().get("type"))

    def test_regular_file_export_and_import_round_trip(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            path = Path(temp_dir) / "layout.json"
            self.app.send({"cmd": "create_window", "id": "saved", "title": "Saved"})
            self.assertEqual("ack", self.app.read_json().get("type"))

            self.app.send({"cmd": "export_json", "path": str(path)})
            exported = self.app.read_json()
            self.assertEqual("export", exported.get("type"), exported)
            self.assertEqual(path, Path(exported["path"]))
            self.assertTrue(path.is_file())

            self.app.send({"cmd": "clear_all"})
            self.assertEqual("ack", self.app.read_json().get("type"))
            self.app.send({"cmd": "import_json", "path": str(path)})
            self.assertEqual("ack", self.app.read_json().get("type"))
            self.app.send({"cmd": "get_state"})
            self.assertIn("saved", self.app.read_json()["windows"])

    @unittest.skipUnless(Path("/dev/stdout").exists(), "/dev paths are Unix-specific")
    def test_import_export_reject_special_files_without_corrupting_protocol(self) -> None:
        for command, path in (("export_cpp", "/dev/stdout"), ("import_json", "/dev/zero")):
            self.app.send({"cmd": command, "path": path, "request_id": command})
            response = self.app.read_json()
            self.assertEqual("error", response.get("type"), response)
            self.assertEqual(command, response.get("cmd"), response)
            self.assertEqual(command, response.get("request_id"), response)
            self.assertIn("regular file", response.get("message", ""), response)

        self.app.send({"cmd": "get_state"})
        self.assertEqual("state", self.app.read_json().get("type"))

    def test_export_rejects_symlink_and_preserves_target(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            target = Path(temp_dir) / "target.cpp"
            link = Path(temp_dir) / "linked.cpp"
            original = b"do not overwrite"
            target.write_bytes(original)
            try:
                link.symlink_to(target)
            except (NotImplementedError, OSError) as error:
                self.skipTest(f"symlinks are unavailable: {error}")

            self.app.send({"cmd": "export_cpp", "path": str(link)})
            response = self.app.read_json()
            self.assertEqual("error", response.get("type"), response)
            self.assertIn("regular file", response.get("message", ""), response)
            self.assertEqual(original, target.read_bytes())

    def test_oversized_import_file_is_rejected_and_app_survives(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            path = Path(temp_dir) / "oversized.json"
            path.write_bytes(b"x" * (4 * 1024 * 1024 + 1))

            self.app.send({"cmd": "import_json", "path": str(path)})
            response = self.app.read_json()
            self.assertEqual("error", response.get("type"), response)
            self.assertIn("4 MiB", response.get("message", ""), response)

            self.app.send({"cmd": "get_state"})
            self.assertEqual("state", self.app.read_json().get("type"))

    def test_table_header_mismatch_does_not_abort(self) -> None:
        self.app.send({"cmd": "create_window", "id": "table", "title": "Table"})
        self.assertEqual("ack", self.app.read_json().get("type"))
        self.app.send(
            {
                "cmd": "add_widget",
                "window": "table",
                "id": "grid",
                "widget_type": "table",
                "columns": 1,
                "headers": ["A", "B"],
                "rows": [["1", "2"]],
            }
        )
        self.assertEqual("ack", self.app.read_json().get("type"))
        time.sleep(0.1)
        self.app.send({"cmd": "get_state"})
        self.assertEqual("state", self.app.read_json().get("type"))

    def test_type_text_focuses_target_and_preserves_utf8(self) -> None:
        self.app.send({"cmd": "create_window", "id": "typing", "title": "Typing"})
        self.assertEqual("ack", self.app.read_json().get("type"))
        self.app.send(
            {
                "cmd": "add_widget",
                "window": "typing",
                "id": "field",
                "widget_type": "input_text",
                "label": "Field",
            }
        )
        self.assertEqual("ack", self.app.read_json().get("type"))
        time.sleep(0.1)
        self.app.send(
            {"cmd": "type_text", "window": "typing", "id": "field", "text": "hé🙂"}
        )
        self.assertEqual("ack", self.app.read_json().get("type"))
        time.sleep(0.1)
        self.app.send({"cmd": "get_state"})
        state = self.app.read_json()
        while state.get("type") == "event":
            state = self.app.read_json()
        self.assertEqual("hé🙂", state["windows"]["typing"]["widgets"]["field"]["text"])

    def test_odd_width_screenshot_has_valid_bmp_layout(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            path = Path(temp_dir) / "odd.bmp"
            self.app.send({"cmd": "set_viewport_size", "width": 391, "height": 241})
            self.assertEqual("ack", self.app.read_json().get("type"))
            self.app.send({"cmd": "screenshot", "path": str(path)})
            self.assertEqual("ack", self.app.read_json().get("type"))
            completion = self.app.read_json()
            self.assertEqual("screenshot", completion.get("type"), completion)

            data = path.read_bytes()
            width, height = struct.unpack_from("<ii", data, 18)
            row_size = ((width * 3 + 3) // 4) * 4
            self.assertEqual(54 + row_size * height, len(data))

    @unittest.skipUnless(Path("/dev/full").exists(), "/dev/full is Unix-specific")
    def test_screenshot_write_failure_is_reported_and_app_survives(self) -> None:
        self.app.send({"cmd": "screenshot", "path": "/dev/full"})
        self.assertEqual("ack", self.app.read_json().get("type"))
        completion = self.app.read_json()
        self.assertEqual("error", completion.get("type"), completion)
        self.assertIn("failed to write", completion.get("message", ""), completion)

        self.app.send({"cmd": "get_state"})
        self.assertEqual("state", self.app.read_json().get("type"))

    @unittest.skipUnless(Path("/dev/stdout").exists(), "/dev/stdout is Unix-specific")
    def test_screenshot_rejects_protocol_stream_path_without_corrupting_output(self) -> None:
        self.app.send(
            {"cmd": "screenshot", "path": "/dev/stdout", "request_id": "unsafe-shot"}
        )
        ack = self.app.read_json()
        self.assertEqual("ack", ack.get("type"), ack)
        completion = self.app.read_json()
        self.assertEqual("error", completion.get("type"), completion)
        self.assertEqual("unsafe-shot", completion.get("request_id"), completion)
        self.assertIn("regular file", completion.get("message", ""), completion)

        self.app.send({"cmd": "get_state"})
        self.assertEqual("state", self.app.read_json().get("type"))

    def test_screenshot_rejects_symlink_and_preserves_target(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            target = Path(temp_dir) / "target.bmp"
            link = Path(temp_dir) / "linked.bmp"
            original = b"do not overwrite"
            target.write_bytes(original)
            try:
                link.symlink_to(target)
            except (NotImplementedError, OSError) as error:
                self.skipTest(f"symlinks are unavailable: {error}")

            self.app.send({"cmd": "screenshot", "path": str(link)})
            self.assertEqual("ack", self.app.read_json().get("type"))
            completion = self.app.read_json()
            self.assertEqual("error", completion.get("type"), completion)
            self.assertIn("regular file", completion.get("message", ""), completion)
            self.assertEqual(original, target.read_bytes())

    def test_annotated_and_widget_screenshots_return_real_metadata(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            plain = Path(temp_dir) / "plain.bmp"
            annotated = Path(temp_dir) / "annotated.bmp"
            widget_path = Path(temp_dir) / "widget.bmp"

            self.app.send({"cmd": "create_window", "id": "shots", "title": "Shots"})
            self.assertEqual("ack", self.app.read_json().get("type"))
            self.app.send(
                {
                    "cmd": "add_widget",
                    "window": "shots",
                    "id": "target",
                    "widget_type": "button",
                    "label": "Target",
                }
            )
            self.assertEqual("ack", self.app.read_json().get("type"))
            time.sleep(0.1)

            for command, path in (
                ("screenshot", plain),
                ("screenshot_annotated", annotated),
            ):
                self.app.send({"cmd": command, "path": str(path)})
                self.assertEqual("ack", self.app.read_json().get("type"))
                completion = self.app.read_json()
                self.assertEqual("screenshot", completion.get("type"), completion)

            self.assertNotEqual(plain.read_bytes(), annotated.read_bytes())

            self.app.send(
                {
                    "cmd": "screenshot_widget",
                    "window": "shots",
                    "widget": "target",
                    "path": str(widget_path),
                }
            )
            self.assertEqual("ack", self.app.read_json().get("type"))
            completion = self.app.read_json()
            info = completion["widget_info"]
            self.assertTrue(info["found"])
            self.assertGreater(info["width"], 0)
            self.assertGreater(info["height"], 0)

            self.app.send(
                {
                    "cmd": "set_widget_visibility",
                    "window": "shots",
                    "id": "target",
                    "condition": "never",
                }
            )
            self.assertEqual("ack", self.app.read_json().get("type"))
            time.sleep(0.1)
            self.app.send(
                {
                    "cmd": "screenshot_widget",
                    "window": "shots",
                    "widget": "target",
                    "path": str(widget_path),
                }
            )
            self.assertEqual("ack", self.app.read_json().get("type"))
            completion = self.app.read_json()
            self.assertEqual("error", completion.get("type"), completion)
            self.assertIn("not rendered", completion.get("message", ""), completion)

    def test_invalid_import_preserves_existing_state(self) -> None:
        self.app.send({"cmd": "create_window", "id": "keep", "title": "Keep"})
        self.assertEqual("ack", self.app.read_json().get("type"))
        self.app.send({"cmd": "import_json", "json": {"windows": [{"title": "Missing id"}]}})
        self.assertEqual("error", self.app.read_json().get("type"))
        self.app.send({"cmd": "get_state"})
        state = self.app.read_json()
        self.assertIn("keep", state["windows"])

    def test_malformed_texture_is_bounded_and_uses_placeholder(self) -> None:
        with tempfile.TemporaryDirectory() as temp_dir:
            path = Path(temp_dir) / "huge.bmp"
            header = bytearray(54)
            header[0:2] = b"BM"
            struct.pack_into("<I", header, 10, 54)
            struct.pack_into("<I", header, 14, 40)
            struct.pack_into("<ii", header, 18, 1_000_000_000, 1_000_000_000)
            struct.pack_into("<HH", header, 26, 1, 24)
            path.write_bytes(header)

            self.app.send({"cmd": "load_texture", "id": "bounded", "path": str(path)})
            response = self.app.read_json()
            self.assertEqual("ack", response.get("type"), response)
            self.assertTrue(response.get("placeholder"), response)
            self.assertEqual((64, 64), (response.get("width"), response.get("height")))

            self.app.send({"cmd": "unload_texture", "id": "bounded"})
            self.assertEqual("ack", self.app.read_json().get("type"))


if __name__ == "__main__":
    unittest.main()
