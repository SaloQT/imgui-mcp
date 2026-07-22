# imgui-mcp Windows Setup Script
# Installs MCP server configuration for all supported AI coding tools on Windows.
# Usage: .\setup.ps1 [-BuildOnly] [-SkipBuild]
#
# Prerequisites:
#   - CMake (winget install Kitware.CMake)
#   - Visual Studio 2022 with C++ workload (or Build Tools)
#   - Python 3.8+ (winget install Python.Python.3.12)
#   - SDL2 (vcpkg install sdl2:x64-windows OR download from libsdl.org)
#   - OpenGL (included with GPU drivers)
#
# Supported tools:
#   omp, Claude Desktop, Claude Code, Cursor, VS Code/Copilot,
#   Windsurf, Cline, Continue, Zed, Codex CLI, Gemini CLI, Roo Code, Augment

param(
    [switch]$BuildOnly,
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ServerPy = Join-Path $ScriptDir "server.py"
$BuildDir = Join-Path $ScriptDir "build"
$Binary = Join-Path $BuildDir "Release\imgui_mcp_app.exe"
$PackagedBinary = Join-Path $ScriptDir "bin\imgui_mcp_app.exe"
if (Test-Path $PackagedBinary) { $Binary = $PackagedBinary }

$PythonCommand = "uv"
if (-not (Get-Command $PythonCommand -ErrorAction SilentlyContinue)) {
    Write-Host "[X] uv is required for the official MCP Python SDK: https://docs.astral.sh/uv/" -ForegroundColor Red
    exit 1
}
$sync = Start-Process -FilePath $PythonCommand -ArgumentList @("sync", "--project", $ScriptDir, "--frozen") -Wait -PassThru -NoNewWindow
if ($sync.ExitCode -ne 0) {
    Write-Host "[X] Failed to install the official MCP Python SDK environment." -ForegroundColor Red
    exit 1
}
$ServerPyJson = $ServerPy -replace '\\','/'
$ScriptDirJson = $ScriptDir -replace '\\','/'
$PythonArgsJson = '"run", "--project", "' + $ScriptDirJson + '", "python", "' + $ServerPyJson + '"'
$PythonYamlArgs = "      - run`n      - --project`n      - `"$ScriptDirJson`"`n      - python`n      - `"$ServerPyJson`""

function Write-OK($msg)   { Write-Host "[OK] $msg" -ForegroundColor Green }
function Write-Warn($msg) { Write-Host "[!] $msg" -ForegroundColor Yellow }
function Write-Err($msg)  { Write-Host "[X] $msg" -ForegroundColor Red }

Write-Host "==========================================================="
Write-Host "  imgui-mcp - Dear ImGui v1.92.8 MCP Server Setup (Windows)"
Write-Host "==========================================================="
Write-Host ""
Write-Host "  Server: $ServerPy"
Write-Host ""

# --- Step 1: Build ---

if (-not $SkipBuild) {
    if (-not (Test-Path $Binary)) {
        Write-OK "Building imgui_mcp_app..."

        # Check for CMake
        if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
            Write-Err "CMake not found. Install: winget install Kitware.CMake"
            exit 1
        }

        # Check for Visual Studio / MSVC
        $vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
        if (Test-Path $vsWhere) {
            $vsPath = & $vsWhere -latest -property installationPath 2>$null
            if ($vsPath) {
                Write-OK "Found Visual Studio: $vsPath"
            }
        }

        # Check for SDL2
        $sdl2Found = $false
        $vcpkgRoot = $env:VCPKG_ROOT
        if ($vcpkgRoot -and (Test-Path (Join-Path $vcpkgRoot "installed\x64-windows\include\SDL2\SDL.h"))) {
            $sdl2Found = $true
            Write-OK "Found SDL2 via vcpkg: $vcpkgRoot"
        }

        # Try to find SDL2 in common locations
        $sdl2Paths = @(
            "C:\SDL2",
            "C:\vcpkg\installed\x64-windows",
            "$env:USERPROFILE\vcpkg\installed\x64-windows",
            "C:\Program Files\SDL2"
        )
        if (-not $sdl2Found) {
            foreach ($p in $sdl2Paths) {
                if (Test-Path (Join-Path $p "include\SDL2\SDL.h")) {
                    $sdl2Found = $true
                    Write-OK "Found SDL2: $p"
                    break
                }
            }
        }

        if (-not $sdl2Found) {
            Write-Warn "SDL2 not found automatically."
            Write-Host "  Option 1: vcpkg install sdl2:x64-windows"
            Write-Host "  Option 2: Download from https://libsdl.org and extract to C:\SDL2"
            Write-Host "  Then re-run this script with: .\setup.ps1"
            Write-Host ""
            Write-Host "  Attempting build anyway (may fail)..."
        }

        # Build
        New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
        Push-Location $BuildDir

        $cmakeArgs = @("..", "-DCMAKE_BUILD_TYPE=Release")
        if ($vcpkgRoot) {
            $cmakeArgs += "-DCMAKE_TOOLCHAIN_FILE=$vcpkgRoot\scripts\buildsystems\vcpkg.cmake"
        }
        # If SDL2 is in a custom location
        foreach ($p in $sdl2Paths) {
            if (Test-Path (Join-Path $p "include\SDL2\SDL.h")) {
                $cmakeArgs += "-DSDL2_DIR=$p"
                $cmakeArgs += "-DCMAKE_PREFIX_PATH=$p"
                break
            }
        }

        & cmake @cmakeArgs 2>&1 | Out-Null
        if ($LASTEXITCODE -ne 0) {
            Write-Err "CMake configure failed. Check SDL2 installation."
            Pop-Location
            exit 1
        }

        & cmake --build . --config Release --parallel 2>&1 | Out-Null
        if ($LASTEXITCODE -ne 0) {
            Write-Err "Build failed."
            Pop-Location
            exit 1
        }

        Pop-Location
        Write-OK "Build complete: $Binary"
    } else {
        Write-OK "Binary already built: $Binary"
    }
}

if ($BuildOnly) { exit 0 }

# --- Step 2: Install configs ---

$installed = 0

function Install-JsonConfig {
    param([string]$Tool, [string]$Path, [string]$Content)

    $dir = Split-Path -Parent $Path
    if (-not (Test-Path $dir)) {
        New-Item -ItemType Directory -Force -Path $dir | Out-Null
    }

    if (Test-Path $Path) {
        $existing = Get-Content $Path -Raw
        if ($existing -match '"imgui"') {
            Write-Warn "${Tool}: already configured ($Path)"
            return
        }
        Write-Warn "${Tool}: config exists and was left unchanged ($Path); merge the imgui entry manually"
        return
    }

    Set-Content -Path $Path -Value $Content -Encoding UTF8
    Write-OK "${Tool}: $Path"
    $script:installed++
}

# Standard mcpServers JSON
$mcpJson = @"
{
  "mcpServers": {
    "imgui": {
      "command": "$PythonCommand",
      "args": [$PythonArgsJson],
      "env": {}
    }
  }
}
"@

# VS Code format
$vscodeJson = @"
{
  "servers": {
    "imgui": {
      "type": "stdio",
      "command": "$PythonCommand",
      "args": [$PythonArgsJson],
      "env": {}
    }
  }
}
"@

# Zed format
$zedJson = @"
{
  "context_servers": {
    "imgui": {
      "source": "custom",
      "command": {
        "path": "$PythonCommand",
        "args": [$PythonArgsJson],
        "env": {}
      }
    }
  }
}
"@

# Codex TOML
$codexToml = @"
[mcp_servers.imgui]
command = "$PythonCommand"
args = [$PythonArgsJson]
"@

Write-Host ""
Write-Host "-- Installing configurations --"
Write-Host ""

# Claude Desktop
$claudeDesktop = "$env:APPDATA\Claude\claude_desktop_config.json"
Install-JsonConfig -Tool "Claude Desktop" -Path $claudeDesktop -Content $mcpJson

# Cursor (global)
$cursorGlobal = "$env:USERPROFILE\.cursor\mcp.json"
Install-JsonConfig -Tool "Cursor" -Path $cursorGlobal -Content $mcpJson

# VS Code (user-level)
$vscodeUser = "$env:APPDATA\Code\User\mcp.json"
Install-JsonConfig -Tool "VS Code/Copilot" -Path $vscodeUser -Content $vscodeJson

# Windsurf (global)
$windsurfGlobal = "$env:USERPROFILE\.codeium\windsurf\mcp_config.json"
Install-JsonConfig -Tool "Windsurf" -Path $windsurfGlobal -Content $mcpJson

# Cline (global)
$clineGlobal = "$env:APPDATA\Code\User\globalStorage\saoudrizwan.claude-dev\settings\cline_mcp_settings.json"
Install-JsonConfig -Tool "Cline" -Path $clineGlobal -Content $mcpJson

# Roo Code (global)
$rooGlobal = "$env:APPDATA\Code\User\globalStorage\rooveterinaryinc.roo-cline\settings\mcp_settings.json"
Install-JsonConfig -Tool "Roo Code" -Path $rooGlobal -Content $mcpJson

# Gemini CLI (global)
$geminiGlobal = "$env:USERPROFILE\.gemini\settings.json"
Install-JsonConfig -Tool "Gemini CLI" -Path $geminiGlobal -Content $mcpJson

# Augment (global)
$augmentGlobal = "$env:USERPROFILE\.augment\mcp.json"
Install-JsonConfig -Tool "Augment" -Path $augmentGlobal -Content $mcpJson

# Zed (global)
$zedSettings = "$env:APPDATA\Zed\settings.json"
if (-not (Test-Path $zedSettings)) {
    Install-JsonConfig -Tool "Zed" -Path $zedSettings -Content $zedJson
} else {
    Write-Warn "Zed: $zedSettings exists - merge context_servers manually"
}

# Codex CLI (global)
$codexConfig = "$env:USERPROFILE\.codex\config.toml"
$codexDir = Split-Path -Parent $codexConfig
if (-not (Test-Path $codexDir)) { New-Item -ItemType Directory -Force -Path $codexDir | Out-Null }
if (-not (Test-Path $codexConfig) -or -not ((Get-Content $codexConfig -Raw) -match 'imgui')) {
    Add-Content -Path $codexConfig -Value $codexToml
    Write-OK "Codex CLI: $codexConfig"
    $installed++
} else {
    Write-Warn "Codex CLI: already configured"
}

# Continue (global - YAML)
$continueConfig = "$env:USERPROFILE\.continue\config.yaml"
if (-not (Test-Path $continueConfig)) {
    $continueDir = Split-Path -Parent $continueConfig
    if (-not (Test-Path $continueDir)) { New-Item -ItemType Directory -Force -Path $continueDir | Out-Null }
    $yamlContent = @"
name: imgui-mcp
version: 0.0.1
schema: v1

mcpServers:
  - name: imgui
    command: $PythonCommand
    args:
$PythonYamlArgs
    env: {}
"@
    Set-Content -Path $continueConfig -Value $yamlContent -Encoding UTF8
    Write-OK "Continue: $continueConfig"
    $installed++
} else {
    Write-Warn "Continue: $continueConfig exists - add mcpServers entry manually"
}

# --- Summary ---

Write-Host ""
Write-Host "==========================================================="
Write-Host "  Setup complete! $installed tools configured."
Write-Host ""
Write-Host "  Project-level configs (portable, commit to git):"
Write-Host "    .omp\mcp.json          -> omp"
Write-Host "    .mcp.json              -> Claude Code"
Write-Host "    .cursor\mcp.json       -> Cursor"
Write-Host "    .vscode\mcp.json       -> VS Code / Copilot"
Write-Host "    .windsurf\mcp.json     -> Windsurf"
Write-Host "    .gemini\settings.json  -> Gemini CLI"
Write-Host "    .roo\mcp.json          -> Roo Code"
Write-Host "    .zed\settings.json     -> Zed"
Write-Host "    .codex\config.toml     -> Codex CLI"
Write-Host ""
Write-Host "  To test: uv run python server.py"
Write-Host "  To demo: uv run python demo.py"
Write-Host "==========================================================="
