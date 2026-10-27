"""Photograph a running WebView2 app, and measure it while you are in there.

The analyzer's UI is an HTML document now, so its manual shot no longer has to
come from a window capture. Two things follow from that, and both are worth
having:

  * The picture is taken by the view's own debugger, so it is exact. The old
    PrintWindow route returns solid black for WebView2 and the pipeline would
    annotate that black rectangle and ship it without complaining.

  * The callout boxes can be measured from the live DOM instead of typed in by
    hand. Eighteen hand-placed rectangles at one fixed capture size is a thing
    that goes quietly wrong the first time anybody moves a control; a selector
    keeps pointing at the control it names.

The viewport is forced to a fixed size so the shot is the same on any machine,
whatever monitor or DPI the build happens to run on.

The app is launched as a SECOND instance in its own WebView2 profile
(RONE_ANALYZER_SECOND_INSTANCE=1 + WEBVIEW2_USER_DATA_FOLDER): the manual can
then be rebuilt while the analyzer somebody is actually using stays open, and
the debug port is ours alone. Only the process this module started is ended.
"""
from __future__ import annotations

import json
import os
import subprocess
import sys
import tempfile
import time
from pathlib import Path

import cdp

DEBUG_PORT = 9339          # not 9222: leaves a developer's own browser alone
VIEW_W, VIEW_H = 1600, 860
SCALE = 2                  # device pixels per CSS pixel in the saved image

# What the picture must never carry: the Center's update bar (it is injected
# into every RONE page when a newer version exists, so a manual shot taken a
# day after a release would print "x.y.z is available" across the header), and
# the NO SIGNAL veil over an analyser that has not been fed yet.
HIDE_FOR_SHOT = ("#rone-upd-bar", "#rone-upd-lock")


def capture(exe: str, out_png: Path, selectors: dict[str, str] | None = None,
            out_rects: Path | None = None, settle: float = 6.0,
            args: list[str] | None = None, pose: str | None = None) -> bool:
    """Launches `exe` (with `args`), photographs its web view, and optionally measures it.

    `selectors` maps a CSS selector to the label the manual should print beside
    it. The rectangles land in `out_rects` as JSON, in the saved image's own
    pixels, ready for annotate.py.

    `pose` is JavaScript run in the page after it has settled and before the
    shot - open a panel, pick a mode - so the picture shows what the manual
    talks about.

    Returns False (loudly, but without raising) if the app could not be
    photographed, so a manual build on a machine without it still finishes.
    """
    env = dict(os.environ)
    env["WEBVIEW2_ADDITIONAL_BROWSER_ARGUMENTS"] = f"--remote-debugging-port={DEBUG_PORT}"
    env["RONE_ANALYZER_SECOND_INSTANCE"] = "1"
    profile = Path(tempfile.gettempdir()) / "rone_manual_wv2"
    profile.mkdir(parents=True, exist_ok=True)
    env["WEBVIEW2_USER_DATA_FOLDER"] = str(profile)

    proc = subprocess.Popen([exe, *(args or [])], env=env)

    conn = None
    try:
        target = cdp.wait_for_page(DEBUG_PORT, timeout=40)
        conn = cdp.Connection(target["webSocketDebuggerUrl"])

        # A fixed viewport, so the shot does not depend on the monitor it was
        # taken on and the hand-checked callout positions stay meaningful.
        conn.call("Emulation.setDeviceMetricsOverride",
                  {"width": VIEW_W, "height": VIEW_H,
                   "deviceScaleFactor": SCALE, "mobile": False})

        # Let the meters draw something. An analyser photographed the instant
        # it opens is a picture of an empty grid.
        time.sleep(settle)

        hide = ";".join(f"document.querySelectorAll({json.dumps(s)}).forEach(e => e.style.display='none')"
                        for s in HIDE_FOR_SHOT)
        conn.evaluate(f"(() => {{ {hide}; return true; }})()")
        if pose:
            conn.evaluate(f"(() => {{ {pose}; return true; }})()")
            time.sleep(1.5)                      # transitions, a repaint or two

        shot = conn.call("Page.captureScreenshot", {"format": "png"})
        out_png.parent.mkdir(parents=True, exist_ok=True)
        out_png.write_bytes(base64_decode(shot["data"]))

        if selectors and out_rects is not None:
            rects = measure(conn, selectors)
            out_rects.parent.mkdir(parents=True, exist_ok=True)
            out_rects.write_text(json.dumps(rects, indent=1), encoding="utf-8")
            missing = [s for s, r in rects.items() if r is None]
            if missing:
                print(f"  WARNING: selectors matched nothing: {', '.join(missing)}")
        return True

    except Exception as e:                                   # noqa: BLE001
        print(f"  capture failed: {e}")
        return False
    finally:
        if conn is not None:
            conn.close()
        # Only the instance this run started. A Get-Process sweep by name
        # would also end the analyzer the developer is working in.
        subprocess.run(["taskkill", "/F", "/T", "/PID", str(proc.pid)], capture_output=True)
        time.sleep(0.5)


def measure(conn: "cdp.Connection", selectors: dict[str, str]) -> dict:
    """Bounding boxes in image pixels, keyed by selector. A hidden element
    (display:none, or the `hidden` attribute) counts as absent."""
    expr = (
        "(() => {"
        f"  const sel = {json.dumps(list(selectors))};"
        f"  const k = {SCALE};"
        "   const out = {};"
        "   for (const s of sel) {"
        "     const el = document.querySelector(s);"
        "     if (!el) { out[s] = null; continue; }"
        "     const r = el.getBoundingClientRect();"
        "     if (!(r.width > 0 && r.height > 0)) { out[s] = null; continue; }"
        "     out[s] = [Math.round(r.left*k), Math.round(r.top*k),"
        "               Math.round(r.width*k), Math.round(r.height*k)];"
        "   }"
        "   return out;"
        "})()")
    return conn.evaluate(expr)


def base64_decode(s: str) -> bytes:
    import base64
    return base64.b64decode(s)


if __name__ == "__main__":
    # Handy for eyeballing: capture_webview.py <exe> <out.png> [app args...]
    ok = capture(sys.argv[1], Path(sys.argv[2]), args=sys.argv[3:])
    sys.exit(0 if ok else 1)
