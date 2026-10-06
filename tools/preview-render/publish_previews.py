"""After Liran has listened to out/<id>-dry.mp3 / -wet.mp3: puts the approved
pairs where the Center finds them. Deploys nothing by itself.

    python publish_previews.py RoneThrow RoneRise ...

1. copies the pair to WEBSITE RONE AUDIO/media/previews/ (served with CORS by
   that folder's _headers rule once the site is deployed);
2. writes "preview": {"dry", "wet"} into versions.json for each plugin.
Then: deploy the site (the deploy protocol), and push versions.json.
"""
import json, os, shutil, sys

HERE = os.path.dirname(os.path.abspath(__file__))
SITE = r'D:\RONE PLUGINS\WEBSITE RONE AUDIO\media\previews'
MANIFEST = os.path.join(HERE, '..', '..', 'versions.json')
BASE = 'https://roneaudio.com/media/previews/'

ids = sys.argv[1:]
if not ids:
    raise SystemExit(__doc__)

os.makedirs(SITE, exist_ok=True)
with open(MANIFEST, encoding='utf-8') as f:
    manifest = json.load(f)

for pid in ids:
    for side in ('dry', 'wet'):
        src = os.path.join(HERE, 'out', f'{pid}-{side}.mp3')
        if not os.path.exists(src):
            raise SystemExit(f'missing {src} - render it first')
        shutil.copyfile(src, os.path.join(SITE, f'{pid}-{side}.mp3'))
    entry = next((p for p in manifest['plugins'] if p['id'] == pid), None)
    if entry is None:
        raise SystemExit(f'{pid} is not in versions.json')
    entry['preview'] = {'dry': BASE + f'{pid}-dry.mp3', 'wet': BASE + f'{pid}-wet.mp3'}
    print('published', pid)

with open(MANIFEST, 'w', encoding='utf-8', newline='\n') as f:
    f.write(json.dumps(manifest, indent=2, ensure_ascii=False) + '\n')
