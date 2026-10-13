"""Runs main.yml's 'Create per-plugin releases' step (and the Verdict) locally
against a scratch git repo and a fake gh, and checks what it uploads.

  python scripts/ci/tests/ci_release_test.py

Needs Python 3 with PyYAML, git and bash (Git Bash on Windows). jq is used when
installed, a stand-in otherwise (see ci_harness.py). Nothing leaves the machine.
"""
import json, os, shutil, sys, tempfile
sys.stdout.reconfigure(encoding="utf-8", errors="replace")
from ci_harness import (ROOT, step_script, fill, write, copy_lf, install_jq, install_gh,
                        git, run_bash, read_outputs, posix)

RUN = '999'
LIVE = {  # what origin/main's versions.json says is live
    'center_installer': {'version': '2.1.2.264'},
    'plugins': [{'id': 'RoneStutter', 'version': '1.5.5.268'},
                {'id': 'RoneThrow', 'version': '1.8.3.270'},
                {'id': 'RONEAnalyzer', 'version': '1.3.13.271'}],
}
PRODUCTS = {  # id -> (folder, base version, windows file, mac file)
    'stutter': ('RoneStutter', '1.5.6', 'RoneStutter_Installer.exe', 'RoneStutter_Installer.pkg'),
    'throw': ('RoneThrow', '1.8.4', 'RoneThrow_Installer.exe', 'RoneThrow_Installer.pkg'),
    'plugins_center': ('RonePluginsCenter', '2.1.3', 'RONE_Plugins_Center_Installer.exe', 'RONE_Plugins_Center.dmg'),
    'analyzer': ('RONEAnalyzer', '1.3.13', 'RONEAnalyzer_Installer.exe', 'RONEAnalyzer_Installer.pkg'),
}
TAGS = {'stutter': 'RoneStutter-latest', 'throw': 'RoneThrow-latest',
        'plugins_center': 'PluginsCenter-latest', 'analyzer': 'RONEAnalyzer-latest'}

failures = []


def check(cond, what):
    print(('  ok   ' if cond else '  FAIL ') + what)
    if not cond:
        failures.append(what)


def fixture(base):
    """origin (bare) + a clone at main with the product folders and versions.json."""
    origin = os.path.join(base, 'origin.git')
    work = os.path.join(base, 'work')
    os.makedirs(work)
    git(base, 'init', '-q', '--bare', '-b', 'main', origin)
    git(work, 'init', '-q', '-b', 'main')
    git(work, 'config', 'user.email', 't@example.invalid')
    git(work, 'config', 'user.name', 'test')
    git(work, 'config', 'core.autocrlf', 'false')
    for pid, (folder, base_v, _, _) in PRODUCTS.items():
        write(os.path.join(work, folder, 'CMakeLists.txt'), 'project(x VERSION %s)\n' % base_v)
    write(os.path.join(work, 'versions.json'), json.dumps(LIVE, indent=2) + '\n')
    copy_lf(os.path.join(ROOT, 'scripts', 'ci', 'release-guard.sh'), os.path.join(work, 'scripts', 'ci', 'release-guard.sh'))
    git(work, 'add', '-A')
    git(work, 'commit', '-q', '-m', 'fixture')
    git(work, 'remote', 'add', 'origin', origin)
    git(work, 'push', '-q', 'origin', 'main')
    return work, git(work, 'rev-parse', 'HEAD')


def run_release(case, changed, built, win_ids=None, mac_ids=None, releases=None, gh_fail='',
                create_fail=False, versions=None):
    """built: {id: ['windows', 'macos']} - which installers exist. releases: {tag: [asset names]}."""
    print('\n== ' + case)
    base = tempfile.mkdtemp(prefix='cirel-')
    work, sha = fixture(base)
    bindir = os.path.join(base, 'bin')
    jq = install_jq(bindir)
    install_gh(bindir)
    state = os.path.join(base, 'ghstate')
    os.makedirs(state)
    for tag, names in (releases or {}).items():
        json.dump({'assets': [{'id': 100 + i, 'name': n} for i, n in enumerate(names)],
                   'body': 'Build #998 · 2026-10-12'}, open(os.path.join(state, tag + '.json'), 'w'))
    for pid, plats in built.items():
        folder, base_v, wf, mf = PRODUCTS[pid]
        ver = (versions or {}).get(pid, base_v + '.' + RUN)
        for plat in plats:
            write(os.path.join(work, 'artifacts', 'installer-%s-%s' % (pid, plat), wf if plat == 'windows' else mf), pid + plat)
            write(os.path.join(work, 'buildinfo', 'buildinfo-%s-%s' % (pid, plat), 'version.txt'), ver)
    win_ids = changed if win_ids is None else win_ids
    mac_ids = changed if mac_ids is None else mac_ids
    mat = lambda ids: json.dumps([{'plugin_id': i} for i in ids], separators=(',', ':'))
    script = fill(step_script('release', 'Create per-plugin releases'), {
        'github.repository': 'owner/repo',
        'github.run_number': RUN,
        'github.run_attempt': '1',
        'needs.detect-changes.outputs.changed': json.dumps(changed, separators=(',', ':')),
        'needs.detect-changes.outputs.matrix_windows': mat(win_ids),
        'needs.detect-changes.outputs.matrix_macos': mat(mac_ids),
    })
    sp = os.path.join(base, 'step.sh')
    write(sp, script)
    out_file = os.path.join(base, 'out.txt')
    env = {'GITHUB_SHA': sha, 'GITHUB_OUTPUT': posix(out_file), 'GITHUB_STEP_SUMMARY': posix(os.path.join(base, 'summary.md')),
           'RUNNER_TEMP': posix(os.path.join(base, 'rt')), 'GH_STATE': posix(state), 'GH_LOG': posix(os.path.join(base, 'gh.log')),
           'GH_FAIL': gh_fail, 'GH_CREATE_FAIL': '1' if create_fail else ''}
    os.makedirs(os.path.join(base, 'rt'))
    r = run_bash(sp, work, env, bindir)
    log = open(os.path.join(base, 'gh.log'), encoding='utf-8').read() if os.path.exists(os.path.join(base, 'gh.log')) else ''
    outs = read_outputs(out_file)
    ups = [l.split(' ', 2)[1:] for l in log.splitlines() if l.startswith('UPLOADED')]
    print('   (%s) exit %d, outputs %s' % (jq, r.returncode, outs))
    for l in (r.stdout + r.stderr).splitlines():
        if '::' in l or 'moved' in l or '✓' in l:
            print('    | ' + l)
    final = {}
    for fn in os.listdir(state):
        if fn.endswith('.json') and not fn.startswith('_'):
            final[fn[:-5]] = [x['name'] for x in json.load(open(os.path.join(state, fn)))['assets']]
    shutil.rmtree(base, ignore_errors=True)
    return r.returncode, outs, ups, log, final


def verdict(**outs):
    vals = {'needs.build-windows.result': 'success', 'needs.build-macos.result': 'success',
            'needs.release.result': 'success', 'needs.update-manifest.result': 'success',
            'needs.release.outputs.guard_skipped': '', 'needs.update-manifest.outputs.guard_skipped': '',
            'needs.release.outputs.upload_failed': '', 'needs.release.outputs.alias_failed': '',
            'needs.update-manifest.outputs.push_skipped': ''}
    vals.update(outs)
    script = fill(step_script('verdict', 'Fail if anything did not build'), vals)
    base = tempfile.mkdtemp(prefix='civer-')
    sp = os.path.join(base, 'v.sh')
    write(sp, script)
    r = run_bash(sp, base, {}, base)
    shutil.rmtree(base, ignore_errors=True)
    return r.returncode, r.stdout


# 1. Normal: both platforms, existing release. Versioned names first, then the aliases.
rc, outs, ups, log, final = run_release('normal release', ['stutter'], {'stutter': ['windows', 'macos']},
                                        releases={'RoneStutter-latest': ['RoneStutter_Installer.exe', 'RoneStutter_Installer.pkg']})
names = [u[1] for u in ups]
check(rc == 0, 'step exits 0')
check(outs.get('published') == '["stutter"]', 'published = [stutter]')
check(outs.get('suffix') == RUN + '.1', 'suffix = <run>.<attempt>')
check(outs.get('upload_failed') == '' and outs.get('alias_failed') == '', 'no failures reported')
check(sorted(names[:2]) == ['RoneStutter_Installer-999.1.exe', 'RoneStutter_Installer-999.1.pkg'], 'versioned files go up first')
check(sorted(names[2:]) == ['RoneStutter_Installer.exe', 'RoneStutter_Installer.pkg'], 'then the unversioned aliases')
check(sorted(final['RoneStutter-latest']) == sorted(['RoneStutter_Installer.exe', 'RoneStutter_Installer.pkg',
                                                     'RoneStutter_Installer-999.1.exe', 'RoneStutter_Installer-999.1.pkg']),
      'the release holds aliases + versioned files')
check('EDITED RoneStutter-latest' in log, 'release notes moved to this build')
check(not any(n.startswith('tmp-') for n in names) and 'RENAMED' not in log, 'a plugin alias is uploaded as before (no swap)')

# 2. R030: the second product's versioned .pkg fails 3 times - the first is still published.
rc, outs, ups, log, final = run_release('upload failure mid-loop (R030)', ['stutter', 'throw'],
                                        {'stutter': ['windows', 'macos'], 'throw': ['windows', 'macos']},
                                        releases={'RoneStutter-latest': [], 'RoneThrow-latest': ['RoneThrow_Installer.exe']},
                                        gh_fail='RoneThrow_Installer-999.1.pkg:3')
check(rc == 0, 'step still exits 0')
check(outs.get('published') == '["stutter"]', 'published = [stutter] only')
check(outs.get('upload_failed') == 'throw', 'upload_failed = throw')
check(not any(u[1] == 'RoneThrow_Installer.exe' for u in ups), "throw's alias was not touched")
check('EDITED RoneThrow-latest' not in log, "throw's release notes still name the old build")
check(final['RoneThrow-latest'].count('RoneThrow_Installer.exe') == 1, "throw's previous alias is still there")

# 3. A stuck upload (500 once) is moved aside and the retry lands.
rc, outs, ups, log, final = run_release('stuck once', ['plugins_center'], {'plugins_center': ['windows', 'macos']},
                                        releases={'PluginsCenter-latest': ['RONE_Plugins_Center_Installer.exe', 'RONE_Plugins_Center.dmg']},
                                        gh_fail='RONE_Plugins_Center-999.1.dmg:1')
check(rc == 0 and outs.get('published') == '["plugins_center"]', 'Center published')
check(sorted(u[1] for u in ups if '999' in u[1] and not u[1].startswith('tmp-')) == ['RONE_Plugins_Center-999.1.dmg', 'RONE_Plugins_Center_Installer-999.1.exe'],
      'Center versioned names: RONE_Plugins_Center_Installer-<s>.exe, RONE_Plugins_Center-<s>.dmg')

# 4. R289: leftovers stale-* and *-stuck-* are deleted, nothing else is.
rc, outs, ups, log, final = run_release('leftover cleanup (R289)', ['plugins_center'], {'plugins_center': ['windows', 'macos']},
                                        releases={'PluginsCenter-latest': ['RONE_Plugins_Center_Installer.exe', 'RONE_Plugins_Center.dmg',
                                                                           'RONE_Plugins_Center-stuck-616274444.dmg',
                                                                           'stale-5-RONE_Plugins_Center.dmg',
                                                                           'RONE_Plugins_Center-998.1.dmg']})
check('DELETED PluginsCenter-latest RONE_Plugins_Center-stuck-616274444.dmg' in log, 'the -stuck- dmg is deleted')
check('DELETED PluginsCenter-latest stale-5-RONE_Plugins_Center.dmg' in log, 'the stale- leftover is deleted')
check('RONE_Plugins_Center-998.1.dmg' in final['PluginsCenter-latest'], "an older build's versioned file is kept")

# 5. A new tag: created empty, then filled one file at a time.
rc, outs, ups, log, final = run_release('new release', ['analyzer'], {'analyzer': ['windows', 'macos']})
check(rc == 0 and outs.get('published') == '["analyzer"]', 'published')
check('CREATED RONEAnalyzer-latest files=0' in log, 'gh release create carries no files')
check(len([u for u in ups if u[0] == 'RONEAnalyzer-latest']) == 4, 'then 2 versioned + 2 aliases uploaded')

# 6. Creating the release fails: that product only is reported, the step goes on.
rc, outs, ups, log, final = run_release('release create fails', ['analyzer', 'stutter'],
                                        {'analyzer': ['windows', 'macos'], 'stutter': ['windows', 'macos']},
                                        releases={'RoneStutter-latest': []}, create_fail=True)
check(rc == 0 and outs.get('published') == '["stutter"]' and outs.get('upload_failed') == 'analyzer', 'analyzer reported, stutter published')

# 7. The alias fails 3 times: the product is published (its versioned files are up), alias_failed says so.
rc, outs, ups, log, final = run_release('alias failure', ['stutter'], {'stutter': ['windows', 'macos']},
                                        releases={'RoneStutter-latest': ['RoneStutter_Installer.exe']},
                                        gh_fail='RoneStutter_Installer.exe:3')
check(rc == 0 and outs.get('published') == '["stutter"]', 'published')
check(outs.get('alias_failed') == 'stutter:RoneStutter_Installer.exe', 'alias_failed names the file')

# 8. Built on Windows only although meant for both: nothing of it goes out (#253 rule kept).
rc, outs, ups, log, final = run_release('partial build', ['plugins_center', 'stutter'],
                                        {'plugins_center': ['windows'], 'stutter': ['windows', 'macos']},
                                        releases={'PluginsCenter-latest': [], 'RoneStutter-latest': []})
check(rc == 0 and outs.get('published') == '["stutter"]', 'only stutter published')
check(not any(u[0] == 'PluginsCenter-latest' for u in ups), 'nothing uploaded for the half-built Center')

# 9. The release guard: a version not newer than live is held back before any upload.
rc, outs, ups, log, final = run_release('guard: not newer', ['throw'], {'throw': ['windows', 'macos']},
                                        releases={'RoneThrow-latest': []}, versions={'throw': '1.8.3.270'})
check(rc == 0 and outs.get('published') == '[]' and outs.get('guard_skipped') == 'throw' and not ups, 'held back, nothing uploaded')

# 11. R090: the Center's alias is swapped, never deleted first.
CEN = ['RONE_Plugins_Center_Installer.exe', 'RONE_Plugins_Center.dmg']
rc, outs, ups, log, final = run_release('Center alias swap (R090)', ['plugins_center'], {'plugins_center': ['windows', 'macos']},
                                        releases={'PluginsCenter-latest': CEN})
fin = final['PluginsCenter-latest']
check(rc == 0 and outs.get('published') == '["plugins_center"]' and outs.get('alias_failed') == '', 'published, no alias failure')
check(all(fin.count(n) == 1 for n in CEN), 'each alias name is there exactly once')
check(sorted(n for n in fin if n.startswith('stale-')) == ['stale-100-RONE_Plugins_Center_Installer.exe', 'stale-101-RONE_Plugins_Center.dmg'],
      'the previous files are kept aside as stale-<id>-<name>')
check(not any(n.startswith('tmp-') for n in fin), 'no tmp- asset is left')
check(not any(l.startswith('DELETED PluginsCenter-latest RONE_Plugins_Center') for l in log.splitlines()), 'the alias is never deleted')
seq = [l for l in log.splitlines() if l.startswith(('UPLOADED PluginsCenter-latest tmp-', 'RENAMED PluginsCenter-latest'))]
for nm, oid in (('RONE_Plugins_Center_Installer.exe', 100), ('RONE_Plugins_Center.dmg', 101)):
    mine = [l for l in seq if l.endswith(nm) or (' %s -> ' % nm) in l]
    check(mine == ['UPLOADED PluginsCenter-latest tmp-999-%s' % nm,
                   'RENAMED PluginsCenter-latest %s -> stale-%d-%s' % (nm, oid, nm),
                   'RENAMED PluginsCenter-latest tmp-999-%s -> %s' % (nm, nm)], '%s: upload tmp, old aside, tmp takes the name' % nm)

# 12. R090: the tmp upload keeps failing - the previous alias keeps its name, alias_failed says so.
rc, outs, ups, log, final = run_release('Center alias swap fails (R090)', ['plugins_center'], {'plugins_center': ['windows', 'macos']},
                                        releases={'PluginsCenter-latest': CEN}, gh_fail='tmp-999-RONE_Plugins_Center.dmg:3')
fin = final['PluginsCenter-latest']
check(rc == 0 and outs.get('published') == '["plugins_center"]', 'published (the versioned files are up)')
check(outs.get('alias_failed') == 'plugins_center:RONE_Plugins_Center.dmg', 'alias_failed names the dmg')
check('RONE_Plugins_Center.dmg' in fin and 'stale-101-RONE_Plugins_Center.dmg' not in fin, 'the previous dmg still answers under its name')
check('RONE_Plugins_Center_Installer.exe' in fin, 'the exe alias was swapped')

# 13. R090: no alias on the Center tag yet - a plain upload, as before.
rc, outs, ups, log, final = run_release('Center alias, first upload', ['plugins_center'], {'plugins_center': ['windows', 'macos']},
                                        releases={'PluginsCenter-latest': []})
check(rc == 0 and sorted(n for n in final['PluginsCenter-latest'] if '999' not in n) == sorted(CEN) and 'RENAMED' not in log,
      'aliases uploaded directly when none exists')

# 10. Verdict: red for every way Release can now go on past a product.
check(verdict()[0] == 0, 'verdict green when all is well')
check(verdict(**{'needs.release.outputs.upload_failed': 'throw'})[0] == 1, 'verdict red on upload_failed')
check(verdict(**{'needs.release.outputs.alias_failed': 'stutter:x.exe'})[0] == 1, 'verdict red on alias_failed')
check(verdict(**{'needs.release.outputs.guard_skipped': 'throw'})[0] == 1, 'verdict red on guard_skipped')
check(verdict(**{'needs.release.result': 'skipped', 'needs.update-manifest.result': 'skipped'})[0] == 0,
      'verdict green when Release is skipped (dry run)')

print('\n' + ('ALL PASSED' if not failures else '%d FAILED' % len(failures)))
sys.exit(1 if failures else 0)
