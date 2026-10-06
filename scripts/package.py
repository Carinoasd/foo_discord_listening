#!/usr/bin/env python3
# 把建置好的 DLL 打包成 foobar2000 可直接安裝的 .fb2k-component。
# Copyright (C) 2026 Carinoasd
# SPDX-License-Identifier: MIT
#
# 用法：package.py --x64 path/to/x64.dll [--x86 path/to/x86.dll] --out dist/
# 32 位元 DLL 放在壓縮檔根目錄，64 位元放在 x64/，這是 foobar2000 2.x 的慣例。
# 只放 DLL：授權與第三方聲明已寫在元件的 About 文字中（Preferences > Components）。

import argparse
import pathlib
import zipfile

NAME = "foo_discord_listening"


# 編譯進 DLL 的 __FILE__ 或 PDB 路徑可能帶出建置機器的使用者資料夾名稱，發佈前一律擋下。
FORBIDDEN = [b"\\Users\\", b"/Users/", b"AppData", "\\Users\\".encode("utf-16-le"), "AppData".encode("utf-16-le")]


def check_no_local_paths(dll: pathlib.Path) -> None:
    data = dll.read_bytes()
    for pattern in FORBIDDEN:
        if pattern in data:
            raise SystemExit(f"{dll} 內含本機路徑（{pattern!r}），拒絕打包")


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--x64", type=pathlib.Path, required=True)
    parser.add_argument("--x86", type=pathlib.Path)
    parser.add_argument("--out", type=pathlib.Path, required=True)
    args = parser.parse_args()

    root = pathlib.Path(__file__).resolve().parent.parent
    version = (root / "VERSION").read_text().strip()
    args.out.mkdir(parents=True, exist_ok=True)
    target = args.out / f"{NAME}-{version}.fb2k-component"

    for dll in filter(None, [args.x64, args.x86]):
        check_no_local_paths(dll)

    with zipfile.ZipFile(target, "w", zipfile.ZIP_DEFLATED) as z:
        z.write(args.x64, f"x64/{NAME}.dll")
        if args.x86:
            z.write(args.x86, f"{NAME}.dll")

    print(target)


if __name__ == "__main__":
    main()
