from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

out_dir = Path(__file__).resolve().parent
size = 215
bg = (18, 24, 32)
accent = (61, 214, 198)
steel = (154, 167, 181)
white = (237, 242, 247)

img = Image.new("RGB", (size, size), bg)
draw = ImageDraw.Draw(img)

margin = 18
draw.rounded_rectangle(
    (margin, margin, size - margin, size - margin),
    radius=28,
    fill=(23, 29, 37),
    outline=(42, 54, 68),
    width=2,
)

pts = [
    (48, 108),
    (72, 108),
    (86, 108),
    (98, 62),
    (112, 148),
    (126, 88),
    (140, 108),
    (168, 108),
]
draw.line(pts, fill=(30, 90, 85), width=10, joint="curve")
draw.line(pts, fill=accent, width=5, joint="curve")

bar_base = 168
bars = [(58, 18), (78, 28), (98, 22), (118, 34), (138, 26)]
for x, h in bars:
    draw.rounded_rectangle((x, bar_base - h, x + 12, bar_base), radius=3, fill=(45, 120, 112))

try:
    font = ImageFont.truetype(r"C:\Windows\Fonts\segoeui.ttf", 22)
    font_small = ImageFont.truetype(r"C:\Windows\Fonts\segoeui.ttf", 11)
except OSError:
    font = ImageFont.load_default()
    font_small = font

text = "StoreVitals"
bbox = draw.textbbox((0, 0), text, font=font)
tw = bbox[2] - bbox[0]
draw.text(((size - tw) / 2, 28), text, fill=white, font=font)

sub = "PARTNER CENTER"
bbox = draw.textbbox((0, 0), sub, font=font_small)
sw = bbox[2] - bbox[0]
draw.text(((size - sw) / 2, 52), sub, fill=steel, font=font_small)

logo_path = out_dir / "storepulse-logo-215.png"
img.save(logo_path, format="PNG", optimize=True)
print(f"wrote {logo_path} bytes={logo_path.stat().st_size}")

# 512 master for app/UI use
master_size = 512
scale = master_size / size
img512 = Image.new("RGB", (master_size, master_size), bg)
d = ImageDraw.Draw(img512)
m = int(18 * scale)
d.rounded_rectangle(
    (m, m, master_size - m, master_size - m),
    radius=int(28 * scale),
    fill=(23, 29, 37),
    outline=(42, 54, 68),
    width=4,
)
pts512 = [(int(x * scale), int(y * scale)) for x, y in pts]
d.line(pts512, fill=(30, 90, 85), width=22, joint="curve")
d.line(pts512, fill=accent, width=12, joint="curve")
bar_base512 = int(168 * scale)
for x, h in bars:
    x0 = int(x * scale)
    h0 = int(h * scale)
    w0 = int(12 * scale)
    d.rounded_rectangle(
        (x0, bar_base512 - h0, x0 + w0, bar_base512),
        radius=6,
        fill=(45, 120, 112),
    )
try:
    font_l = ImageFont.truetype(r"C:\Windows\Fonts\segoeui.ttf", 52)
    font_s = ImageFont.truetype(r"C:\Windows\Fonts\segoeui.ttf", 22)
except OSError:
    font_l = font
    font_s = font_small

bbox = d.textbbox((0, 0), text, font=font_l)
tw = bbox[2] - bbox[0]
d.text(((master_size - tw) / 2, int(28 * scale)), text, fill=white, font=font_l)
bbox = d.textbbox((0, 0), sub, font=font_s)
sw = bbox[2] - bbox[0]
d.text(((master_size - sw) / 2, int(52 * scale)), sub, fill=steel, font=font_s)

master_path = out_dir / "storepulse-logo-512.png"
img512.save(master_path, format="PNG", optimize=True)
print(f"wrote {master_path} bytes={master_path.stat().st_size}")
