#!/usr/bin/env python3
# 把建置好的 DLL 打包成 foobar2000 可直接安裝的 .fb2k-component。
# Copyright (C) 2026 Carinoasd
# SPDX-License-Identifier: MIT
#
# 用法：package.py --x64 path/to/x64.dll [--x86 path/to/x86.dll] --out dist/
# 32 位元 DLL 放在壓縮檔根目錄，64 位元放在 x64/，這是 foobar2000 2.x 的慣例。

import argparse
import pathlib
import zipfile

NAME = "foo_discord_listening"


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

    with zipfile.ZipFile(target, "w", zipfile.ZIP_DEFLATED) as z:
        z.write(args.x64, f"x64/{NAME}.dll")
        if args.x86:
            z.write(args.x86, f"{NAME}.dll")
        z.write(root / "LICENSE", "LICENSE")
        z.write(root / "THIRD_PARTY_NOTICES.md", "THIRD_PARTY_NOTICES.md")

    print(target)


if __name__ == "__main__":
    main()
