#!/usr/bin/env python3
"""PS5 Vulkan Template - a run's pictures on a few sheets, to look at together.

    contact-sheet.py RUN_DIR      RUN_DIR/sheet-<n>.png, six pictures each, named

Copyright (C) 2026 Mihawk
SPDX-License-Identifier: MIT
"""
import sys
from pathlib import Path

from PIL import Image, ImageDraw

run = Path(sys.argv[1])
pictures = sorted(p for p in run.glob("*.png") if not p.name.startswith("sheet-"))
W, H = 800, 450
for n in range(0, len(pictures), 6):
    sheet = Image.new("RGB", (W * 2, H * 3))
    for i, picture in enumerate(pictures[n:n + 6]):
        image = Image.open(picture).convert("RGB").resize((W, H), Image.LANCZOS)
        draw = ImageDraw.Draw(image)
        draw.rectangle([0, H - 28, 300, H], fill=(0, 0, 0))
        draw.text((6, H - 24), picture.stem, fill=(255, 255, 0))
        sheet.paste(image, ((i % 2) * W, (i // 2) * H))
    sheet.save(run / f"sheet-{n // 6 + 1}.png")
    print(run / f"sheet-{n // 6 + 1}.png")
