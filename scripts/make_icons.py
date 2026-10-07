#!/usr/bin/env python3
# 產生 Discord 用的圖示（原創設計，不使用 foobar2000 官方 logo）。
# Copyright (C) 2026 Carinoasd
# SPDX-License-Identifier: MIT
#
# 用法：python3 scripts/make_icons.py   → 輸出到 assets/
# 先以 4 倍解析度繪製再縮小，讓邊緣平滑。

import math
import pathlib

from PIL import Image, ImageDraw

OUT = pathlib.Path(__file__).resolve().parent.parent / "assets"
SS = 4  # supersampling

TOP = (88, 70, 214)      # 靛紫
BOTTOM = (156, 64, 196)  # 紫紅
WHITE = (255, 255, 255, 255)
DARK = (32, 34, 46, 255)


def gradient(size: int) -> Image.Image:
    img = Image.new("RGBA", (size, size))
    px = img.load()
    for y in range(size):
        for x in range(size):
            t = (x * 0.35 + y * 0.65) / size
            px[x, y] = tuple(int(TOP[i] + (BOTTOM[i] - TOP[i]) * t) for i in range(3)) + (255,)
    return img


def rounded_mask(size: int, radius: int) -> Image.Image:
    mask = Image.new("L", (size, size), 0)
    ImageDraw.Draw(mask).rounded_rectangle((0, 0, size - 1, size - 1), radius=radius, fill=255)
    return mask


def draw_note_with_waves(draw: ImageDraw.ImageDraw, s: int) -> None:
    """八分音符 + 右側兩道聲波弧線。座標以 s（畫布邊長）為單位。"""
    # 符頭（傾斜的橢圓，用多邊形近似）
    cx, cy, rx, ry, tilt = 0.36 * s, 0.68 * s, 0.105 * s, 0.075 * s, math.radians(-22)
    pts = []
    for i in range(72):
        a = 2 * math.pi * i / 72
        x, y = rx * math.cos(a), ry * math.sin(a)
        pts.append((cx + x * math.cos(tilt) - y * math.sin(tilt), cy + x * math.sin(tilt) + y * math.cos(tilt)))
    draw.polygon(pts, fill=WHITE)
    # 符桿
    stem_x = cx + rx * 0.82
    draw.rectangle((stem_x - 0.028 * s, 0.22 * s, stem_x + 0.012 * s, cy - 0.02 * s), fill=WHITE)
    # 符尾
    flag = [
        (stem_x - 0.008 * s, 0.22 * s),
        (stem_x + 0.15 * s, 0.33 * s),
        (stem_x + 0.12 * s, 0.45 * s),
        (stem_x + 0.06 * s, 0.37 * s),
        (stem_x - 0.008 * s, 0.33 * s),
    ]
    draw.polygon(flag, fill=WHITE)
    # 聲波
    w = int(0.038 * s)
    for r in (0.16, 0.27):
        box = (0.60 * s - r * s, 0.50 * s - r * s, 0.60 * s + r * s, 0.50 * s + r * s)
        draw.arc(box, start=-50, end=50, fill=WHITE, width=w)


def app_icon(size: int, path: pathlib.Path, radius_ratio: float = 0.22) -> None:
    s = size * SS
    bg = gradient(s)
    canvas = Image.new("RGBA", (s, s), (0, 0, 0, 0))
    canvas.paste(bg, (0, 0), rounded_mask(s, int(s * radius_ratio)))
    draw_note_with_waves(ImageDraw.Draw(canvas), s)
    canvas.resize((size, size), Image.LANCZOS).save(path, optimize=True)


def small_icon(kind: str, size: int, path: pathlib.Path) -> None:
    s = size * SS
    img = Image.new("RGBA", (s, s), (0, 0, 0, 0))
    d = ImageDraw.Draw(img)
    d.ellipse((0, 0, s - 1, s - 1), fill=DARK)
    if kind == "playing":
        d.polygon([(0.38 * s, 0.28 * s), (0.38 * s, 0.72 * s), (0.74 * s, 0.50 * s)], fill=WHITE)
    elif kind == "paused":
        d.rounded_rectangle((0.32 * s, 0.28 * s, 0.45 * s, 0.72 * s), radius=int(0.03 * s), fill=WHITE)
        d.rounded_rectangle((0.55 * s, 0.28 * s, 0.68 * s, 0.72 * s), radius=int(0.03 * s), fill=WHITE)
    elif kind == "stopped":
        d.rounded_rectangle((0.32 * s, 0.32 * s, 0.68 * s, 0.68 * s), radius=int(0.04 * s), fill=WHITE)
    img.resize((size, size), Image.LANCZOS).save(path, optimize=True)


def main() -> None:
    OUT.mkdir(exist_ok=True)
    app_icon(1024, OUT / "app-icon.png")          # 可上傳到 Discord Developer Portal
    app_icon(512, OUT / "no-art.png", 0.0)        # 沒有封面時的大圖（Discord 會自行裁成圓角）
    for kind in ("playing", "paused", "stopped"):
        small_icon(kind, 256, OUT / f"{kind}.png")
    for p in sorted(OUT.glob("*.png")):
        print(p.name, p.stat().st_size)


if __name__ == "__main__":
    main()
