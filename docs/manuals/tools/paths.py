"""Shared locations for the manual tooling (docs/manuals/tools/*).

Layout inside the monorepo:
  docs/manuals/<Product> - User Manual.pdf   the shipped manuals (committed; installers copy them)
  docs/manuals/tools/                         this tooling (committed)
  docs/manuals/work/                          regenerated: extracted UIs, screenshots, images, HTML (git-ignored)
"""
import pathlib, os

TOOLS = pathlib.Path(__file__).resolve().parent
DOCS = TOOLS.parent                      # docs/manuals
ROOT = DOCS.parent.parent                # monorepo root
WORK = pathlib.Path(os.environ.get("RONE_MANUALS_WORK", DOCS / "work"))
UI = WORK / "ui"                         # served plugin UIs (+ harness, mocks)
SHOTS = WORK / "shots"                   # raw screenshots + DOM rects
DOCIMG = WORK / "docimg"                 # annotated PNGs and crops
DOCIMG_JPG = WORK / "docimg_jpg"         # JPEG copies embedded in the PDFs
HTML = WORK / "html"                     # generated manual HTML (open in a browser to proofread)
OUT = DOCS                               # the PDFs
EDGE = os.environ.get("RONE_EDGE", r"C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe")
PORT = int(os.environ.get("RONE_MANUALS_PORT", "8765"))
FONT = r"C:\Windows\Fonts\segoeuib.ttf"

def ensure():
    for d in (WORK, UI, SHOTS, DOCIMG, DOCIMG_JPG, HTML):
        d.mkdir(parents=True, exist_ok=True)
