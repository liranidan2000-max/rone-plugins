"""Brand phase B report: does each plugin page carry its own fonts, and are its labels readable?

    python brand/ui_check.py            # table for every plugin
    python brand/ui_check.py RoneClipper

Per plugin page it reports:
  fonts   bundled = the page has Shared/fonts/rone-fonts.css verbatim and the
          editor serves it (roneFontResource); stucker = Stucker's own older copy;
          google = it still depends on fonts.googleapis.com
  <9px    CSS rules with a font-size under 9 px (the floor on the design canvas)
  faint   CSS rules that set a font-size AND colour the text with var(--faint),
          which is 2.3:1 on the ground - decoration only, readable text is --dim or lighter
Report only; nothing is edited.
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CSS = open(os.path.join(ROOT, "Shared", "fonts", "rone-fonts.css"), encoding="utf-8").read().strip()

PLUGINS = ["ReverseReverbVST", "rone-flanger-", "RoneStutter", "RoneStucker", "RoneThrow", "RoneClipper",
           "RoneRise", "RoneIron", "RoneGater", "RONE AFTER SPACE", "Rone1176", "Rone2A", "RoneBassline",
           "RoneChoir", "RoneControl", "RoneDelay", "RoneGrain", "RoneLock", "RoneMotion", "rone sync verb",
           "RONEAnalyzer", "RoneStereo"]
PAGES = ["Source/ui/index.html", "Resources/ui/index.html", "Source/ui/index.base.html", "Source/WebUI.h",
         "Resources/ui/index.html"]


def page_of(folder):
    for rel in PAGES:
        p = os.path.join(ROOT, folder, rel)
        if os.path.exists(p):
            return p
    return None


def editor_serves_fonts(folder):
    src = os.path.join(ROOT, folder, "Source")
    if not os.path.isdir(src):
        return False
    for name in os.listdir(src):
        if name.endswith(".cpp"):
            text = open(os.path.join(src, name), encoding="utf-8", errors="replace").read()
            if "roneFontResource" in text:
                return True
    return False


def check(folder):
    page = page_of(folder)
    if not page:
        return folder, "-", "-", "-", "no page found"
    text = open(page, encoding="utf-8", errors="replace").read()
    if CSS in text.replace("\r\n", "\n") and editor_serves_fonts(folder):
        fonts = "bundled"
    elif CSS in text.replace("\r\n", "\n"):
        fonts = "css only"
    elif "fonts/sora-latin-" in text:
        fonts = "stucker"
    elif "fonts.googleapis.com" in text:
        fonts = "google"
    else:
        fonts = "?"
    if fonts in ("bundled", "stucker") and "fonts.googleapis.com" in text:
        fonts += "+google"
    small = faint = 0
    for rule in re.findall(r"\{([^{}]*)\}", text):
        m = re.search(r"font-size\s*:\s*([0-9.]+)px", rule)
        if not m:
            continue
        if float(m.group(1)) < 9:
            small += 1
        if re.search(r"(?<![-\w])color\s*:\s*var\(--faint\)", rule):
            faint += 1
    return folder, fonts, small, faint, os.path.relpath(page, ROOT)


def main():
    rows = [check(f) for f in (sys.argv[1:] or PLUGINS) if os.path.isdir(os.path.join(ROOT, f))]
    print(f"{'plugin':18} {'fonts':16} {'<9px':>5} {'faint':>6}  page")
    for r in rows:
        print(f"{r[0]:18} {r[1]:16} {str(r[2]):>5} {str(r[3]):>6}  {r[4]}")


if __name__ == "__main__":
    main()
