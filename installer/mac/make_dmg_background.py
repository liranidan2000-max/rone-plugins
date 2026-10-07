"""Draws the Plugins Center .dmg window background (2.0.6): the app on the
left, an arrow, the Applications link on the right, and one line that says
what to do. Light on purpose: Finder writes the icon names in black.

    python installer/mac/make_dmg_background.py

Writes dmg-background.png (640x400) and dmg-background@2x.png (1280x800).
scripts/ci/make-center-dmg.sh places the icons at the matching points:
the app at (170, 190), Applications at (470, 190).
"""
import os
from PIL import Image, ImageDraw, ImageFont

HERE = os.path.dirname(os.path.abspath(__file__))
FONTS = r'C:\Windows\Fonts'
PURPLE = (157, 107, 255)


def font(name, size):
    return ImageFont.truetype(os.path.join(FONTS, name), size)


def draw(scale):
    W, H = 640 * scale, 400 * scale
    img = Image.new('RGB', (W, H))
    d = ImageDraw.Draw(img)
    for y in range(H):                                   # a quiet top-to-bottom wash
        t = y / H
        c = tuple(int(a + (b - a) * t) for a, b in zip((247, 247, 249), (236, 236, 241)))
        d.line([(0, y), (W, y)], fill=c)
    s = lambda v: int(v * scale)

    d.text((s(28), s(24)), 'R O N E   P L U G I N S', font=font('segoeuib.ttf', s(11)), fill=PURPLE)
    title = 'Install the Plugins Center'
    f = font('segoeuib.ttf', s(22))
    d.text(((W - d.textlength(title, font=f)) / 2, s(54)), title, font=f, fill=(20, 22, 26))

    # the arrow between the two icons (icons are 112 px, centred on y = 190)
    y, x0, x1 = s(190), s(250), s(388)
    d.line([(x0, y), (x1 - s(16), y)], fill=PURPLE, width=s(7))
    d.polygon([(x1, y), (x1 - s(24), y - s(15)), (x1 - s(24), y + s(15))], fill=PURPLE)

    line1 = 'Drag RONE Plugins Center onto Applications'
    line2 = 'Then open it from your Applications folder'
    f1, f2 = font('segoeuib.ttf', s(15)), font('segoeui.ttf', s(12))
    d.text(((W - d.textlength(line1, font=f1)) / 2, s(318)), line1, font=f1, fill=(46, 49, 56))
    d.text(((W - d.textlength(line2, font=f2)) / 2, s(344)), line2, font=f2, fill=(122, 127, 136))
    return img


draw(1).save(os.path.join(HERE, 'dmg-background.png'))
draw(2).save(os.path.join(HERE, 'dmg-background@2x.png'))
print('written')
