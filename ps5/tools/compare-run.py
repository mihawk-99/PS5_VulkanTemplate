#!/usr/bin/env python3
"""PS5 Vulkan Samples - a console run's pictures beside the host reference's.

    compare-run.py RUN_DIR HOST_DIR [ID...]    RUN_DIR/compare-<n>.png, three
                                               samples a sheet: console | host

Copyright (C) 2026 Mihawk
SPDX-License-Identifier: MIT
"""
import sys
from pathlib import Path

from PIL import Image, ImageChops, ImageDraw

run, host = Path(sys.argv[1]), Path(sys.argv[2])
names = sys.argv[3:] or sorted(p.stem for p in run.glob("*.png") if not p.stem.startswith(("sheet-", "compare-")))
W, H = 800, 450
for n in range(0, len(names), 3):
    sheet = Image.new("RGB", (W * 3, H * 3))
    for row, name in enumerate(names[n:n + 3]):
        a = Image.open(run / f"{name}.png").convert("RGB").resize((W, H), Image.LANCZOS)
        b = Image.open(host / f"{name}.png").convert("RGB").resize((W, H), Image.LANCZOS)
        d = ImageChops.difference(a, b).point(lambda v: min(255, v * 4))
        for col, (image, label) in enumerate(((a, "console"), (b, "host"), (d, "difference x4"))):
            draw = ImageDraw.Draw(image)
            draw.rectangle([0, H - 28, 360, H], fill=(0, 0, 0))
            draw.text((6, H - 24), f"{name}: {label}", fill=(255, 255, 0))
            sheet.paste(image, (col * W, row * H))
    sheet.save(run / f"compare-{n // 3 + 1}.png")
    print(run / f"compare-{n // 3 + 1}.png")
