"""Syntax check of .github/workflows/main.yml without GitHub.

  python scripts/ci/tests/workflow_syntax_test.py

Parses the YAML, then hands every `run:` block to its own shell's parser:
bash -n for the bash steps (ubuntu / macOS, or shell: bash), and PowerShell's
parser for the pwsh steps (the Windows default). `${{ ... }}` expressions are
replaced by a placeholder first, the way the runner substitutes them.
Also checks that every `needs.<job>.outputs.<name>` that a job reads is
declared by that job. Needs Python 3 with PyYAML, bash, and powershell.exe or
pwsh for the PowerShell half (skipped with a note when neither exists).
"""
import os, re, shutil, subprocess, sys, tempfile
import yaml

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, '..', '..', '..'))
YML = os.path.join(ROOT, '.github', 'workflows', 'main.yml')
EXPR = re.compile(r'\$\{\{.*?\}\}', re.S)


def bash_exe():
    for c in (shutil.which('bash'), r'C:\Program Files\Git\usr\bin\bash.exe', r'C:\Program Files\Git\bin\bash.exe'):
        if c and os.path.exists(c):
            return c
    sys.exit('bash not found')


def ps_exe():
    """pwsh (7, what the runner uses) first; Windows PowerShell 5.1 as a fallback,
    which does not know the pipeline chain operators || and && (see below)."""
    return shutil.which('pwsh') or shutil.which('powershell') or shutil.which('powershell.exe')


# Windows PowerShell 5.1 rejects `a || b` / `a && b`, which pwsh 7 accepts.
CHAIN_ONLY = re.compile(r"The token '(\|\||&&)' is not a valid statement separator")


def main():
    text = open(YML, encoding='utf-8').read()
    wf = yaml.safe_load(text)
    jobs = wf['jobs']
    fails = 0
    bash, ps = bash_exe(), ps_exe()
    tmp = tempfile.mkdtemp(prefix='wfsyntax-')
    n_bash = n_ps = 0
    for jname, job in jobs.items():
        runs_on = str(job.get('runs-on', ''))
        default_shell = 'pwsh' if runs_on.startswith('windows') else 'bash'
        for i, step in enumerate(job.get('steps', [])):
            if 'run' not in step:
                continue
            shell = step.get('shell', default_shell)
            body = EXPR.sub('EXPR', step['run'])
            label = '%s / %s' % (jname, step.get('name', '#%d' % i))
            if shell == 'bash':
                p = os.path.join(tmp, 'step%d.sh' % n_bash)
                open(p, 'w', encoding='utf-8', newline='\n').write(body)
                r = subprocess.run([bash, '-n', p], capture_output=True, text=True)
                n_bash += 1
            elif shell in ('pwsh', 'powershell'):
                if not ps:
                    continue
                p = os.path.join(tmp, 'step%d.ps1' % n_ps)
                open(p, 'w', encoding='utf-8-sig', newline='\r\n').write(body)
                cmd = ("$e=$null;$t=$null;[void][System.Management.Automation.Language.Parser]::ParseFile('%s',[ref]$t,[ref]$e);"
                       "if($e.Count){$e|%%{'line '+$_.Extent.StartLineNumber+': '+$_.Message};exit 1}" % p.replace("'", "''"))
                r = subprocess.run([ps, '-NoProfile', '-NonInteractive', '-Command', cmd], capture_output=True, text=True)
                n_ps += 1
                if r.returncode != 0 and 'pwsh' not in os.path.basename(ps).lower():
                    errs = [l for l in r.stdout.splitlines() if l.strip()]
                    if errs and all(CHAIN_ONLY.search(l) for l in errs):
                        r = subprocess.CompletedProcess(r.args, 0, '', '')
            else:
                continue
            if r.returncode != 0:
                fails += 1
                print('FAIL %s (%s)\n%s%s' % (label, shell, r.stdout, r.stderr))
    # needs.<job>.outputs.<name> must be declared by <job>
    for m in re.finditer(r'needs\.([A-Za-z0-9_-]+)\.outputs\.([A-Za-z0-9_]+)', text):
        job, out = m.group(1), m.group(2)
        declared = (jobs.get(job) or {}).get('outputs') or {}
        if out not in declared:
            fails += 1
            print('FAIL needs.%s.outputs.%s is not declared by job %s' % (job, out, job))
    # steps.<id>.outputs read inside a job must name a step id of that job
    for jname, job in jobs.items():
        ids = {s.get('id') for s in job.get('steps', []) if s.get('id')}
        blob = yaml.safe_dump(job)
        for m in re.finditer(r'steps\.([A-Za-z0-9_-]+)\.outputs', blob):
            if m.group(1) not in ids:
                fails += 1
                print('FAIL job %s reads steps.%s but has no step with that id' % (jname, m.group(1)))
    shutil.rmtree(tmp, ignore_errors=True)
    print('checked %d bash and %d PowerShell run blocks%s' % (n_bash, n_ps, '' if ps else ' (no PowerShell found)'))
    print('all passed' if not fails else '%d failure(s)' % fails)
    return 1 if fails else 0


if __name__ == '__main__':
    sys.exit(main())
