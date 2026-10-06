#!/usr/bin/env python3
# 從 CHANGELOG.md 取出指定版本的段落當作 Release 說明，並檢查 tag 與 VERSION 一致。
# Copyright (C) 2026 Carinoasd
# SPDX-License-Identifier: MIT
#
# 用法：release_notes.py v0.1.0

import pathlib
import re
import sys

root = pathlib.Path(__file__).resolve().parent.parent
tag = sys.argv[1]
version = tag.removeprefix("v")

file_version = (root / "VERSION").read_text().strip()
if file_version != version:
    sys.exit(f"tag {tag} 與 VERSION 檔的 {file_version} 不一致")

text = (root / "CHANGELOG.md").read_text(encoding="utf-8")
match = re.search(r"^## \[" + re.escape(version) + r"\].*?$(.*?)(?=^## \[|\Z)", text, re.MULTILINE | re.DOTALL)
if not match:
    sys.exit(f"CHANGELOG.md 中找不到 {version} 的區段")

body = re.sub(r"(?:^\[[^\]]+\]:[^\n]*\n?)+\Z", "", match.group(1).strip(), flags=re.MULTILINE).strip()
if not body:
    sys.exit(f"CHANGELOG.md 中 {version} 的區段是空的")
print(body)
