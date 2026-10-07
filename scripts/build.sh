#!/usr/bin/env bash
# 從 WSL 呼叫 Windows 端的 MSVC 建置（本機開發用，不進交付包）。
# 原始碼先同步到 Windows 本機磁碟再建置，避免 MSVC 處理 \\wsl.localhost 路徑的問題。
# 用法：scripts/build.sh [Release|Debug] [x64|x86]
# 環境變數 FDL_CMAKE_ARGS 可附加 CMake 參數，例如 FDL_CMAKE_ARGS=-DFDL_BUILD_TOOLS=ON
set -euo pipefail

ROOT=$(cd "$(dirname "$0")/.." && pwd)
CONFIG=${1:-Release}
ARCH=${2:-x64}
VCVARS=$([ "$ARCH" = x86 ] && echo vcvars32.bat || echo vcvars64.bat)

win_env() { (cd /mnt/c && cmd.exe /c "echo %$1%" 2>/dev/null | tr -d '\r'); }

WIN_BASE="$(win_env LOCALAPPDATA)\\fdl-build"
WSL_BASE=$(wslpath "$WIN_BASE")
VSWHERE="/mnt/c/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe"
VS_PATH=$("$VSWHERE" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath | tr -d '\r')
[ -n "$VS_PATH" ] || { echo "找不到含 MSVC 的 Visual Studio" >&2; exit 1; }

mkdir -p "$WSL_BASE/src"
rsync -a --delete --exclude .git --exclude build --exclude dist "$ROOT/" "$WSL_BASE/src/"

cat > "$WSL_BASE/build.cmd" <<CMD
@echo off
call "$VS_PATH\\VC\\Auxiliary\\Build\\$VCVARS" >nul || exit /b 1
cmake -S "%~dp0src" -B "%~dp0out-$ARCH-$CONFIG" -G Ninja -DCMAKE_BUILD_TYPE=$CONFIG ${FDL_CMAKE_ARGS:-} || exit /b 1
cmake --build "%~dp0out-$ARCH-$CONFIG" || exit /b 1
CMD

(cd /mnt/c && cmd.exe /c "$WIN_BASE\\build.cmd")

mkdir -p "$ROOT/build/$ARCH"
cp "$WSL_BASE/out-$ARCH-$CONFIG/foo_discord_listening.dll" "$ROOT/build/$ARCH/"
for tool in ipc_probe client_test; do
    if [ -f "$WSL_BASE/out-$ARCH-$CONFIG/$tool.exe" ]; then
        cp "$WSL_BASE/out-$ARCH-$CONFIG/$tool.exe" "$ROOT/build/$ARCH/"
    fi
done
echo "OK: build/$ARCH/foo_discord_listening.dll ($CONFIG)"
