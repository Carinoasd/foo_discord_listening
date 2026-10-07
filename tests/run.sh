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
build_and_run() {
    local name=$1
    shift
    g++ -std=c++20 -Wall -Wextra -Werror -O1 -fsanitize=address,undefined \
        -I"$ROOT/tests/stub" -I"$ROOT/src" -I"$JSON_DIR" \
        "$@" "$ROOT/tests/$name.cpp" -o "$OUT/$name"
    echo "== $name"
    "$OUT/$name"
}

build_and_run activity_test "$ROOT/src/discord/activity.cpp"
build_and_run musicbrainz_test "$ROOT/src/art/musicbrainz_query.cpp"
build_and_run uploader_test "$ROOT/src/art/uploader_output.cpp"
build_and_run providers_test "$ROOT/src/art/providers_query.cpp" "$ROOT/src/art/musicbrainz_query.cpp"
build_and_run import_rich_test "$ROOT/src/import_rich.cpp"
build_and_run update_test
