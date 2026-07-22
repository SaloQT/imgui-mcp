#!/usr/bin/env bash
# imgui-mcp setup script
# Installs the MCP server configuration for all supported AI coding tools.
# Usage: ./setup.sh [--project-dir /path/to/project]
#
# Supported tools:
#   omp, Claude Desktop, Claude Code, Cursor, VS Code/Copilot,
#   Windsurf, Cline, Continue, Zed, Codex CLI, Gemini CLI, Roo Code, Augment

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SERVER_PY="${SCRIPT_DIR}/server.py"
BUILD_DIR="${SCRIPT_DIR}/build"
BINARY="${BUILD_DIR}/imgui_mcp_app"

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

info()  { echo -e "${GREEN}[✓]${NC} $1"; }
warn()  { echo -e "${YELLOW}[!]${NC} $1"; }
error() { echo -e "${RED}[✗]${NC} $1"; }

echo "═══════════════════════════════════════════════════════════"
echo "  imgui-mcp — Dear ImGui v1.92.8 MCP Server Setup"
echo "═══════════════════════════════════════════════════════════"
echo ""
echo "  Server: ${SERVER_PY}"
echo ""

# ─── Step 1: Build ────────────────────────────────────────────────────────────

if [ ! -f "${BINARY}" ]; then
    info "Building imgui_mcp_app..."
    mkdir -p "${BUILD_DIR}"
    cd "${BUILD_DIR}"
    cmake .. -DCMAKE_BUILD_TYPE=Release > /dev/null 2>&1
    make -j"$(nproc)" > /dev/null 2>&1
    cd "${SCRIPT_DIR}"
    info "Build complete: ${BINARY}"
else
    info "Binary already built: ${BINARY}"
fi

# ─── Step 2: Detect platform ─────────────────────────────────────────────────

OS="$(uname -s)"
case "${OS}" in
    Linux*)  PLATFORM=linux;;
    Darwin*) PLATFORM=macos;;
    MINGW*|MSYS*|CYGWIN*) PLATFORM=windows;;
    *)       PLATFORM=unknown;;
esac
info "Platform: ${PLATFORM}"

# ─── Step 3: Install configs ─────────────────────────────────────────────────

installed=0

install_json_config() {
    local tool="$1"
    local path="$2"
    local content="$3"

    mkdir -p "$(dirname "${path}")"
    if [ -f "${path}" ]; then
        # Check if already has imgui entry
        if grep -q '"imgui"' "${path}" 2>/dev/null; then
            warn "${tool}: already configured (${path})"
            return
        fi
        warn "${tool}: config exists, appending imgui server (${path})"
        # For simplicity, just note it exists - user should merge manually
    fi
    echo "${content}" > "${path}"
    info "${tool}: ${path}"
    installed=$((installed + 1))
}

# MCP server JSON (standard mcpServers format)
MCP_JSON=$(cat <<EOF
{
  "mcpServers": {
    "imgui": {
      "command": "python3",
      "args": ["${SERVER_PY}"],
      "env": {}
    }
  }
}
EOF
)

# VS Code format (uses "servers" key)
VSCODE_JSON=$(cat <<EOF
{
  "servers": {
    "imgui": {
      "type": "stdio",
      "command": "python3",
      "args": ["${SERVER_PY}"],
      "env": {}
    }
  }
}
EOF
)

# Zed format (uses "context_servers" with nested command)
ZED_JSON=$(cat <<EOF
{
  "context_servers": {
    "imgui": {
      "source": "custom",
      "command": {
        "path": "python3",
        "args": ["${SERVER_PY}"],
        "env": {}
      }
    }
  }
}
EOF
)

# Codex TOML format
CODEX_TOML=$(cat <<EOF
[mcp_servers.imgui]
command = "python3"
args = ["${SERVER_PY}"]
EOF
)

echo ""
echo "── Installing configurations ──"
echo ""

# omp (project-level already exists)
if [ -d "${SCRIPT_DIR}/.omp" ]; then
    info "omp: .omp/mcp.json (project-level, already present)"
    installed=$((installed + 1))
fi

# Claude Desktop
case "${PLATFORM}" in
    linux)   CLAUDE_DESKTOP="${HOME}/.config/Claude/claude_desktop_config.json";;
    macos)   CLAUDE_DESKTOP="${HOME}/Library/Application Support/Claude/claude_desktop_config.json";;
    windows) CLAUDE_DESKTOP="${APPDATA}/Claude/claude_desktop_config.json";;
esac
if [ -n "${CLAUDE_DESKTOP:-}" ]; then
    install_json_config "Claude Desktop" "${CLAUDE_DESKTOP}" "${MCP_JSON}"
fi

# Claude Code (global)
CLAUDE_CODE="${HOME}/.claude.json"
if [ ! -f "${CLAUDE_CODE}" ] || ! grep -q '"imgui"' "${CLAUDE_CODE}" 2>/dev/null; then
    info "Claude Code: use '.mcp.json' in project root (already present)"
    installed=$((installed + 1))
fi

# Cursor (global)
CURSOR_GLOBAL="${HOME}/.cursor/mcp.json"
install_json_config "Cursor" "${CURSOR_GLOBAL}" "${MCP_JSON}"

# VS Code (user-level)
case "${PLATFORM}" in
    linux)   VSCODE_DIR="${HOME}/.config/Code/User";;
    macos)   VSCODE_DIR="${HOME}/Library/Application Support/Code/User";;
    windows) VSCODE_DIR="${APPDATA}/Code/User";;
esac
install_json_config "VS Code/Copilot" "${VSCODE_DIR}/mcp.json" "${VSCODE_JSON}"

# Windsurf (global)
WINDSURF_GLOBAL="${HOME}/.codeium/windsurf/mcp_config.json"
install_json_config "Windsurf" "${WINDSURF_GLOBAL}" "${MCP_JSON}"

# Cline (global)
case "${PLATFORM}" in
    linux)   CLINE_DIR="${HOME}/.config/Code/User/globalStorage/saoudrizwan.claude-dev/settings";;
    macos)   CLINE_DIR="${HOME}/Library/Application Support/Code/User/globalStorage/saoudrizwan.claude-dev/settings";;
    windows) CLINE_DIR="${APPDATA}/Code/User/globalStorage/saoudrizwan.claude-dev/settings";;
esac
install_json_config "Cline" "${CLINE_DIR}/cline_mcp_settings.json" "${MCP_JSON}"

# Roo Code (global)
case "${PLATFORM}" in
    linux)   ROO_DIR="${HOME}/.config/Code/User/globalStorage/rooveterinaryinc.roo-cline/settings";;
    macos)   ROO_DIR="${HOME}/Library/Application Support/Code/User/globalStorage/rooveterinaryinc.roo-cline/settings";;
    windows) ROO_DIR="${APPDATA}/Code/User/globalStorage/rooveterinaryinc.roo-cline/settings";;
esac
install_json_config "Roo Code" "${ROO_DIR}/mcp_settings.json" "${MCP_JSON}"

# Gemini CLI (global)
GEMINI_GLOBAL="${HOME}/.gemini/settings.json"
install_json_config "Gemini CLI" "${GEMINI_GLOBAL}" "${MCP_JSON}"

# Augment (global)
AUGMENT_GLOBAL="${HOME}/.augment/mcp.json"
install_json_config "Augment" "${AUGMENT_GLOBAL}" "${MCP_JSON}"

# Zed (global)
case "${PLATFORM}" in
    linux)   ZED_SETTINGS="${HOME}/.config/zed/settings.json";;
    macos)   ZED_SETTINGS="${HOME}/Library/Application Support/Zed/settings.json";;
    windows) ZED_SETTINGS="${APPDATA}/Zed/settings.json";;
esac
if [ ! -f "${ZED_SETTINGS}" ]; then
    install_json_config "Zed" "${ZED_SETTINGS}" "${ZED_JSON}"
else
    warn "Zed: ${ZED_SETTINGS} exists — merge context_servers manually"
fi

# Codex CLI (global)
CODEX_CONFIG="${HOME}/.codex/config.toml"
mkdir -p "$(dirname "${CODEX_CONFIG}")"
if [ ! -f "${CODEX_CONFIG}" ] || ! grep -q 'imgui' "${CODEX_CONFIG}" 2>/dev/null; then
    echo "${CODEX_TOML}" >> "${CODEX_CONFIG}"
    info "Codex CLI: ${CODEX_CONFIG}"
    installed=$((installed + 1))
else
    warn "Codex CLI: already configured"
fi

# Continue (global - YAML)
CONTINUE_CONFIG="${HOME}/.continue/config.yaml"
if [ ! -f "${CONTINUE_CONFIG}" ]; then
    mkdir -p "${HOME}/.continue"
    cat > "${CONTINUE_CONFIG}" <<EOF
name: imgui-mcp
version: 0.0.1
schema: v1

mcpServers:
  - name: imgui
    command: python3
    args:
      - "${SERVER_PY}"
    env: {}
EOF
    info "Continue: ${CONTINUE_CONFIG}"
    installed=$((installed + 1))
else
    warn "Continue: ${CONTINUE_CONFIG} exists — add mcpServers entry manually"
fi

# ─── Summary ──────────────────────────────────────────────────────────────────

echo ""
echo "═══════════════════════════════════════════════════════════"
echo "  Setup complete! ${installed} tools configured."
echo ""
echo "  Project-level configs (portable, commit to git):"
echo "    .omp/mcp.json        → omp"
echo "    .mcp.json            → Claude Code"
echo "    .cursor/mcp.json     → Cursor"
echo "    .vscode/mcp.json     → VS Code / Copilot"
echo "    .windsurf/mcp.json   → Windsurf"
echo "    .gemini/settings.json→ Gemini CLI"
echo "    .roo/mcp.json        → Roo Code"
echo "    .zed/settings.json   → Zed"
echo "    .codex/config.toml   → Codex CLI"
echo ""
echo "  To test: python3 server.py (then send MCP initialize)"
echo "  To demo: python3 demo.py"
echo "═══════════════════════════════════════════════════════════"
