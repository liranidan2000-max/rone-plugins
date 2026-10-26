"""Brand phase B: give a plugin the shared RONE UI fonts (Shared/fonts) instead of Google Fonts.

    python brand/apply_fonts.py <plugin folder> --target <CMake target> --page <file> [--page <file> ...]

Does the three mechanical steps of the recipe in brand/README.md, as text:
  1. CMakeLists.txt: include Shared/fonts/RoneFonts.cmake and rone_add_fonts(<target>)
     right before the target's first top-level target_link_libraries.
  2. The editor (the .cpp defining ::getResource): #include "../../Shared/RoneFonts.h" and
     `if (auto font = roneFontResource (url)) return font;` as the first line of getResource.
  3. Each page: the fonts.googleapis.com <link> becomes a <style> with Shared/fonts/rone-fonts.css
     (plus rone-fonts-hardware.css with --hardware: Archivo Narrow, Roboto, Roboto Condensed;
     then name the files it uses after the target in rone_add_fonts). In a WebUI.h the CSS goes
     into its own raw string, so no literal grows past MSVC's 16 KB limit.
Labels (step 4) are a judgement per plugin and stay manual. Idempotent: a step already done is skipped.
"""
import argparse
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FONTS = os.path.join(ROOT, "Shared", "fonts")
HARDWARE = False
LINK = re.compile(r'[ \t]*<link[^>]*fonts\.googleapis\.com[^>]*>[ \t]*\r?\n?')
PRECONNECT = re.compile(r'[ \t]*<link[^>]*rel="preconnect"[^>]*fonts\.(googleapis|gstatic)\.com[^>]*>[ \t]*\r?\n?')


def read(path):
    with open(path, encoding="utf-8", newline="") as f:
        return f.read()


def write(path, text):
    with open(path, "w", encoding="utf-8", newline="") as f:
        f.write(text)


def eol(text):
    return "\r\n" if "\r\n" in text else "\n"


def css_for(page_text, nl):
    css = read(os.path.join(FONTS, "rone-fonts.css")).replace("\r\n", "\n").strip()
    if HARDWARE:
        css += "\n" + read(os.path.join(FONTS, "rone-fonts-hardware.css")).replace("\r\n", "\n").strip()
    return css.replace("\n", nl)


def do_page(path):
    text = read(path)
    m = LINK.search(text)
    if not m:
        return "page: no Google Fonts link (done already?)"
    nl = eol(text)
    css = css_for(text, nl)
    if path.endswith(".h"):
        # inside a raw string: close it, add the CSS as its own std::string, reopen with the same delimiter
        opens = list(re.finditer(r'R"([A-Za-z0-9_]*)\(', text[:m.start()]))
        if not opens:
            sys.exit(f"{path}: the link is not inside a raw string")
        d = opens[-1].group(1)
        new = (f'){d}") + std::string (R"{d}(<style>{nl}{css}{nl}</style>{nl}){d}")'
               f' + std::string (R"{d}(')
    else:
        new = f"<style>{nl}{css}{nl}</style>{nl}"
    text = text[:m.start()] + new + text[m.end():]
    text = PRECONNECT.sub("", text)
    write(path, text)
    return "page: Google Fonts link -> bundled @font-face"


def do_cmake(folder, target):
    path = os.path.join(folder, "CMakeLists.txt")
    text = read(path)
    if "rone_add_fonts" in text:
        return "cmake: already"
    nl = eol(text)
    m = re.search(rf"^target_link_libraries\s*\(\s*{re.escape(target)}\b", text, re.M)
    if not m:
        sys.exit(f"{path}: no top-level target_link_libraries({target}")
    block = (f"# The RONE UI fonts built into the binary (brand phase B, Shared/fonts){nl}"
             f"include(${{CMAKE_CURRENT_SOURCE_DIR}}/../Shared/fonts/RoneFonts.cmake){nl}"
             f"rone_add_fonts({target}){nl}{nl}")
    write(path, text[:m.start()] + block + text[m.start():])
    return "cmake: rone_add_fonts"


def do_editor(folder):
    cands = []
    for dp, dns, fns in os.walk(folder):
        dns[:] = [x for x in dns if x not in ("JUCE", "build", ".git", "tests", "mockup")]
        cands += [os.path.join(dp, f) for f in fns if f.endswith(".cpp")]
    for path in cands:
        text = read(path)
        m = re.search(r"::getResource\s*\(\s*const juce::String\s*&\s*url\s*\)\s*\{", text)
        if not m:
            continue
        if "roneFontResource" in text:
            return "editor: already"
        nl = eol(text)
        # the plugin folder sits in the monorepo root beside Shared/
        sub = os.path.relpath(os.path.dirname(path), folder)
        depth = 0 if sub == "." else len(sub.split(os.sep))
        rel = "../" * (depth + 1) + "Shared/RoneFonts.h"
        text = (text[:m.end()] + f"{nl}    if (auto font = roneFontResource (url))   // the bundled UI fonts (Shared/fonts){nl}"
                f"        return font;{nl}" + text[m.end():])
        incs = list(re.finditer(r'^#include [<"][^>"]+[>"][^\n]*\n', text, re.M))
        at = incs[-1].end() if incs else 0
        text = text[:at] + f'#include "{rel}"{nl}' + text[at:]
        write(path, text)
        return f"editor: {os.path.relpath(path, folder)}"
    sys.exit(f"{folder}: no ::getResource (const juce::String& url) found")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("folder")
    ap.add_argument("--target", required=True)
    ap.add_argument("--page", action="append", default=[])
    ap.add_argument("--hardware", action="store_true", help="also the hardware faces (rone-fonts-hardware.css)")
    a = ap.parse_args()
    global HARDWARE
    HARDWARE = a.hardware
    print(do_cmake(a.folder, a.target))
    print(do_editor(a.folder))
    for p in a.page:
        print(do_page(os.path.join(a.folder, p)))


if __name__ == "__main__":
    main()
