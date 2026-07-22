# imgui-mcp Project Memory

## Repository
- **GitHub**: https://github.com/SaloQT/imgui-mcp
- **Local**: /coding/ompimguimcp
- **Version**: 1.0.4 | **License**: MIT

## What It Is
Dear ImGui MCP Server — AI agents design, control & debug game UIs in real-time via Model Context Protocol. 70+ MCP tools, 102 widget types, visual feedback loop (screenshots), input simulation, animation, theming, game patterns, export to C++/Lua/JSON.

## Architecture
- `server.py` — MCP server, 71 tools, zero Python deps, JSON-RPC 2.0 over stdio
- `src/main.cpp` — App lifecycle, SDL2+OpenGL3, stdin reader thread, emit helpers
- `src/render.cpp` — render_widget() with 102 widget type switch cases
- `src/commands.cpp` — process_command() with all command handlers + parse_widget_type()
- `src/types.h` — Shared enums (WidgetType, EaseType, Anchor), structs (Widget, WindowState, Animation, Scene, Snapshot), extern globals

## Critical Design Decisions
1. **Three mutexes**: g_mutex (state), g_cmd_mutex (command queue), out_mutex (stdout) — prevents deadlock with non-recursive std::mutex
2. **Main loop pattern**: drain queue with g_cmd_mutex → process commands WITHOUT holding any lock → render with g_mutex held
3. **Windows console app**: `#define SDL_MAIN_HANDLED` + `SDL_SetMainReady()` + `-mconsole` linker flag (avoids WinMain requirement)
4. **MCP config differences**: VS Code uses `"servers"` key (not `"mcpServers"`), Zed uses `"context_servers"` with nested `command.path`, Codex uses TOML, Continue uses YAML array format

## Supported IDE/CLI Tools (13)
omp (.omp/mcp.json), Claude Desktop/Code (.mcp.json), Cursor (.cursor/mcp.json), VS Code/Copilot (.vscode/mcp.json), Windsurf (.windsurf/mcp.json), Gemini CLI (.gemini/settings.json), Codex CLI (.codex/config.toml), Cline (global), Roo Code (.roo/mcp.json), Zed (.zed/settings.json), Continue (~/.continue/config.yaml), Augment (~/.augment/mcp.json)

## Build Commands
- Linux: `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build --parallel`
- Windows cross: `cmake -B build-win -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64-x86_64.cmake && cmake --build build-win`
- Release: `./release.sh [version]` → validates `VERSION` and writes Linux/Windows archives to `dist/`

## CI/CD
`.github/workflows/release.yml` — triggers on `v*` tag push. Two parallel build jobs (Linux native, Windows MinGW cross-compile), then a release job that downloads artifacts and creates GitHub Release with binaries + auto-generated notes.

## Release Process
```bash
git tag v1.1.0 && git push origin v1.1.0  # automatic via GitHub Actions
```

## GitHub SEO
- Name: `imgui-mcp` (shortest, hits both top search keywords)
- 18 topics for discoverability
- README: badges, collapsible tool configs, architecture diagram, full feature tables
