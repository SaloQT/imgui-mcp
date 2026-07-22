#!/usr/bin/env bash
# Creates distributable release packages for Linux and Windows.
# Usage: ./release.sh [version]
# Output:
#   imgui-mcp-<version>-linux-x64.tar.gz
#   imgui-mcp-<version>-windows-x64.zip

set -euo pipefail

VERSION="${1:-1.0.0}"
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "${SCRIPT_DIR}"

echo "═══════════════════════════════════════════════════════════"
echo "  imgui-mcp v${VERSION} — Release Builder"
echo "═══════════════════════════════════════════════════════════"
echo ""

# ─── Build Linux ──────────────────────────────────────────────────────────────

echo "── Building Linux (native) ──"
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release > /dev/null 2>&1
make -j"$(nproc)" > /dev/null 2>&1
cd ..
echo "  ✓ build/imgui_mcp_app ($(du -h build/imgui_mcp_app | cut -f1))"

# ─── Build Windows (cross-compile with MinGW) ────────────────────────────────

echo "── Building Windows (MinGW cross-compile) ──"
if command -v x86_64-w64-mingw32-g++ &>/dev/null; then
    mkdir -p build-win
    cd build-win
    cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/mingw-w64-x86_64.cmake -DCMAKE_BUILD_TYPE=Release > /dev/null 2>&1
    make -j"$(nproc)" > /dev/null 2>&1
    cd ..
    echo "  ✓ build-win/imgui_mcp_app.exe ($(du -h build-win/imgui_mcp_app.exe | cut -f1))"
    WINDOWS_BUILT=true
else
    echo "  ⚠ MinGW not found, skipping Windows build"
    WINDOWS_BUILT=false
fi

# ─── Package Linux ────────────────────────────────────────────────────────────

echo ""
echo "── Packaging Linux ──"
LINUX_NAME="imgui-mcp-${VERSION}-linux-x64"
LINUX_DIR="/tmp/${LINUX_NAME}"
rm -rf "${LINUX_DIR}"
mkdir -p "${LINUX_DIR}"

# Source + configs
cp -r src/ "${LINUX_DIR}/src/"
cp -r vendor/ "${LINUX_DIR}/vendor/"
cp -r cmake/ "${LINUX_DIR}/cmake/"
cp CMakeLists.txt Makefile server.py demo.py setup.sh README.md "${LINUX_DIR}/"
cp .mcp.json "${LINUX_DIR}/"
cp -r .omp/ .cursor/ .vscode/ .windsurf/ .gemini/ .roo/ .zed/ .codex/ "${LINUX_DIR}/" 2>/dev/null || true

# Pre-built binary
mkdir -p "${LINUX_DIR}/bin"
cp build/imgui_mcp_app "${LINUX_DIR}/bin/"

cd /tmp
tar -czf "${LINUX_NAME}.tar.gz" "${LINUX_NAME}"
LINUX_SIZE=$(du -h "${LINUX_NAME}.tar.gz" | cut -f1)
echo "  ✓ ${LINUX_NAME}.tar.gz (${LINUX_SIZE})"
cd "${SCRIPT_DIR}"
rm -rf "${LINUX_DIR}"

# ─── Package Windows ──────────────────────────────────────────────────────────

if [ "${WINDOWS_BUILT}" = true ]; then
    echo "── Packaging Windows ──"
    WIN_NAME="imgui-mcp-${VERSION}-windows-x64"
    WIN_DIR="/tmp/${WIN_NAME}"
    rm -rf "${WIN_DIR}"
    mkdir -p "${WIN_DIR}"

    # Source + configs
    cp -r src/ "${WIN_DIR}/src/"
    cp -r vendor/ "${WIN_DIR}/vendor/"
    cp -r cmake/ "${WIN_DIR}/cmake/"
    cp -r cross/ "${WIN_DIR}/cross/"
    cp CMakeLists.txt server.py demo.py setup.ps1 README.md "${WIN_DIR}/"
    cp .mcp.json "${WIN_DIR}/"
    cp -r .omp/ .cursor/ .vscode/ .windsurf/ .gemini/ .roo/ .zed/ .codex/ "${WIN_DIR}/" 2>/dev/null || true

    # Pre-built binary + SDL2.dll
    mkdir -p "${WIN_DIR}/bin"
    cp build-win/imgui_mcp_app.exe "${WIN_DIR}/bin/"
    cp cross/bin/SDL2.dll "${WIN_DIR}/bin/"

    # Create installer batch file
    cat > "${WIN_DIR}/install.bat" <<'BATCH'
@echo off
echo imgui-mcp Windows Installer
echo ===========================
echo.
echo Adding to PATH and running setup...
powershell -ExecutionPolicy Bypass -File "%~dp0setup.ps1" -SkipBuild
echo.
echo Done! Restart your IDE/CLI to pick up the MCP server.
pause
BATCH

    cd /tmp
    zip -qr "${WIN_NAME}.zip" "${WIN_NAME}"
    WIN_SIZE=$(du -h "${WIN_NAME}.zip" | cut -f1)
    echo "  ✓ ${WIN_NAME}.zip (${WIN_SIZE})"
    rm -rf "${WIN_DIR}"
    cd "${SCRIPT_DIR}"
fi

# ─── Summary ──────────────────────────────────────────────────────────────────

echo ""
echo "═══════════════════════════════════════════════════════════"
echo "  Release v${VERSION} complete!"
echo ""
echo "  Packages:"
echo "    /tmp/${LINUX_NAME}.tar.gz (${LINUX_SIZE})"
if [ "${WINDOWS_BUILT}" = true ]; then
echo "    /tmp/${WIN_NAME}.zip (${WIN_SIZE})"
fi
echo ""
echo "  Linux install:"
echo "    tar -xzf ${LINUX_NAME}.tar.gz && cd ${LINUX_NAME} && ./setup.sh"
echo ""
echo "  Windows install:"
echo "    Extract zip, run install.bat (or: powershell .\\setup.ps1)"
echo ""
echo "  Both packages include:"
echo "    • Pre-built binary (bin/)"
echo "    • Full source code (src/ + vendor/)"
echo "    • MCP server (server.py, zero Python deps)"
echo "    • IDE configs for 13 tools"
echo "    • Demo script (demo.py)"
echo "═══════════════════════════════════════════════════════════"
