#!/usr/bin/env python3
"""What every RONE product must install, checked against the real installers.

Born from RONE Flanger: no AU from its first commit until 2.1.9. CMake, the manifest, the
site and the manual all agreed with each other, so nothing looked wrong, and Logic users
simply never saw the plugin. This compares the shipped payload with a fixed contract.

  In the Mac leg (blocking), after pkgutil --expand-full:
    python3 scripts/ci/format_contract.py payload --id RoneFlanger --root <dir with ./Library>

  Against what is live (.github/workflows/format-contract.yml, every night):
    python3 scripts/ci/format_contract.py live

  One installer by hand, a .pkg file parsed without macOS:
    python3 scripts/ci/format_contract.py pkg --id RoneFlanger --file <x.pkg>

Every plugin ships VST3 + AU + Standalone. A product that does not goes in EXCEPTIONS, with
its reason, so that a missing format is a decision and never an accident.
"""
import argparse, json, os, re, struct, sys, tempfile, urllib.error, urllib.request, zlib
import xml.etree.ElementTree as ET

MANIFEST_URL = 'https://raw.githubusercontent.com/liranidan2000-max/rone-plugins/main/versions.json'
FX_URL = 'https://roneaudio.com/assets/fx.js'
UA = {'User-Agent': 'Mozilla/5.0 (RONE format contract)'}

VST3 = 'Library/Audio/Plug-Ins/VST3/'
AU = 'Library/Audio/Plug-Ins/Components/'
APP = 'Applications/'
INSTALL_DIRS = (VST3, AU, APP, APP + 'RONE Plugins/')

# id -> the bundles it installs, when that is not the standard three
EXCEPTIONS = {
    # An app, with two small plugins that feed it from the DAW
    'RONEAnalyzer': [APP + 'RONE Analyzer.app',
                     VST3 + 'RONE Analyzer Bridge.vst3', AU + 'RONE Analyzer Bridge.component',
                     VST3 + 'RONE Analyzer Channel.vst3', AU + 'RONE Analyzer Channel.component'],
}


def required(entry):
    if entry['id'] in EXCEPTIONS:
        return EXCEPTIONS[entry['id']]
    name = entry['name']
    return [VST3 + name + '.vst3', AU + name + '.component', APP + name + '.app']


def norm(path):
    return path[2:] if path.startswith('./') else path.lstrip('/')


def find_entry(manifest, product_id):
    return next((e for e in manifest.get('plugins', []) if e.get('id') == product_id), None)


def check_payload(entry, bundles):
    """bundles: paths relative to the install root."""
    have = {norm(b) for b in bundles}
    # The app sits in /Applications or in /Applications/RONE Plugins
    apps = {os.path.basename(b) for b in have if b.endswith('.app')}
    bad = []
    for want in required(entry):
        found = os.path.basename(want) in apps if want.endswith('.app') else want in have
        if not found:
            bad.append('does not install ' + want)
    return bad


def check_manifest(entry):
    bad = []
    formats = entry.get('formats') or []
    names = [os.path.basename(b) for b in required(entry)]
    for ext, fmt in (('.vst3', 'VST3'), ('.component', 'AU'), ('.app', 'Standalone')):
        if any(n.endswith(ext) for n in names) and fmt not in formats:
            bad.append('manifest formats has no ' + fmt)
    for key in ('vst3_bundle', 'au_bundle'):
        if entry.get(key) and entry[key] not in names:
            bad.append(f'manifest {key} = {entry[key]}, which is not in the payload')
    return bad


def pkg_info(path):
    """(bundles, version) of a flat .pkg (a xar archive), read with the standard library."""
    with open(path, 'rb') as f:
        data = f.read()
    magic, header, _, toc_size, _, _ = struct.unpack('>4sHHQQI', data[:28])
    if magic != b'xar!':
        raise ValueError(path + ' is not a .pkg')
    heap = header + toc_size
    bundles, version = [], ''
    for node in ET.fromstring(zlib.decompress(data[header:heap])).iter('file'):
        item = node.find('data')
        if node.findtext('name') != 'PackageInfo' or item is None:
            continue
        start = heap + int(item.findtext('offset'))
        info = ET.fromstring(zlib.decompress(data[start:start + int(item.findtext('length'))]))
        version = version or info.get('version', '')
        bundles += [b.get('path') for b in info.iter('bundle') if b.get('path')]
    return bundles, version


def get(url):
    with urllib.request.urlopen(urllib.request.Request(url, headers=UA), timeout=120) as r:
        return r.read()


def base(version):
    return '.'.join(str(version or '').split('.')[:3])


def chips_on_page(html):
    row = re.search(r'class="meta-row">(.*?)</div>', html, re.S)
    return set(re.findall(r'class="chip">([A-Z0-9]+)</span>', row.group(1))) if row else None


def check_site(entry, fx):
    """Returns (problems, notes). A page that will not load is a note: the site's own health
    has its own checks, and Cloudflare may refuse a data-centre address."""
    bad, notes = [], []
    want = {f.upper() for f in entry.get('formats') or []}
    url = entry.get('store_url', '')
    try:
        chips = chips_on_page(get(url).decode('utf-8', 'replace'))
    except (urllib.error.URLError, OSError) as e:
        return [], [f'site page not read ({url}): {e}']
    if chips is None:
        notes.append('no format chips found on ' + url)
    elif chips != want:
        bad.append(f'product page shows {sorted(chips)}, manifest says {sorted(want)}')
    slug = re.search(r'/products/([a-z0-9-]+)', url)
    if fx and slug:
        card = re.search(r"slug:'" + slug.group(1) + r"'.*?fmt:\[(.*?)\]", fx, re.S)
        if card and set(re.findall(r"'([A-Z0-9]+)'", card.group(1))) != want:
            bad.append(f'assets/fx.js shows {card.group(1)}, manifest says {sorted(want)}')
    return bad, notes


def live():
    manifest = json.loads(get(MANIFEST_URL))
    try:
        fx = get(FX_URL).decode('utf-8', 'replace')
    except (urllib.error.URLError, OSError) as e:
        print('note: fx.js not read:', e)
        fx = ''
    failed = 0
    with tempfile.TemporaryDirectory() as tmp:
        for entry in manifest['plugins']:
            bad = check_manifest(entry)
            mac_version = entry.get('version_mac') or entry.get('version')
            if base(entry.get('version')) != base(mac_version):
                bad.append(f"Mac is on {mac_version}, Windows on {entry.get('version')}")
            path = os.path.join(tmp, entry['id'] + '.pkg')
            with open(path, 'wb') as f:
                f.write(get(entry['download_url_mac']))
            bundles, version = pkg_info(path)
            os.remove(path)
            bad += check_payload(entry, bundles)
            if base(version) != base(mac_version):
                bad.append(f'live pkg is {version}, manifest says {mac_version}')
            site_bad, notes = check_site(entry, fx)
            bad += site_bad
            for note in notes:
                print(f"note: {entry['id']}: {note}")
            print(('FAIL ' if bad else 'ok   ') + entry['id'], mac_version)
            for item in bad:
                print('     - ' + item)
            failed += bool(bad)
    print(f'{failed} product(s) failed' if failed else 'every product holds its contract')
    return 1 if failed else 0


def report(product_id, bad):
    for item in bad:
        print(f'::error::{product_id}: {item}')
    if not bad:
        print(product_id + ': the installer holds its contract')
    return 1 if bad else 0


def payload(args):
    with open(args.manifest, encoding='utf-8') as f:
        entry = find_entry(json.load(f), args.id)
    bundles = []
    for folder in INSTALL_DIRS:
        top = os.path.join(args.root, folder)
        if os.path.isdir(top):
            bundles += [folder + n for n in os.listdir(top)]
    if entry is None:
        # New to the Center: the same rule, with the name its VST3 carries
        vst3 = [b for b in bundles if b.startswith(VST3) and b.endswith('.vst3')]
        if not vst3:
            return report(args.id, ['does not install a VST3'])
        entry = {'id': args.id, 'name': os.path.basename(vst3[0])[:-len('.vst3')]}
    return report(args.id, check_payload(entry, bundles))


def main():
    parser = argparse.ArgumentParser()
    sub = parser.add_subparsers(dest='cmd', required=True)
    one = sub.add_parser('payload')
    one.add_argument('--id', required=True)
    one.add_argument('--root', required=True)
    one.add_argument('--manifest', default='versions.json')
    one = sub.add_parser('pkg')
    one.add_argument('--id', required=True)
    one.add_argument('--file', required=True)
    one.add_argument('--manifest', default='versions.json')
    sub.add_parser('live')
    args = parser.parse_args()
    if args.cmd == 'live':
        return live()
    if args.cmd == 'payload':
        return payload(args)
    with open(args.manifest, encoding='utf-8') as f:
        entry = find_entry(json.load(f), args.id)
    return report(args.id, check_payload(entry, pkg_info(args.file)[0]))


if __name__ == '__main__':
    sys.exit(main())
