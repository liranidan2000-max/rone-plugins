"""Tests for the versions.json update (audit R207, R028/R205):
  - main.yml's 'Bump versions.json' step writes a plan with the versioned URLs,
    and leaves a product alone when its file does not answer or GitHub's digest differs;
  - scripts/ci/manifest-plan.sh's plan_commit_push writes the plan onto the newest
    main: a concurrent bot commit, a hand edit and a newer live version all survive.

  python scripts/ci/tests/manifest_plan_test.py

Scratch git repos only; gh and curl are fakes; jq is real when installed.
"""
import json, os, shutil, subprocess, sys, tempfile
sys.stdout.reconfigure(encoding='utf-8', errors='replace')
from ci_harness import (ROOT, step_script, fill, write, copy_lf, install_jq, install_gh,
                        git, run_bash, read_outputs, posix)

failures = []


def check(cond, what):
    print(('  ok   ' if cond else '  FAIL ') + what)
    if not cond:
        failures.append(what)


REL = 'https://github.com/liranidan2000-max/rone-plugins/releases/download/'
LIVE = {
    'license_mode': 'paid',
    'license_message': '',
    'last_updated': '2026-10-13T00:00:00Z',
    'center_installer': {'version': '2.1.2.264', 'url': REL + 'PluginsCenter-latest/RONE_Plugins_Center_Installer.exe',
                         'url_mac': REL + 'PluginsCenter-latest/RONE_Plugins_Center.dmg', 'sha256': 'c0', 'sha256_mac': 'c1'},
    'plugins': [
        {'id': 'RoneStutter', 'version': '1.5.5.268', 'download_url': REL + 'RoneStutter-latest/RoneStutter_Installer.exe',
         'sha256': 'a0', 'size': 1, 'download_url_mac': REL + 'RoneStutter-latest/RoneStutter_Installer.pkg', 'sha256_mac': 'a1', 'size_mac': 2},
        {'id': 'RoneThrow', 'version': '1.8.3.270', 'download_url': REL + 'RoneThrow-latest/RoneThrow_Installer.exe',
         'sha256': 'b0', 'size': 3, 'download_url_mac': REL + 'RoneThrow-latest/RoneThrow_Installer.pkg', 'sha256_mac': 'b1', 'size_mac': 4},
    ],
}
SCRIPTS = ['release-guard.sh', 'manifest-plan.sh']


def repo(base, live=LIVE):
    origin = os.path.join(base, 'origin.git')
    ci = os.path.join(base, 'ci')
    os.makedirs(ci)
    git(base, 'init', '-q', '--bare', '-b', 'main', origin)
    git(ci, 'init', '-q', '-b', 'main')
    for c in (('user.email', 't@example.invalid'), ('user.name', 'seed'), ('core.autocrlf', 'false')):
        git(ci, 'config', *c)
    write(os.path.join(ci, 'versions.json'), json.dumps(live, indent=2, ensure_ascii=False) + '\n')
    write(os.path.join(ci, 'RoneStutter', 'CMakeLists.txt'), 'project(x VERSION 1.5.6)\n')
    write(os.path.join(ci, 'RonePluginsCenter', 'CMakeLists.txt'), 'project(x VERSION 2.1.3)\n')
    write(os.path.join(ci, 'RoneThrow', 'CMakeLists.txt'), 'project(x VERSION 1.8.4)\n')
    for sname in SCRIPTS:
        copy_lf(os.path.join(ROOT, 'scripts', 'ci', sname), os.path.join(ci, 'scripts', 'ci', sname))
    git(ci, 'add', '-A')
    git(ci, 'commit', '-q', '-m', 'seed')
    git(ci, 'remote', 'add', 'origin', origin)
    git(ci, 'push', '-q', '-u', 'origin', 'main')
    other = os.path.join(base, 'other')
    subprocess.run(['git', 'clone', '-q', '-b', 'main', origin, other], check=True)
    for c in (('user.email', 'liran@example.invalid'), ('user.name', 'other'), ('core.autocrlf', 'false')):
        git(other, 'config', *c)
    return origin, ci, other


def origin_manifest(origin):
    out = subprocess.run(['git', '--git-dir', origin, 'show', 'main:versions.json'], capture_output=True, text=True, check=True).stdout
    return json.loads(out)


def plug(m, pid):
    return next(p for p in m['plugins'] if p['id'] == pid)


def gh_state(base, notes):
    st = os.path.join(base, 'ghstate')
    os.makedirs(st, exist_ok=True)
    for tag, (body, assets) in notes.items():
        json.dump({'body': body, 'assets': assets}, open(os.path.join(st, tag + '.json'), 'w'))
    return st


# ---------------------------------------------------------------------------
# A: plan_commit_push
# ---------------------------------------------------------------------------
PUSH_DRIVER = r'''
set -e
source scripts/ci/release-guard.sh
source scripts/ci/manifest-plan.sh
git config user.name "github-actions[bot]"; git config user.email "bot@example.invalid"
# RACE: a bash function run in the other clone before our push, on the attempts listed in RACE_ON
git() {
  if [ "$1" = push ] && [ -n "${RACE_ON:-}" ]; then
    n=$(cat "$T/pushes" 2>/dev/null || echo 0); n=$((n + 1)); echo "$n" > "$T/pushes"
    case " $RACE_ON " in *" $n "*) ( cd "$OTHER" && bash "$T/race.sh" "$n" ) ;; esac
  fi
  command git "$@"
}
PLAN_SKIPPED=''
rc=0; plan_commit_push "$PLAN" 999 owner/repo "ci: auto-update versions.json (build #999) [skip ci]" || rc=$?
echo "RC=$rc"
echo "SKIPPED=${PLAN_SKIPPED# }"
'''


def run_push(case, plan_lines, race_sh=None, race_on='', live=LIVE, notes=None):
    print('\n== ' + case)
    base = tempfile.mkdtemp(prefix='mplan-')
    origin, ci, other = repo(base, live)
    bindir = os.path.join(base, 'bin')
    jq = install_jq(bindir)
    install_gh(bindir)
    st = gh_state(base, notes or {'RoneStutter-latest': ('Build #999 · x', []), 'RoneThrow-latest': ('Build #999 · x', []),
                                   'PluginsCenter-latest': ('Build #999 · x', [])})
    plan = os.path.join(base, 'plan.txt')
    write(plan, ''.join(l + '\n' for l in plan_lines))
    if race_sh:
        write(os.path.join(base, 'race.sh'), race_sh)
    drv = os.path.join(base, 'drv.sh')
    write(drv, PUSH_DRIVER)
    env = {'PLAN': posix(plan), 'T': posix(base), 'OTHER': posix(other), 'RACE_ON': race_on,
           'GH_STATE': posix(st), 'GH_LOG': posix(os.path.join(base, 'gh.log'))}
    r = run_bash(drv, ci, env, bindir, flags='')
    out = r.stdout + r.stderr
    rc = int(out.split('RC=')[1].split()[0]) if 'RC=' in out else -1
    skipped = out.split('SKIPPED=')[1].splitlines()[0] if 'SKIPPED=' in out else '?'
    print('   (%s) rc=%d skipped=%r' % (jq, rc, skipped))
    for l in out.splitlines():
        if '::' in l or 'rejected' in l or 'already carries' in l:
            print('    | ' + l)
    m = origin_manifest(origin)
    commits = subprocess.run(['git', '--git-dir', origin, 'log', '--format=%an|%s', 'main'], capture_output=True, text=True).stdout.splitlines()
    shutil.rmtree(base, ignore_errors=True)
    return rc, skipped, m, commits


STUTTER = 'stutter|RoneStutter|1.5.6.999|RoneStutter-latest|https://g/RoneStutter-latest/RoneStutter_Installer-999.1.exe|w999|111|https://g/RoneStutter-latest/RoneStutter_Installer-999.1.pkg|m999|222'
CENTER = 'plugins_center|__center__|2.1.3.999|PluginsCenter-latest|https://g/PluginsCenter-latest/RONE_Plugins_Center_Installer-999.1.exe|cw|0|https://g/PluginsCenter-latest/RONE_Plugins_Center-999.1.dmg|cm|0'

# A1: nothing in the way - one bot commit, the entry written, last_updated moved, hand fields untouched.
rc, sk, m, commits = run_push('plain push', [STUTTER])
p = plug(m, 'RoneStutter')
check(rc == 0 and sk == '', 'pushed, nothing skipped')
check(p['version'] == '1.5.6.999' and p['download_url'].endswith('RoneStutter_Installer-999.1.exe') and p['sha256'] == 'w999'
      and p['size'] == 111 and p['download_url_mac'].endswith('-999.1.pkg') and p['sha256_mac'] == 'm999' and p['size_mac'] == 222,
      'stutter entry = version + versioned urls + sha + size')
check(m['last_updated'] != LIVE['last_updated'], 'last_updated moved')
check(plug(m, 'RoneThrow') == LIVE['plugins'][1] and m['center_installer'] == LIVE['center_installer'], 'other entries untouched')
check(len(commits) == 2 and commits[0] == 'github-actions[bot]|ci: auto-update versions.json (build #999) [skip ci]', 'one bot commit')

# A2: a concurrent bot commit for ANOTHER product lands between our fetch and push (it also moves last_updated).
RACE_THROW = r'''
jq '(.plugins[]|select(.id==$id)).version = $v' --arg id RoneThrow --arg v 1.8.4.998 versions.json > v.tmp && mv v.tmp versions.json
jq '.last_updated=$t' --arg t 2026-10-13T09:09:09Z versions.json > v.tmp && mv v.tmp versions.json
git add versions.json && git commit -q -m "ci: auto-update versions.json (build #998) [skip ci]" && git push -q origin HEAD:main
'''
rc, sk, m, commits = run_push('concurrent bot commit (R207 a)', [STUTTER], race_sh=RACE_THROW, race_on='1')
check(rc == 0, 'pushed on the second attempt')
check(plug(m, 'RoneThrow')['version'] == '1.8.4.998', "the other run's throw bump survives")
check(plug(m, 'RoneStutter')['version'] == '1.5.6.999', 'our stutter bump is there too')
check(len(commits) == 3, 'seed + their commit + ours, nothing lost or duplicated')

# A3: a hand edit of license_message races us - it must survive (no -X theirs).
# the jq stand-in knows no license_message filter, so the hand edit is made with sed
RACE_HAND = r'''
sed -i 's/"license_message": ""/"license_message": "Maintenance tonight"/' versions.json
git add versions.json && git commit -q -m "license: message" && git push -q origin HEAD:main
'''
rc, sk, m, commits = run_push('hand edit races (R207 b)', [STUTTER], race_sh=RACE_HAND, race_on='1')
check(rc == 0 and m['license_message'] == 'Maintenance tonight', 'the hand-edited license_message survives')
check(plug(m, 'RoneStutter')['version'] == '1.5.6.999', 'and the bump is written')

# A4: main already carries a NEWER stutter (another run) - our entry is left alone and flagged.
live_newer = json.loads(json.dumps(LIVE))
plug(live_newer, 'RoneStutter')['version'] = '1.5.7.1000'
rc, sk, m, commits = run_push('newer live version (R207 c)', [STUTTER], live=live_newer)
check(rc == 0 and sk == 'stutter', 'left alone, reported in PLAN_SKIPPED')
check(plug(m, 'RoneStutter')['version'] == '1.5.7.1000' and len(commits) == 1, 'main untouched, no commit')

# A5: the newer version arrives during our attempt (the race), then we must not overwrite it.
RACE_NEWER = r'''
sed -i 's/"version": "1.5.5.268"/"version": "1.5.7.1000"/' versions.json
git add versions.json && git commit -q -m "ci: auto-update versions.json (build #1000) [skip ci]" && git push -q origin HEAD:main
'''
rc, sk, m, commits = run_push('newer version races in', [STUTTER, CENTER], race_sh=RACE_NEWER, race_on='1')
check(rc == 0 and sk == 'stutter', 'stutter skipped on the retry')
check(plug(m, 'RoneStutter')['version'] == '1.5.7.1000', "the newer stutter stays")
c = m['center_installer']
check(c['version'] == '2.1.3.999' and c['url'].endswith('RONE_Plugins_Center_Installer-999.1.exe') and c['sha256'] == 'cw'
      and c['url_mac'].endswith('RONE_Plugins_Center-999.1.dmg') and c['sha256_mac'] == 'cm', 'Center entry: each url with its own hash')

# A6: a re-run - main already has exactly this version: nothing to commit, rc 0, nothing flagged.
live_same = json.loads(json.dumps(LIVE))
plug(live_same, 'RoneStutter')['version'] = '1.5.6.999'
rc, sk, m, commits = run_push('re-run, already live', [STUTTER], live=live_same)
check(rc == 0 and sk == '' and len(commits) == 1, 'no commit, nothing flagged')

# A7: three rejections in a row - gives up with rc 1 (the job then fails and says re-run).
RACE_ALWAYS = r'''
echo "$1" >> race.log; git add -A >/dev/null 2>&1; git commit -q --allow-empty -m "race $1" && git push -q origin HEAD:main
'''
rc, sk, m, commits = run_push('rejected three times', [STUTTER], race_sh=RACE_ALWAYS, race_on='1 2 3')
check(rc == 1, 'returns 1 after 3 attempts')
check(plug(m, 'RoneStutter')['version'] == '1.5.5.268', 'main keeps the old entry (consistent: old url + old hash)')

# A8: a newer BUILD owns the release (its notes say Build #1001) - left alone.
rc, sk, m, commits = run_push('newer build owns the asset', [STUTTER],
                              notes={'RoneStutter-latest': ('Build #1001 · x', [])})
check(rc == 0 and sk == 'stutter' and len(commits) == 1, 'left alone and flagged')

# ---------------------------------------------------------------------------
# B: the 'Bump versions.json' step itself
# ---------------------------------------------------------------------------
FAKE_CURL = '#!/bin/bash\nfor a in "$@"; do u="$a"; done\necho "curl $u" >> "$GH_LOG"\ncase " $CURL_FAIL " in *" $u "*) exit 22 ;; esac\nexit 0\n'


def run_bump(case, published, built, digests=None, curl_fail='', suffix='999.1', live=None):
    print('\n== ' + case)
    base = tempfile.mkdtemp(prefix='mbump-')
    origin, ci, other = repo(base, live or LIVE)
    bindir = os.path.join(base, 'bin')
    jq = install_jq(bindir)
    install_gh(bindir)
    write(os.path.join(bindir, 'curl'), FAKE_CURL, 0o755)
    assets = lambda tag: [{'id': i, 'name': n, 'digest': d} for i, (n, d) in enumerate((digests or {}).get(tag, {}).items())]
    st = gh_state(base, {t: ('Build #999 · x', assets(t)) for t in ('RoneStutter-latest', 'RoneThrow-latest', 'PluginsCenter-latest')})
    files = {'stutter': ('RoneStutter_Installer.exe', 'RoneStutter_Installer.pkg', '1.5.6.999'),
             'throw': ('RoneThrow_Installer.exe', 'RoneThrow_Installer.pkg', '1.8.4.999'),
             'plugins_center': ('RONE_Plugins_Center_Installer.exe', 'RONE_Plugins_Center.dmg', '2.1.3.999')}
    for pid in built:
        w, mfile, ver = files[pid]
        for plat, fn in (('windows', w), ('macos', mfile)):
            write(os.path.join(ci, 'artifacts', 'installer-%s-%s' % (pid, plat), fn), pid + plat)
            write(os.path.join(ci, 'buildinfo', 'buildinfo-%s-%s' % (pid, plat), 'version.txt'), ver)
    mat = json.dumps([{'plugin_id': i} for i in built], separators=(',', ':'))
    script = fill(step_script('update-manifest', 'Bump versions.json'), {
        "needs.release.outputs.published || '[]'": json.dumps(published, separators=(',', ':')),
        'needs.detect-changes.outputs.matrix_windows': mat, 'needs.detect-changes.outputs.matrix_macos': mat,
        'github.run_number': '999', 'github.repository': 'liranidan2000-max/rone-plugins', 'needs.release.outputs.suffix': suffix})
    sp = os.path.join(base, 'bump.sh')
    write(sp, script)
    rt = os.path.join(base, 'rt')
    os.makedirs(rt)
    env = {'GITHUB_OUTPUT': posix(os.path.join(base, 'out')), 'GITHUB_STEP_SUMMARY': posix(os.path.join(base, 'sum')),
           'RUNNER_TEMP': posix(rt), 'GH_STATE': posix(st), 'GH_LOG': posix(os.path.join(base, 'gh.log')), 'CURL_FAIL': curl_fail,
           'GITHUB_SHA': git(ci, 'rev-parse', 'HEAD')}
    r = run_bash(sp, ci, env, bindir)
    plan = open(os.path.join(rt, 'bump-plan.txt'), encoding='utf-8').read().splitlines() if os.path.exists(os.path.join(rt, 'bump-plan.txt')) else None
    outs = read_outputs(os.path.join(base, 'out'))
    print('   (%s) exit %d outputs %s' % (jq, r.returncode, outs))
    for l in (r.stdout + r.stderr).splitlines():
        if '::' in l or 'planned' in l:
            print('    | ' + l)
    shutil.rmtree(base, ignore_errors=True)
    return r.returncode, plan, outs


import hashlib
sha = lambda s: hashlib.sha256(s.encode()).hexdigest()
rc, plan, outs = run_bump('bump: two products', ['stutter', 'plugins_center'], ['stutter', 'plugins_center'])
check(rc == 0 and outs.get('guard_skipped') == '', 'step ok, nothing held back')
check(plan is not None and len(plan) == 2, 'two plan lines')
if plan and len(plan) == 2:
    f = plan[0].split('|')
    check(f[:4] == ['stutter', 'RoneStutter', '1.5.6.999', 'RoneStutter-latest'], 'stutter: id, jid, version, tag')
    check(f[4] == REL + 'RoneStutter-latest/RoneStutter_Installer-999.1.exe'
          and f[7] == REL + 'RoneStutter-latest/RoneStutter_Installer-999.1.pkg',
          'stutter: versioned exe + pkg URLs')
    check(f[5] == sha('stutterwindows') and f[8] == sha('stuttermacos') and f[6] == str(len('stutterwindows')), 'stutter: hashes and size of the artefacts')
    g = plan[1].split('|')
    check(g[4].endswith('/PluginsCenter-latest/RONE_Plugins_Center_Installer-999.1.exe')
          and g[7].endswith('/PluginsCenter-latest/RONE_Plugins_Center-999.1.dmg'), 'Center: versioned exe + dmg URLs')

rc, plan, outs = run_bump('bump: a URL does not answer', ['stutter', 'throw'], ['stutter', 'throw'],
                          curl_fail=REL + 'RoneThrow-latest/RoneThrow_Installer-999.1.pkg')
check(rc == 0 and outs.get('guard_skipped') == 'throw', 'throw held back (Verdict goes red)')
check(plan is not None and len(plan) == 1 and plan[0].startswith('stutter|'), 'only stutter planned')

rc, plan, outs = run_bump('bump: GitHub digest differs', ['stutter'], ['stutter'],
                          digests={'RoneStutter-latest': {'RoneStutter_Installer-999.1.exe': 'sha256:' + '0' * 64,
                                                          'RoneStutter_Installer-999.1.pkg': 'sha256:' + sha('stuttermacos')}})
check(rc == 0 and outs.get('guard_skipped') == 'stutter' and plan == [], 'held back on a digest mismatch')

rc, plan, outs = run_bump('bump: digest matches', ['stutter'], ['stutter'],
                          digests={'RoneStutter-latest': {'RoneStutter_Installer-999.1.exe': 'sha256:' + sha('stutterwindows'),
                                                          'RoneStutter_Installer-999.1.pkg': 'sha256:' + sha('stuttermacos')}})
check(rc == 0 and outs.get('guard_skipped') == '' and plan and len(plan) == 1, 'planned when the digests agree')

rc, plan, outs = run_bump('bump: no suffix from Release', ['stutter'], ['stutter'], suffix='')
check(rc != 0 and plan == [], 'fails loudly, nothing planned')

# R027: every URL a Center will follow must be a file on this repository's releases.
import copy
def live_with(pid, field, url):
    m = copy.deepcopy(LIVE)
    for p in m['plugins']:
        if p['id'] == pid: p[field] = url
    return m
rc, plan, outs = run_bump('R027: the live manifest passes', ['stutter'], ['stutter'])
check(rc == 0, 'real-repository URLs pass')
rc, plan, outs = run_bump('R027: another host on main', ['stutter'], ['stutter'],
                          live=live_with('RoneThrow', 'download_url', 'https://evil.example/RoneThrow_Installer.exe'))
check(rc != 0, 'a foreign host on main fails the step (nothing is pushed)')
rc, plan, outs = run_bump('R027: another repository on main', ['stutter'], ['stutter'],
                          live=live_with('RoneThrow', 'download_url_mac', 'https://github.com/someone/else/releases/download/x/RoneThrow_Installer.pkg'))
check(rc != 0, 'another repository fails the step')
rc, plan, outs = run_bump('R027: a dot segment on main', ['stutter'], ['stutter'],
                          live=live_with('RoneThrow', 'download_url', REL + '../RoneThrow_Installer.exe'))
check(rc != 0, 'a dot segment fails the step')
m = copy.deepcopy(LIVE); m['center_installer']['url_mac'] = 'http://github.com/liranidan2000-max/rone-plugins/releases/download/PluginsCenter-latest/RONE_Plugins_Center.dmg'
rc, plan, outs = run_bump('R027: plain http for the Center', ['stutter'], ['stutter'], live=m)
check(rc != 0, 'http:// fails the step')
m = copy.deepcopy(LIVE); del m['center_installer']['url_mac']
rc, plan, outs = run_bump('R027: a field that is absent', ['stutter'], ['stutter'], live=m)
check(rc == 0, 'an absent URL field is not an error')

print('\n' + ('ALL PASSED' if not failures else '%d FAILED' % len(failures)))
sys.exit(1 if failures else 0)
