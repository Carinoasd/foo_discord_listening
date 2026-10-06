#!/usr/bin/env bash
# 在 Linux/WSL 上執行不依賴 foobar2000 的單元測試。
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
OUT=$(mktemp -d)
trap 'rm -rf "$OUT"' EXIT
JSON_DIR="$ROOT/.local/json"
if [ ! -f "$JSON_DIR/nlohmann/json.hpp" ]; then
    mkdir -p "$JSON_DIR/nlohmann"
    curl -sfLo "$JSON_DIR/nlohmann/json.hpp" https://github.com/nlohmann/json/releases/download/v3.12.0/json.hpp
fi
g++ -std=c++20 -Wall -Wextra -Werror -O1 -fsanitize=address,undefined \
    -I"$ROOT/tests/stub" -I"$ROOT/src" -I"$JSON_DIR" \
    "$ROOT/src/discord/activity.cpp" "$ROOT/tests/activity_test.cpp" -o "$OUT/activity_test"
"$OUT/activity_test"
