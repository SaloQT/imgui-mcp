#!/usr/bin/env bash
# Build reproducible release archives for the version declared in VERSION.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VERSION_FILE="${SCRIPT_DIR}/VERSION"
DECLARED_VERSION="$(tr -d '[:space:]' < "${VERSION_FILE}")"
VERSION="${1:-${DECLARED_VERSION}}"
MODE="${2:---all}"

if [[ ! "${VERSION}" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
    echo "Invalid version: ${VERSION}" >&2
    exit 2
fi
if [[ "${VERSION}" != "${DECLARED_VERSION}" ]]; then
    echo "Version ${VERSION} does not match VERSION (${DECLARED_VERSION})" >&2
    exit 2
fi
if (( $# > 2 )); then
    echo "Usage: $0 [version] [--all|--linux-only|--windows-only]" >&2
    exit 2
fi

BUILD_LINUX=true
BUILD_WINDOWS=true
case "${MODE}" in
    --all) ;;
    --linux-only) BUILD_WINDOWS=false ;;
    --windows-only) BUILD_LINUX=false ;;
    *)
        echo "Unknown release mode: ${MODE}" >&2
        exit 2
        ;;
esac

BUILD_DIR="${SCRIPT_DIR}/build"
BUILD_WIN_DIR="${SCRIPT_DIR}/build-win"
DIST_DIR="${SCRIPT_DIR}/dist"
TEMP_ROOT="$(mktemp -d "${TMPDIR:-/tmp}/imgui-mcp-release.XXXXXX")"
trap 'rm -rf -- "${TEMP_ROOT}"' EXIT

SOURCE_DATE_EPOCH="${SOURCE_DATE_EPOCH:-$(git -C "${SCRIPT_DIR}" log -1 --format=%ct 2>/dev/null || true)}"
if [[ ! "${SOURCE_DATE_EPOCH}" =~ ^[0-9]+$ ]] || (( SOURCE_DATE_EPOCH < 315532800 )); then
    echo "SOURCE_DATE_EPOCH must be a Unix timestamp on or after 1980-01-01" >&2
    exit 2
fi

RELEASE_ARCHIVES=()

mkdir -p "${DIST_DIR}"

echo "imgui-mcp v${VERSION} release build"

if [[ "${BUILD_LINUX}" == true ]]; then
    cmake -S "${SCRIPT_DIR}" -B "${BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release
    cmake --build "${BUILD_DIR}" --parallel
    ctest --test-dir "${BUILD_DIR}" --output-on-failure
fi

WINDOWS_BUILT=false
if [[ "${BUILD_WINDOWS}" == true ]]; then
    if command -v x86_64-w64-mingw32-g++ >/dev/null 2>&1 && \
       [[ -f "${SCRIPT_DIR}/cross/lib/libSDL2.dll.a" ]] && \
       [[ -f "${SCRIPT_DIR}/cross/bin/SDL2.dll" ]]; then
        python3 -m unittest discover -s "${SCRIPT_DIR}/tests" -p 'test_server.py' -v
        cmake -S "${SCRIPT_DIR}" -B "${BUILD_WIN_DIR}" \
            -DCMAKE_TOOLCHAIN_FILE="${SCRIPT_DIR}/cmake/mingw-w64-x86_64.cmake" \
            -DCMAKE_BUILD_TYPE=Release
        cmake --build "${BUILD_WIN_DIR}" --parallel
        WINDOWS_BUILT=true
    elif [[ "${MODE}" == "--windows-only" ]]; then
        echo "Windows release requires MinGW and cross/lib/libSDL2.dll.a plus cross/bin/SDL2.dll" >&2
        exit 1
    else
        echo "Skipping Windows archive: MinGW/SDL2 cross dependencies are unavailable" >&2
    fi
fi

copy_common_files() {
    local destination="$1"
    cp -r "${SCRIPT_DIR}/src" "${SCRIPT_DIR}/vendor" "${SCRIPT_DIR}/cmake" \
        "${SCRIPT_DIR}/assets" "${SCRIPT_DIR}/tests" "${destination}/"
    cp "${SCRIPT_DIR}/CMakeLists.txt" "${SCRIPT_DIR}/server.py" \
        "${SCRIPT_DIR}/demo.py" "${SCRIPT_DIR}/README.md" \
        "${SCRIPT_DIR}/LICENSE" "${SCRIPT_DIR}/VERSION" "${destination}/"
    cp "${SCRIPT_DIR}/.mcp.json" "${destination}/"
    mkdir -p "${destination}/.omp"
    cp "${SCRIPT_DIR}/.omp/mcp.json" "${destination}/.omp/"
    for config_dir in .cursor .vscode .windsurf .gemini .roo .zed .codex .cline .continue; do
        if [[ -d "${SCRIPT_DIR}/${config_dir}" ]]; then
            cp -r "${SCRIPT_DIR}/${config_dir}" "${destination}/"
        fi
    done
    find "${destination}" -type f \( -name '*.pyc' -o -name '*.pyo' \) -delete
    find "${destination}" -depth -type d -name '__pycache__' -empty -delete
}

normalize_tree() {
    local tree="$1"
    find "${tree}" -type d -exec chmod 0755 {} +
    find "${tree}" -type f -exec chmod 0644 {} +
    find "${tree}" -type f \( -name '*.sh' -o -name '*.py' -o -name 'imgui_mcp_app' -o -name '*.exe' \) \
        -exec chmod 0755 {} +
    find "${tree}" -exec touch -h -d "@${SOURCE_DATE_EPOCH}" {} +
}

verify_manifest() {
    local archive="$1"
    local root="$2"
    local kind="$3"
    shift 3
    local listing="${TEMP_ROOT}/$(basename "${archive}").entries"

    case "${kind}" in
        tar) tar -tzf "${archive}" > "${listing}" ;;
        zip) unzip -Z1 "${archive}" > "${listing}" ;;
        *) echo "Unknown archive kind: ${kind}" >&2; return 2 ;;
    esac

    if grep -Eq '(^|/)__pycache__/|\.py[co]$' "${listing}"; then
        echo "Archive contains Python cache files: ${archive}" >&2
        return 1
    fi

    local entry
    for entry in "$@"; do
        if ! grep -Fqx "${root}/${entry}" "${listing}"; then
            echo "Archive manifest is missing ${root}/${entry}: ${archive}" >&2
            return 1
        fi
    done
}

COMMON_MANIFEST=(
    VERSION server.py demo.py README.md LICENSE CMakeLists.txt .mcp.json .omp/mcp.json
    assets/windows/app-icon.png assets/windows/app.ico assets/windows/app.rc
    assets/windows/resource.h tests/test_server.py tests/test_native_protocol.py
)

if [[ "${BUILD_LINUX}" == true ]]; then
    LINUX_NAME="imgui-mcp-${VERSION}-linux-x64"
    LINUX_DIR="${TEMP_ROOT}/${LINUX_NAME}"
    mkdir -p "${LINUX_DIR}/bin"
    copy_common_files "${LINUX_DIR}"
    cp "${SCRIPT_DIR}/Makefile" "${SCRIPT_DIR}/setup.sh" "${LINUX_DIR}/"
    cp "${BUILD_DIR}/imgui_mcp_app" "${LINUX_DIR}/bin/"
    normalize_tree "${LINUX_DIR}"
    LINUX_ARCHIVE="${DIST_DIR}/${LINUX_NAME}.tar.gz"
    tar --sort=name --mtime="@${SOURCE_DATE_EPOCH}" --owner=0 --group=0 --numeric-owner \
        -C "${TEMP_ROOT}" -cf - "${LINUX_NAME}" | gzip -n > "${LINUX_ARCHIVE}"
    verify_manifest "${LINUX_ARCHIVE}" "${LINUX_NAME}" tar \
        "${COMMON_MANIFEST[@]}" Makefile setup.sh bin/imgui_mcp_app
    RELEASE_ARCHIVES+=("${LINUX_ARCHIVE}")
fi

if [[ "${WINDOWS_BUILT}" == true ]]; then
    WIN_NAME="imgui-mcp-${VERSION}-windows-x64"
    WIN_DIR="${TEMP_ROOT}/${WIN_NAME}"
    mkdir -p "${WIN_DIR}/bin"
    copy_common_files "${WIN_DIR}"
    cp "${SCRIPT_DIR}/setup.ps1" "${WIN_DIR}/"
    cp "${BUILD_WIN_DIR}/imgui_mcp_app.exe" "${WIN_DIR}/bin/"
    cp "${SCRIPT_DIR}/cross/bin/SDL2.dll" "${WIN_DIR}/bin/"
    cp "${SCRIPT_DIR}/release.sh" "${WIN_DIR}/"
    printf '%s\r\n' \
        '@echo off' \
        'echo imgui-mcp Windows Installer' \
        'echo ===========================' \
        'powershell -ExecutionPolicy Bypass -File "%~dp0setup.ps1" -SkipBuild' \
        'pause' > "${WIN_DIR}/install.bat"
    normalize_tree "${WIN_DIR}"
    WIN_ARCHIVE="${DIST_DIR}/${WIN_NAME}.zip"
    rm -f -- "${WIN_ARCHIVE}"
    (
        cd "${TEMP_ROOT}"
        find "${WIN_NAME}" -print | LC_ALL=C sort | zip -X -q "${WIN_ARCHIVE}" -@
    )
    verify_manifest "${WIN_ARCHIVE}" "${WIN_NAME}" zip \
        "${COMMON_MANIFEST[@]}" setup.ps1 install.bat release.sh \
        bin/imgui_mcp_app.exe bin/SDL2.dll
    RELEASE_ARCHIVES+=("${WIN_ARCHIVE}")
fi

echo "Release archives:"
printf '%s\n' "${RELEASE_ARCHIVES[@]}"
