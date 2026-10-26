"""The hand model for a plugin with a RONE Control rack - the five files the rack's camera loads (Google's MediaPipe
hand landmarker and its runtime, Apache 2.0, unchanged), fetched by pinned SHA-256 into one folder the installer ships:

    python scripts/ci/fetch_hands.py <out folder>          # e.g. <workspace>/hands/mp

The installers copy that folder into Program Files/RONE Plugins/Hands (installer/<Plugin>.iss, "hands" block); the
rack copies the files from there into RONE Control's link folder the first time its camera starts
(Shared/RoneControlRack/LinkView.cpp, prepareLinkFolder). Rack-first, Liran 26.10.2026: every rack installer carries
the model, so a plugin's camera works without RONE Control installed. The same files, same versions, as RONE Control's
own tools/fetch_mediapipe.py - keep the two lists equal. Mac installers ship none: a macOS plug-in cannot own the camera.
"""
import hashlib
import pathlib
import sys
import urllib.request

NPM = 'https://cdn.jsdelivr.net/npm/@mediapipe/tasks-vision@1.0.1/'
MODEL = 'https://storage.googleapis.com/mediapipe-models/hand_landmarker/hand_landmarker/float16/1/hand_landmarker.task'

FILES = [
    ('vision_bundle.mjs', NPM + 'vision_bundle.mjs', 'd885630c297c0b20b1fe86096cb06291c4c8080876f27852e724f24ac603713f'),
    ('wasm/vision_wasm_internal.js', NPM + 'wasm/vision_wasm_internal.js', 'e170ee67dd4e16c1a6fcd8840a206687e5a59b22c20e4a902bc445b095454d73'),
    ('wasm/vision_wasm_internal.wasm', NPM + 'wasm/vision_wasm_internal.wasm', '8da277a733926eacd0474b8704b36742d6ec3231c57a860c5b889dff8f1df886'),
    ('hand_landmarker.task', MODEL, 'fbc2a30080c3c557093b5ddfc334698132eb341044ccee322ccf8bcf3607cde1'),
]

NOTICE = """These files are served unchanged from Google's MediaPipe:

  vision_bundle.mjs, wasm/*   @mediapipe/tasks-vision 1.0.1 (npm), Copyright Google LLC, Apache License 2.0
  hand_landmarker.task        MediaPipe Hand Landmarker model (float16, v1), Copyright Google LLC, Apache License 2.0
                              https://storage.googleapis.com/mediapipe-models/hand_landmarker/hand_landmarker/float16/1/hand_landmarker.task

Apache License 2.0: https://www.apache.org/licenses/LICENSE-2.0
MediaPipe: https://github.com/google-ai-edge/mediapipe
"""


def sha256(path: pathlib.Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> int:
    if len(sys.argv) != 2:
        print(__doc__)
        return 2
    out = pathlib.Path(sys.argv[1])
    bad = 0
    for rel, url, want in FILES:
        dest = out / rel
        if dest.exists() and sha256(dest) == want:
            print(f'ok      {rel}')
            continue
        dest.parent.mkdir(parents=True, exist_ok=True)
        req = urllib.request.Request(url, headers={'User-Agent': 'rone-hands-fetch/1.0'})
        with urllib.request.urlopen(req, timeout=120) as r:
            data = r.read()
        got = hashlib.sha256(data).hexdigest()
        if got != want:
            print(f'BAD     {rel}: sha256 {got}, expected {want}')
            bad += 1
            continue
        dest.write_bytes(data)
        print(f'fetched {rel} ({len(data):,} bytes)')
    (out / 'NOTICE.txt').write_text(NOTICE, encoding='utf-8', newline='\n')
    print(f'notice  NOTICE.txt')
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main())
