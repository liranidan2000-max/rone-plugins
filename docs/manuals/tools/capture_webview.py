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
"""
from __future__ import annotations

import json
import os
import subprocess
import sys
import time
from pathlib import Path

import cdp

DEBUG_PORT = 9339          # not 9222: leaves a developer's own browser alone
VIEW_W, VIEW_H = 1600, 860
SCALE = 2                  # device pixels per CSS pixel in the saved image


def _stop(exe_name: str) -> None:
    subprocess.run(["powershell", "-NoProfile", "-Command",
                    f"Get-Process | Where-Object {{ $_.ProcessName -like '{exe_name}*' }} "
                    "| Stop-Process -Force"],
                   capture_output=True)


def capture(exe: str, out_png: Path, selectors: dict[str, str] | None = None,
            out_rects: Path | None = None, settle: float = 6.0) -> bool:
    """Launches `exe`, photographs its web view, and optionally measures it.

    `selectors` maps a CSS selector to the label the manual should print beside
    it. The rectangles land in `out_rects` as JSON, in the saved image's own
    pixels, ready for annotate.py.

    Returns False (loudly, but without raising) if the app could not be
    photographed, so a manual build on a machine without it still finishes.
    """
    exe_name = Path(exe).stem
    _stop(exe_name)
    time.sleep(1.0)

    env = dict(os.environ)
    env["WEBVIEW2_ADDITIONAL_BROWSER_ARGUMENTS"] = f"--remote-debugging-port={DEBUG_PORT}"
    proc = subprocess.Popen([exe], env=env)

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
        proc.terminate()
        time.sleep(0.5)
        _stop(exe_name)


def measure(conn: "cdp.Connection", selectors: dict[str, str]) -> dict:
    """Bounding boxes in image pixels, keyed by selector."""
    expr = (
        "(() => {"
        f"  const sel = {json.dumps(list(selectors))};"
        f"  const k = {SCALE};"
        "   const out = {};"
        "   for (const s of sel) {"
        "     const el = document.querySelector(s);"
        "     if (!el) { out[s] = null; continue; }"
        "     const r = el.getBoundingClientRect();"
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
    # Handy for eyeballing: capture_webview.py <exe> <out.png>
    ok = capture(sys.argv[1], Path(sys.argv[2]))
    sys.exit(0 if ok else 1)
