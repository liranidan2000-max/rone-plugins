"""Shared plumbing for the CI tests: runs a `run:` block of .github/workflows/main.yml
locally with bash, with ${{ ... }} expressions filled in, a fake `gh`, and - when no
real jq is installed - a stand-in that knows exactly the jq filters the workflow and
scripts/ci/*.sh use (an unknown filter fails the test loudly instead of guessing).

Not a test itself; see ci_release_test.py and manifest-plan.test.sh.
"""
import json, os, re, shutil, subprocess, sys
import yaml

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, '..', '..', '..'))
YML = os.path.join(ROOT, '.github', 'workflows', 'main.yml')


def bash_exe():
    for c in (shutil.which('bash'), r'C:\Program Files\Git\usr\bin\bash.exe', r'C:\Program Files\Git\bin\bash.exe'):
        if c and os.path.exists(c):
            return c
    sys.exit('bash not found')


def posix(p):
    """C:\\x\\y -> /c/x/y for Git Bash; unchanged elsewhere."""
    p = os.path.abspath(p)
    if os.name == 'nt' and re.match(r'^[A-Za-z]:', p):
        return '/' + p[0].lower() + p[2:].replace('\\', '/')
    return p


def workflow():
    return yaml.safe_load(open(YML, encoding='utf-8'))


def step_script(job, name):
    steps = workflow()['jobs'][job]['steps']
    return next(s['run'] for s in steps if s.get('name') == name)


def fill(script, values):
    """Replace every ${{ expr }} with values[expr]; an expression left over fails."""
    def sub(m):
        expr = m.group(1).strip()
        if expr not in values:
            raise KeyError('no value for ${{ %s }}' % expr)
        return values[expr]
    return re.sub(r'\$\{\{(.*?)\}\}', sub, script)


def write(path, text, mode=None):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, 'w', encoding='utf-8', newline='\n') as f:
        f.write(text)
    if mode:
        os.chmod(path, mode)


def copy_lf(src, dst):
    """Copy a script with LF endings (a Windows checkout may have CRLF)."""
    write(dst, open(src, encoding='utf-8').read().replace('\r\n', '\n'))


# ---------------------------------------------------------------------------
# jq stand-in. Each entry: normalised filter text -> function(data, args) -> list of
# outputs. Only what scripts/ci/*.sh and the workflow steps under test call.
# ---------------------------------------------------------------------------
JQ_PY = r'''
import json, sys, re
sys.stdout.reconfigure(newline='\n')
a = sys.argv[1:]; arg = {}; raw = False; compact = False; null_in = False; exit_status = False
files = []
flt = None
while a:
    x = a.pop(0)
    if x == '--arg': k = a.pop(0); arg[k] = a.pop(0)
    elif x == '--argjson': k = a.pop(0); arg[k] = json.loads(a.pop(0))
    elif x in ('-r', '--raw-output'): raw = True
    elif x in ('-c', '--compact-output'): compact = True
    elif x in ('-n', '--null-input'): null_in = True
    elif x in ('-e', '--exit-status'): exit_status = True
    elif x.startswith('-') and len(x) > 1 and flt is None and re.fullmatch(r'-[rcne]+', x):
        raw |= 'r' in x; compact |= 'c' in x; null_in |= 'n' in x; exit_status |= 'e' in x
    elif flt is None: flt = x
    else: files.append(x)
norm = ' '.join(flt.split())
if null_in: data = None
elif files: data = json.load(open(files[0], encoding='utf-8'))
else:
    txt = sys.stdin.read()
    try: data = json.loads(txt)
    except Exception: sys.exit(5)

def plugin(d, i):
    return [p for p in d.get('plugins', []) if p.get('id') == i]

def setp(d, i, **kv):
    for p in plugin(d, i):
        for k, v in kv.items(): p[k] = v
    return d

def ci(d, **kv):
    d.setdefault('center_installer', {}).update(kv); return d

F = {
 '.': lambda d: [d],
 '.[]': lambda d: list(d),
 'any(.[]; .plugin_id == $id)': lambda d: [any(x.get('plugin_id') == arg['id'] for x in d)],
 'any(.[]; . == $id)': lambda d: [any(x == arg['id'] for x in d)],
 '. + [$id]': lambda d: [d + [arg['id']]],
 '. + [$e]': lambda d: [d + [arg['e']]],
 '.center_installer.version // ""': lambda d: [(d.get('center_installer') or {}).get('version') or ''],
 '[.plugins[]? | select(.id == $id) | .version][0] // ""': lambda d: [([p.get('version') for p in plugin(d, arg['id'])] + [None])[0] or ''],
 '[.plugins[]? | select(.id == $id) | (.version_mac // .version)][0] // ""':
     lambda d: [([p.get('version_mac') or p.get('version') for p in plugin(d, arg['id'])] + [None])[0] or ''],
 '(.plugins[]|select(.id==$id)).version_mac = $v': lambda d: [setp(d, arg['id'], version_mac=arg['v'])],
 '(.plugins[]|select(.id==$id)) |= (if .version_mac == null and (.sha256_mac // "") != "" then .version_mac = .version else . end)':
     lambda d: [[p.__setitem__('version_mac', p.get('version')) for p in plugin(d, arg['id'])
                 if p.get('version_mac') is None and (p.get('sha256_mac') or '') != ''] and d or d],
 '.center_installer.version = $v': lambda d: [ci(d, version=arg['v'])],
 '.center_installer.url = $u | .center_installer.sha256 = $sw': lambda d: [ci(d, url=arg['u'], sha256=arg['sw'])],
 '.center_installer.url_mac = $um | .center_installer.sha256_mac = $sm': lambda d: [ci(d, url_mac=arg['um'], sha256_mac=arg['sm'])],
 '(.plugins[]|select(.id==$id)).version = $v': lambda d: [setp(d, arg['id'], version=arg['v'])],
 '(.plugins[]|select(.id==$id)).download_url = $u | (.plugins[]|select(.id==$id)).sha256 = $sw': lambda d: [setp(d, arg['id'], download_url=arg['u'], sha256=arg['sw'])],
 '(.plugins[]|select(.id==$id)).size = $sz': lambda d: [setp(d, arg['id'], size=arg['sz'])],
 '(.plugins[]|select(.id==$id)).download_url_mac = $um | (.plugins[]|select(.id==$id)).sha256_mac = $sm': lambda d: [setp(d, arg['id'], download_url_mac=arg['um'], sha256_mac=arg['sm'])],
 '(.plugins[]|select(.id==$id)).size_mac = $sz': lambda d: [setp(d, arg['id'], size_mac=arg['sz'])],
 '.last_updated=$t': lambda d: [dict(d, last_updated=arg['t'])],
 '.center_installer.url, .center_installer.url_mac, (.plugins[]? | .download_url, .download_url_mac) | select(. != null)':
     lambda d: [u for u in [(d.get('center_installer') or {}).get('url'), (d.get('center_installer') or {}).get('url_mac')]
                + [x for p in d.get('plugins', []) for x in (p.get('download_url'), p.get('download_url_mac'))] if u is not None],
}
if norm not in F:
    sys.exit('jq stand-in: unknown filter: ' + flt)
out = F[norm](data)
for o in out:
    if raw and isinstance(o, str): print(o)
    elif compact: print(json.dumps(o, separators=(',', ':'), ensure_ascii=False))
    else: print(json.dumps(o, indent=2, ensure_ascii=False))
if exit_status and (not out or out[-1] in (None, False)): sys.exit(1)
'''


def install_jq(bindir):
    """Real jq when there is one, the stand-in otherwise. Returns a label."""
    if shutil.which('jq'):
        return 'jq'
    write(os.path.join(bindir, 'jq.py'), JQ_PY)
    py = posix(sys.executable)
    write(os.path.join(bindir, 'jq'), '#!/bin/bash\nexec "%s" "$(dirname "$0")/jq.py" "$@"\n' % py, 0o755)
    return 'jq stand-in'


# ---------------------------------------------------------------------------
# Fake gh. State in $GH_STATE/<tag>.json = {"assets": [{"id", "name"}], "body": ""}.
# $GH_FAIL: space-separated "<name>:<n>" - the next n uploads of that file name fail.
# $GH_CREATE_FAIL=1: release create fails. Every call is logged to $GH_LOG.
# ---------------------------------------------------------------------------
FAKE_GH = r'''#!/bin/bash
echo "gh $*" >> "$GH_LOG"
exec "$PYEXE" "$(dirname "$0")/gh.py" "$@"
'''

GH_PY = r'''
import json, os, sys, re
st = os.environ['GH_STATE']; log = open(os.environ['GH_LOG'], 'a', encoding='utf-8')
a = sys.argv[1:]
def path(tag): return os.path.join(st, tag + '.json')
def load(tag):
    return json.load(open(path(tag))) if os.path.exists(path(tag)) else None
def save(tag, r): json.dump(r, open(path(tag), 'w'))
def nextid():
    p = os.path.join(st, '_id'); n = int(open(p).read()) + 1 if os.path.exists(p) else 1000
    open(p, 'w').write(str(n)); return n
def fails(name):
    spec = os.environ.get('GH_FAIL', '').split(); p = os.path.join(st, '_fails.json')
    used = json.load(open(p)) if os.path.exists(p) else {}
    for s in spec:
        n, k = s.rsplit(':', 1)
        if n == name and used.get(n, 0) < int(k):
            used[n] = used.get(n, 0) + 1; json.dump(used, open(p, 'w')); return True
    return False
if a[:2] == ['release', 'view']:
    r = load(a[2])
    if r is None: sys.exit(1)
    if '--json' in a: print(r.get('body', ''))
    sys.exit(0)
if a[:2] == ['release', 'create']:
    if os.environ.get('GH_CREATE_FAIL') == '1': print('HTTP 500', file=sys.stderr); sys.exit(1)
    tag = a[2]
    # every option takes a value except --latest=false
    opts = {'--title', '--notes', '-R'}; rest = a[3:]; i = 0; files = []
    while i < len(rest):
        if rest[i] in opts: i += 2; continue
        if rest[i].startswith('--'): i += 1; continue
        files.append(rest[i]); i += 1
    notes = rest[rest.index('--notes') + 1] if '--notes' in rest else ''
    save(tag, {'assets': [{'id': nextid(), 'name': os.path.basename(f)} for f in files], 'body': notes})
    log.write('CREATED %s files=%d\n' % (tag, len(files))); sys.exit(0)
if a[:2] == ['release', 'upload']:
    tag, f = a[2], a[3]; name = os.path.basename(f); r = load(tag)
    if r is None: print('release not found', file=sys.stderr); sys.exit(1)
    if fails(name): print('HTTP 500', file=sys.stderr); sys.exit(1)
    if '--clobber' not in a and any(x['name'] == name for x in r['assets']):
        print('already_exists', file=sys.stderr); sys.exit(1)
    r['assets'] = [x for x in r['assets'] if x['name'] != name] + [{'id': nextid(), 'name': name}]
    save(tag, r); log.write('UPLOADED %s %s\n' % (tag, name)); sys.exit(0)
if a[:2] == ['release', 'edit']:
    r = load(a[2]); r['body'] = a[a.index('--notes') + 1]; save(a[2], r)
    log.write('EDITED %s\n' % a[2]); sys.exit(0)
if a[0] == 'api':
    if a[1] == '-X':
        method, url = a[2], a[3]; aid = int(url.rsplit('/', 1)[1])
        for fn in os.listdir(st):
            if not fn.endswith('.json') or fn.startswith('_'): continue
            tag = fn[:-5]; r = load(tag)
            for x in r['assets']:
                if x['id'] == aid:
                    if method == 'DELETE': r['assets'].remove(x); log.write('DELETED %s %s\n' % (tag, x['name']))
                    if method == 'PATCH':
                        new = a[a.index('-f') + 1].split('=', 1)[1]; log.write('RENAMED %s %s -> %s\n' % (tag, x['name'], new)); x['name'] = new
                    save(tag, r); sys.exit(0)
        sys.exit(1)
    m = re.match(r'repos/[^/]+/[^/]+/releases/tags/(.+)$', a[1])
    if m:
        r = load(m.group(1))
        if r is None: sys.exit(1)
        q = a[a.index('--jq') + 1] if '--jq' in a else ''
        if 'startswith("stale-")' in q:
            for x in r['assets']:
                if x['name'].startswith('stale-') or ('-stuck-' in x['name'] and 'test("-stuck-")' in q): print(x['id'])
        elif '.digest' in q:
            for x in r['assets']: print(x['name'], x.get('digest') or 'null')
        else:
            mm = re.search(r'\.name == "([^"]+)"', q)
            for x in r['assets']:
                if mm and x['name'] == mm.group(1): print(x['id'])
        sys.exit(0)
print('fake gh: unhandled ' + ' '.join(a), file=sys.stderr); sys.exit(2)
'''


def install_gh(bindir):
    write(os.path.join(bindir, 'gh.py'), GH_PY)
    write(os.path.join(bindir, 'gh'), FAKE_GH, 0o755)
    write(os.path.join(bindir, 'sleep'), '#!/bin/bash\nexit 0\n', 0o755)   # no real waits


def git(cwd, *args, env=None):
    r = subprocess.run(['git', '-C', cwd] + list(args), capture_output=True, text=True, env=env)
    if r.returncode != 0:
        raise RuntimeError('git %s: %s' % (' '.join(args), r.stderr))
    return r.stdout.strip()


def run_bash(script_path, cwd, env, bindir, flags='-e'):
    full = dict(os.environ)
    full.update(env)
    full['PYEXE'] = posix(sys.executable)
    cmd = 'export PATH="%s:$PATH"; cd "%s"; bash %s "%s"' % (posix(bindir), posix(cwd), flags, posix(script_path))
    return subprocess.run([bash_exe(), '-c', cmd], capture_output=True, text=True,
                          encoding='utf-8', errors='replace', env=full)


def read_outputs(path):
    out = {}
    if os.path.exists(path):
        for line in open(path, encoding='utf-8'):
            if '=' in line:
                k, v = line.rstrip('\n').split('=', 1)
                out[k] = v
    return out
