# RONE user manuals

One PDF per product. The installers copy the PDF into the install folder
(Windows `C:\Program Files\RONE Plugins\Manuals\`, macOS `/Users/Shared/RONE
Plugins/Manuals/`) and the Plugins Center opens it from the card menu
(**Manual**), falling back to the copy served from this repository.

| Product | File |
|---|---|
| RONE Stutter | `RONE Stutter - User Manual.pdf` |
| RONE Stucker | `RONE Stucker - User Manual.pdf` |
| RONE Flanger | `RONE Flanger - User Manual.pdf` |
| RONE Reverse Reverb | `RONE Reverse Reverb - User Manual.pdf` |
| RONE AFTERSPACE | `RONE AFTERSPACE - User Manual.pdf` |
| RONE Analyzer | `RONE Analyzer - User Manual.pdf` |

Changing a PDF triggers that plugin's CI build (see the path filters in
`.github/workflows/main.yml`), so a manual update ships on its own.

## Regenerating

The screenshots are rendered from the plugin sources themselves - the same
HTML the WebView shows, at the editor's exact size - so a UI change only needs
a rerun, never a manual screenshot session:

```
python docs/manuals/tools/pipeline.py
```

Requirements: Python 3 with Pillow (`pip install pillow`), Microsoft Edge (used
headless for screenshots and PDF printing), internet access for the Google
Fonts the interfaces use. The Analyzer's native window is captured from the
installed application (`C:\Program Files\RONE Plugins\RONE Analyzer.exe`); pass
`--no-native` to skip that step and keep the previous capture.

Intermediate files live in `docs/manuals/work/` (git-ignored):

- `work/ui/` – the extracted / copied interfaces plus `harness.html`, `_shot.js`
  (state + DOM rects) and `_mock.js` (a fake JUCE bridge so the sample-based
  plugins show real defaults and a loaded file);
- `work/shots/` – raw 2x screenshots and the DOM rects JSON of every control;
- `work/docimg/` – annotated tour images and detail crops (`tools/annotate.py`);
- `work/html/` – the generated HTML, handy for proofreading in a browser.

Only the PDFs need rebuilding after editing text:

```
python docs/manuals/tools/pipeline.py --pdf-only
```

## Editing

- Text and structure: `tools/content_<plugin>.py` (one Python dict per manual,
  sections as HTML snippets using the helpers in `build_manuals.py`).
- Shared sections (installation through the Center, interface conventions,
  troubleshooting / specs / support): `tools/build_manuals.py`.
- Callouts and crops: `tools/annotate.py` (selectors are DOM ids from the UI;
  the Analyzer and Center use hand-placed boxes).
- Screenshot states (which buttons are clicked, which mock scenario runs):
  `tools/pipeline.py` (`STATES`) and `tools/_mock.js`.
- Look and feel of the PDF: the CSS at the top of `tools/build_manuals.py`.
  Each manual uses the plugin's own accent colour.

Adding a plugin: extract its UI in `pipeline.py` (`PLUGINS`), add its states,
an `annotate.py` block, a `content_<id>.py`, a `manual` field in
`versions.json`, a `Source:` line in its `.iss`, and its PDF path to the CI
path filter.
