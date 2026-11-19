#!/usr/bin/env python3
"""Generate Store tile / splash / .ico assets from branding/storepulse-logo-512.png."""

from __future__ import annotations

from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parents[1]
SRC = ROOT / "branding" / "storepulse-logo-512.png"
ASSETS = Path(__file__).resolve().parent / "Assets"
BRANDING = ROOT / "branding"

BG = (15, 20, 25, 255)  # #0f1419


def fit(img: Image.Image, size: tuple[int, int], pad_ratio: float = 0.12) -> Image.Image:
    canvas = Image.new("RGBA", size, BG)
    max_w = int(size[0] * (1.0 - 2 * pad_ratio))
    max_h = int(size[1] * (1.0 - 2 * pad_ratio))
    logo = img.copy()
    logo.thumbnail((max_w, max_h), Image.Resampling.LANCZOS)
    x = (size[0] - logo.size[0]) // 2
    y = (size[1] - logo.size[1]) // 2
    canvas.alpha_composite(logo, (x, y))
    return canvas


def main() -> None:
    if not SRC.exists():
        raise SystemExit(f"Missing logo: {SRC}")
    logo = Image.open(SRC).convert("RGBA")
    ASSETS.mkdir(parents=True, exist_ok=True)

    targets = {
        "StoreLogo.png": (50, 50),
        "Square44x44Logo.png": (44, 44),
        "Square71x71Logo.png": (71, 71),
        "Square150x150Logo.png": (150, 150),
        "Square310x310Logo.png": (310, 310),
        "Wide310x150Logo.png": (310, 150),
        "SplashScreen.png": (620, 300),
    }
    for name, size in targets.items():
        out = ASSETS / name
        fit(logo, size).save(out, "PNG")
        print("wrote", out)

    # Windows app icon (.ico) — PIL embeds the listed sizes from a master image.
    ico_path = BRANDING / "StorePulse.ico"
    master = fit(logo, (256, 256), pad_ratio=0.08)
    master.save(
        ico_path,
        format="ICO",
        sizes=[(16, 16), (24, 24), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)],
    )
    print("wrote", ico_path)


if __name__ == "__main__":
    main()
