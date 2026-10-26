"""Spread brand/brand.json to every surface that carries a product colour.

    python brand/brand_sync.py           # report drift, exit 1 if any
    python brand/brand_sync.py --write   # fix it in place

Targets (each one is a hand-kept copy that used to drift on its own):
  versions.json                       accent per plugin entry (colours the Center card)
  RonePluginsCenter/.../catalog.js    ACCENTS fallback table
  <Plugin>/dev-channel.json           accent of the admin-only DEV card
  docs/manuals/tools/content_*.py     print colour of the PDF manual
  ../WEBSITE RONE AUDIO/assets        site.css --c-*, fx.js hero units, account.js rack, admin.js dots
  ../WEBSITE RONE AUDIO/קידום/instagram/build.mjs   ACC table of the social kit

Plugin UIs are not touched: each one changes with its own release.
Files are edited as text (one line or one block), never re-serialised, so
other sessions' uncommitted edits in the same file survive.
"""
import json
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
SITE = os.path.join(os.path.dirname(ROOT), "WEBSITE RONE AUDIO")
BRAND = json.load(open(os.path.join(ROOT, "brand", "brand.json"), encoding="utf-8"))
P = BRAND["products"]
HEX = re.compile(r"#[0-9A-Fa-f]{6}")

drift = []      # (file, what, old, new)
missing = []    # (file, what)


def rel(path):
    return os.path.relpath(path, os.path.dirname(ROOT))


def rgb(h):
    n = int(h[1:], 16)
    return f"{(n >> 16) & 255},{(n >> 8) & 255},{n & 255}"


def read(path):
    with open(path, encoding="utf-8", newline="") as f:
        return f.read()


def write(path, text):
    with open(path, "w", encoding="utf-8", newline="") as f:
        f.write(text)


def nl(text):
    return "\r\n" if "\r\n" in text else "\n"


def replace_block(path, start_re, end_re, body, label, text=None):
    """Swap everything between a start line and an end line for `body`."""
    text = read(path) if text is None else text
    m = re.search(start_re, text)
    if not m:
        missing.append((rel(path), label + " (block not found)"))
        return text
    e = re.compile(end_re).search(text, m.end())
    old = text[m.end():e.start()]
    body = body.replace("\n", nl(text))
    if old != body:
        drift.append((rel(path), label, "(block)", f"{len(P)} products"))
        text = text[:m.end()] + body + text[e.start():]
    return text


# ---- versions.json: one "accent" line per plugin entry ------------------------------------------
def sync_versions():
    path = os.path.join(ROOT, "versions.json")
    text = read(path)
    for pid, p in P.items():
        m = re.search(r'"id":\s*"' + re.escape(pid) + r'"', text)
        if not m:
            continue                      # not released yet: nothing to colour
        nxt = re.compile(r'\n\s*\{\s*\r?\n\s*"id":').search(text, m.end())
        stop = nxt.start() if nxt else len(text)
        a = re.compile(r'("accent":\s*")(#[0-9A-Fa-f]{6})(")').search(text, m.end(), stop)
        if not a:
            missing.append((rel(path), f"{pid} has no accent"))
            continue
        if a.group(2).upper() != p["screen"].upper():
            drift.append((rel(path), pid, a.group(2), p["screen"]))
            text = text[:a.start(2)] + p["screen"] + text[a.end(2):]
    return {path: text}


# ---- Center fallback table ----------------------------------------------------------------------
def sync_center():
    path = os.path.join(ROOT, "RonePluginsCenter", "Resources", "ui", "src", "catalog.js")
    items = [f"{k}: '{v['screen']}'" for k, v in P.items()]
    lines, row = [], []
    for it in items:
        row.append(it)
        if len(row) == 4:
            lines.append("  " + ", ".join(row) + ",")
            row = []
    if row:
        lines.append("  " + ", ".join(row) + ",")
    body = "\n" + "\n".join(lines) + "\n"
    return {path: replace_block(path, r"export const ACCENTS = \{[^\r\n]*", r"\}", body, "ACCENTS")}


# ---- DEV channel cards ----------------------------------------------------------------------------
def sync_dev():
    out = {}
    for pid, p in P.items():
        folder = p.get("dev_folder")
        if not folder:
            continue
        path = os.path.join(ROOT, folder, "dev-channel.json")
        if not os.path.exists(path):
            missing.append((rel(path), "file not found"))
            continue
        text = read(path)
        a = re.search(r'("accent":\s*")(#[0-9A-Fa-f]{6})', text)
        if not a:
            missing.append((rel(path), "no accent"))
            continue
        if a.group(2).upper() != p["screen"].upper():
            drift.append((rel(path), pid, a.group(2), p["screen"]))
            out[path] = text[:a.start(2)] + p["screen"] + text[a.end(2):]
    return out


# ---- PDF manuals: print colour -------------------------------------------------------------------
def sync_manuals():
    out = {}
    for pid, p in P.items():
        name = p.get("manual")
        if not name:
            continue
        path = os.path.join(ROOT, "docs", "manuals", "tools", f"content_{name}.py")
        if not os.path.exists(path):
            missing.append((rel(path), "file not found"))
            continue
        want = p.get("print", p["screen"])
        text = read(path)
        a = re.search(r'("accent":\s*")(#[0-9A-Fa-f]{6})', text)
        if not a:
            missing.append((rel(path), "no accent"))
            continue
        if a.group(2).upper() != want.upper():
            drift.append((rel(path), pid + " (print)", a.group(2), want))
            out[path] = text[:a.start(2)] + want + text[a.end(2):]
    return out


# ---- the site ----------------------------------------------------------------------------------
def sync_site():
    out = {}
    css = os.path.join(SITE, "assets", "site.css")
    width = max(len(p["key"]) for p in P.values())
    body = "    /* product neons - generated from rone-plugins/brand/brand.json, edit there */\n" + "\n".join(
        f"    --c-{p['key']}:{' ' * (width - len(p['key']) + 1)}{p['screen']};" for p in P.values()
    )
    out[css] = replace_block(css, r"(?=    /\* product neons)|(?=    --c-reverb:)",
                             r"\r?\n[ \t]*\r?\n", body, "--c-* block")
    # product pages: <body class="acc-throw"> scopes, whose glow rgba once stayed on an old colour
    body = "\n".join(
        f".acc-{p['key']}{' ' * (width - len(p['key']) + 1)}{{ --kn: var(--c-{p['key']});{' ' * (width - len(p['key']) + 1)}"
        f"--kn-glow: rgba({rgb(p['screen'])},0.5); --kn-soft: rgba({rgb(p['screen'])},0.14); }}" for p in P.values()
    )
    out[css] = replace_block(css, r"(?=\.acc-reverb )", r"\r?\n[ \t]*\r?\n", body, ".acc-* scopes", text=out[css])

    fx = os.path.join(SITE, "assets", "fx.js")
    text = read(fx)
    for pid, p in P.items():
        m = re.search(r"slug:'" + re.escape(p["slug"]) + r"'[^\n]*?accent:'(#[0-9A-Fa-f]{6})',\s*rgb:'([0-9,]+)'", text)
        if m and (m.group(1).upper() != p["screen"].upper() or m.group(2) != rgb(p["screen"])):
            drift.append((rel(fx), p["slug"], m.group(1), p["screen"]))
            text = text[:m.start(1)] + p["screen"] + text[m.end(1):m.start(2)] + rgb(p["screen"]) + text[m.end(2):]
    out[fx] = text

    acc = os.path.join(SITE, "assets", "account.js")
    text = read(acc)
    for pid, p in P.items():
        m = re.search(r"\{ id: '" + re.escape(pid) + r"'[^\n]*?accent: '(#[0-9A-Fa-f]{6})'", text)
        if m and m.group(1).upper() != p["screen"].upper():
            drift.append((rel(acc), pid, m.group(1), p["screen"]))
            text = text[:m.start(1)] + p["screen"] + text[m.end(1):]
    out[acc] = text

    adm = os.path.join(SITE, "assets", "admin.js")
    body = "\n" + "".join(f"  {k if k.isidentifier() else repr(k)}: '{v['screen']}',\n" for k, v in P.items()) \
        + "  bundle: '#B99BFF',\n"
    out[adm] = replace_block(adm, r"export const BRAND = \{[^\r\n]*", r"\};", body, "BRAND")
    return out


# ---- social kit -----------------------------------------------------------------------------------
def sync_social():
    path = os.path.join(SITE, "קידום", "instagram", "build.mjs")
    width = max(len(p["key"]) for p in P.values())
    rows = []
    for p in P.values():
        k = p["key"] if p["key"].isidentifier() else f"'{p['key']}'"
        c = rgb(p["screen"])
        pad = " " * (width + 2 - len(k))
        rows.append(f"  {k}:{pad}{{ kn: '{p['screen']}', glow: 'rgba({c},.5)', soft: 'rgba({c},.14)' }},\n")
    body = "\n" + "".join(rows)
    return {path: replace_block(path, r"const ACC = \{[^\r\n]*", r"\};", body, "ACC")}


def main():
    do_write = "--write" in sys.argv
    edits = {}
    for fn in (sync_versions, sync_center, sync_dev, sync_manuals, sync_site, sync_social):
        edits.update(fn())
    for f, what, old, new in drift:
        print(f"DRIFT   {f}: {what} {old} -> {new}")
    for f, what in missing:
        print(f"MISSING {f}: {what}")
    if not drift:
        print(f"OK - every surface matches brand.json ({len(P)} products)")
    if do_write and drift:
        changed = 0
        for path, text in edits.items():
            if text != read(path):
                write(path, text)
                changed += 1
        print(f"wrote {changed} files")
    return 1 if drift and not do_write else 0


if __name__ == "__main__":
    sys.exit(main())
