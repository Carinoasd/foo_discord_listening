#!/usr/bin/env python3
# 產生 README 用的示意動圖（仿 Discord 狀態卡片；歌名與封面皆為虛構）。
# Copyright (C) 2026 Carinoasd
# SPDX-License-Identifier: MIT
#
# 用法：python3 scripts/make_demo_gif.py <字型目錄> → 輸出 docs/demo.gif

import math
import pathlib
import sys

from PIL import Image, ImageDraw, ImageFont

ROOT = pathlib.Path(__file__).resolve().parent.parent
FONT_DIR = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else "/mnt/c/Windows/Fonts")
W, H, S = 600, 196, 2  # 以 2 倍繪製再縮小

BG = (43, 45, 49)
CARD = (35, 36, 40)
WHITE = (242, 243, 245)
GREY = (181, 186, 193)
DIM = (128, 132, 142)
BAR_BG = (78, 80, 88)


def font(name: str, size: int) -> ImageFont.FreeTypeFont:
    return ImageFont.truetype(str(FONT_DIR / name), size * S)


F_HEAD = font("segoeuib.ttf", 11)
F_TITLE = font("segoeuib.ttf", 15)
F_TEXT = font("segoeui.ttf", 13)
F_SMALL = font("segoeui.ttf", 11)
F_NOTE = ImageFont.truetype(str(FONT_DIR / "msjh.ttc"), 10 * S)  # 「示意圖」需要中文字形

ICON = Image.open(ROOT / "assets" / "app-icon.png").convert("RGBA")
SMALL = {k: Image.open(ROOT / "assets" / f"{k}.png").convert("RGBA") for k in ("playing", "paused")}
NO_ART = Image.open(ROOT / "assets" / "no-art.png").convert("RGBA")


def fake_cover(seed: int, size: int) -> Image.Image:
    """虛構的抽象封面：同心圓與漸層。"""
    img = Image.new("RGBA", (size, size))
    px = img.load()
    hues = [(255, 140, 90), (90, 170, 255)][seed]
    for y in range(size):
        for x in range(size):
            d = math.hypot(x - size * 0.35, y - size * 0.4) / size
            r = 0.5 + 0.5 * math.sin(d * 18 + seed)
            px[x, y] = tuple(int(c * (0.35 + 0.65 * r)) for c in hues) + (255,)
    return img


COVERS = [fake_cover(0, 96 * S), fake_cover(1, 96 * S)]
TRACKS = [
    ("Midnight Signal", "The Example Band", "Placeholder Nights", 214),
    ("Paper Satellites", "Demo Orchestra", "Imaginary Album", 187),
]


def rounded(img: Image.Image, radius: int) -> Image.Image:
    mask = Image.new("L", img.size, 0)
    ImageDraw.Draw(mask).rounded_rectangle((0, 0, img.size[0] - 1, img.size[1] - 1), radius=radius, fill=255)
    out = Image.new("RGBA", img.size, (0, 0, 0, 0))
    out.paste(img, (0, 0), mask)
    return out


def mmss(seconds: float) -> str:
    seconds = int(seconds)
    return f"{seconds // 60}:{seconds % 60:02d}"


def frame(track: int, elapsed: float, paused: bool, cover: Image.Image) -> Image.Image:
    img = Image.new("RGB", (W * S, H * S), BG)
    d = ImageDraw.Draw(img)
    d.rounded_rectangle((12 * S, 12 * S, (W - 12) * S, (H - 12) * S), radius=10 * S, fill=CARD)
    d.text((28 * S, 22 * S), "LISTENING TO FOOBAR2000", font=F_HEAD, fill=GREY)

    art = rounded(cover.resize((96 * S, 96 * S)), 8 * S)
    img.paste(art, (28 * S, 46 * S), art)
    small = SMALL["paused" if paused else "playing"].resize((28 * S, 28 * S))
    ring = Image.new("RGBA", (34 * S, 34 * S), (0, 0, 0, 0))
    ImageDraw.Draw(ring).ellipse((0, 0, 34 * S - 1, 34 * S - 1), fill=CARD + (255,))
    ring.paste(small, (3 * S, 3 * S), small)
    img.paste(ring, (104 * S, 122 * S), ring)

    title, artist, album, length = TRACKS[track]
    x = 140 * S
    d.text((x, 50 * S), title + (" (Paused)" if paused else ""), font=F_TITLE, fill=WHITE)
    d.text((x, 74 * S), artist, font=F_TEXT, fill=GREY)
    d.text((x, 94 * S), album, font=F_TEXT, fill=GREY)
    if not paused:
        bar_y, x0, x1 = 128 * S, x + 40 * S, (W - 70) * S
        d.text((x, bar_y - 8 * S), mmss(elapsed), font=F_SMALL, fill=GREY)
        d.text((x1 + 10 * S, bar_y - 8 * S), mmss(length), font=F_SMALL, fill=GREY)
        d.rounded_rectangle((x0, bar_y - 2 * S, x1, bar_y + 2 * S), radius=2 * S, fill=BAR_BG)
        d.rounded_rectangle((x0, bar_y - 2 * S, x0 + int((x1 - x0) * elapsed / length), bar_y + 2 * S), radius=2 * S, fill=WHITE)
    d.text(((W - 120) * S, (H - 30) * S), "Illustration / 示意圖", font=F_NOTE, fill=DIM)
    return img.resize((W, H), Image.LANCZOS)


def main() -> None:
    frames, durations = [], []
    t = 71.0
    for _ in range(16):  # 播放中，進度條前進
        frames.append(frame(0, t, False, COVERS[0])); durations.append(250); t += 1
    for _ in range(10):  # 暫停
        frames.append(frame(0, t, True, COVERS[0])); durations.append(250)
    frames.append(frame(1, 0, False, NO_ART)); durations.append(900)  # 換曲：封面還在查詢，先顯示預設圖
    t = 1.0
    for _ in range(16):
        frames.append(frame(1, t, False, COVERS[1])); durations.append(250); t += 1
    # 從各段代表畫面拼出共用調色盤（兩張封面的顏色都要涵蓋），避免逐格閃爍並縮小檔案
    sample = Image.new("RGB", (W, H * 4))
    for i, k in enumerate((0, 16, 26, len(frames) - 1)):  # 播放、暫停、預設圖、第二首
        sample.paste(frames[k], (0, H * i))
    palette = sample.convert("P", palette=Image.ADAPTIVE, colors=224)
    out = [f.quantize(palette=palette, dither=Image.Dither.NONE) for f in frames]
    path = ROOT / "docs" / "demo.gif"
    out[0].save(path, save_all=True, append_images=out[1:], duration=durations, loop=0, optimize=True)
    print(path, path.stat().st_size, "bytes,", len(out), "frames")


if __name__ == "__main__":
    main()
