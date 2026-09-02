"""Extract the embedded WebView HTML from a RONE WebUI.h into a standalone file.
Resolves getIndexHTML() by concatenating, in order of appearance, every raw
string literal and every getX() call inside each function body."""
import re, sys, pathlib

RAW = re.compile(r'R"rawhtml\((.*?)\)rawhtml"', re.S)
FUNC = re.compile(r'inline\s+(?:const\s+)?std::string&?\s+(get\w+)\s*\(\s*\)\s*\{', re.S)

def parse(src):
    funcs = {}
    for m in FUNC.finditer(src):
        name = m.group(1)
        # find matching closing brace by scanning (raw strings may contain braces -> mask them first)
        start = m.end()
        masked = RAW.sub(lambda mm: 'R"rawhtml(' + ('\x00' * len(mm.group(1))) + ')rawhtml"', src)
        depth, i = 1, start
        while depth and i < len(masked):
            c = masked[i]
            if c == '{': depth += 1
            elif c == '}': depth -= 1
            i += 1
        funcs[name] = src[start:i-1]
    return funcs

def resolve(name, funcs, seen=()):
    body = funcs[name]
    out = []
    tokens = re.finditer(r'R"rawhtml\((.*?)\)rawhtml"|\b(get\w+)\s*\(\s*\)', body, re.S)
    for t in tokens:
        if t.group(1) is not None:
            out.append(t.group(1))
        else:
            fn = t.group(2)
            if fn in funcs and fn != name and fn not in seen:
                out.append(resolve(fn, funcs, seen + (name,)))
    return ''.join(out)

if __name__ == '__main__':
    src = pathlib.Path(sys.argv[1]).read_text(encoding='utf-8')
    funcs = parse(src)
    html = resolve('getIndexHTML', funcs)
    pathlib.Path(sys.argv[2]).write_text(html, encoding='utf-8')
    print(f"{sys.argv[2]}: {len(html)} bytes from {list(funcs)}")
