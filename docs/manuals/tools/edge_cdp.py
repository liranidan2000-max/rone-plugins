"""Drive headless Edge over its own DevTools protocol.

Edge 153 (2026-09) prints nothing for `--dump-dom` - the launcher exits with
code 0 and an empty stdout in every headless mode - and `--screenshot` under
`--virtual-time-budget` no longer waits for the page's own timers, so the
doc-shot state (the `js:` / `click:` recipe in _shot.js) was never applied.
Both used to work. Instead of trusting flags, this talks to the page: navigate,
wait until _shot.js has posted the rects into the harness, then capture the
screenshot and read the rects from the same live page.

No websocket package on this machine, so the client is a few dozen lines of
hand-rolled RFC 6455 (HTTP upgrade, masked frames out, unmasked in).
"""
import base64, json, os, socket, struct, subprocess, tempfile, time, urllib.request


class WS:
    def __init__(self, url):
        _, rest = url.split("://", 1)
        hostport, path = rest.split("/", 1)
        host, port = hostport.split(":")
        self.s = socket.create_connection((host, int(port)), timeout=60)
        key = base64.b64encode(os.urandom(16)).decode()
        req = (f"GET /{path} HTTP/1.1\r\nHost: {hostport}\r\nUpgrade: websocket\r\n"
               f"Connection: Upgrade\r\nSec-WebSocket-Key: {key}\r\nSec-WebSocket-Version: 13\r\n\r\n")
        self.s.sendall(req.encode())
        buf = b""
        while b"\r\n\r\n" not in buf:
            buf += self.s.recv(4096)
        self.buf = buf.split(b"\r\n\r\n", 1)[1]
        self.id = 0

    def send(self, method, params=None):
        self.id += 1
        payload = json.dumps({"id": self.id, "method": method, "params": params or {}}).encode()
        hdr = bytearray([0x81])
        n = len(payload)
        if n < 126: hdr.append(0x80 | n)
        elif n < 65536: hdr.append(0x80 | 126); hdr += struct.pack(">H", n)
        else: hdr.append(0x80 | 127); hdr += struct.pack(">Q", n)
        mask = os.urandom(4); hdr += mask
        self.s.sendall(bytes(hdr) + bytes(b ^ mask[i % 4] for i, b in enumerate(payload)))
        return self.id

    def _read(self, n):
        while len(self.buf) < n:
            chunk = self.s.recv(1 << 20)
            if not chunk: raise EOFError("devtools socket closed")
            self.buf += chunk
        out, self.buf = self.buf[:n], self.buf[n:]
        return out

    def recv(self):
        b1, b2 = self._read(2)
        n = b2 & 0x7F
        if n == 126: n = struct.unpack(">H", self._read(2))[0]
        elif n == 127: n = struct.unpack(">Q", self._read(8))[0]
        if b2 & 0x80: self._read(4)
        data = self._read(n)
        return json.loads(data) if (b1 & 0x0F) == 1 else None

    def call(self, method, params=None):
        i = self.send(method, params)
        while True:
            m = self.recv()
            if m and m.get("id") == i:
                if "error" in m: raise RuntimeError(f"{method}: {m['error']}")
                return m.get("result", {})

    def eval(self, expr):
        r = self.call("Runtime.evaluate", {"expression": expr, "returnByValue": True})
        if "exceptionDetails" in r:
            raise RuntimeError(r["exceptionDetails"].get("text", "evaluate failed"))
        return r.get("result", {}).get("value")


def _free_port():
    with socket.socket() as s:
        s.bind(("127.0.0.1", 0)); return s.getsockname()[1]


class Edge:
    """A headless Edge with one page, driven over CDP. Use as a context manager."""
    def __init__(self, exe, width, height, dpr):
        self.exe, self.w, self.h, self.dpr = exe, width, height, dpr
        self.port = _free_port()
        self.prof = tempfile.mkdtemp(prefix="edgeshot_")
        self.p = None; self.ws = None

    def __enter__(self):
        self.p = subprocess.Popen([self.exe, "--headless=new", "--disable-gpu", "--hide-scrollbars", "--no-first-run",
                                   "--disable-extensions", "--no-default-browser-check", f"--user-data-dir={self.prof}",
                                   f"--window-size={self.w},{self.h}", f"--force-device-scale-factor={self.dpr}",
                                   f"--remote-debugging-port={self.port}", "about:blank"],
                                  stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        target = None
        for _ in range(100):
            try:
                with urllib.request.urlopen(f"http://127.0.0.1:{self.port}/json", timeout=1) as r:
                    for t in json.load(r):
                        if t.get("type") == "page": target = t["webSocketDebuggerUrl"]; break
            except Exception:
                pass
            if target: break
            time.sleep(0.2)
        if not target: self.__exit__(); raise RuntimeError("headless Edge exposed no page target")
        self.ws = WS(target)
        self.ws.call("Page.enable"); self.ws.call("Runtime.enable")
        self.ws.call("Emulation.setDeviceMetricsOverride",
                     {"width": self.w, "height": self.h, "deviceScaleFactor": self.dpr, "mobile": False})
        return self

    def goto(self, url):
        self.ws.call("Page.navigate", {"url": url})

    def wait_for(self, expr, timeout=30.0, interval=0.25):
        """Poll a JS expression until it is truthy; returns its value."""
        t0 = time.time()
        while time.time() - t0 < timeout:
            try:
                v = self.ws.eval(expr)
                if v: return v
            except Exception:
                pass
            time.sleep(interval)
        raise TimeoutError(f"gave up waiting for {expr[:60]}")

    def screenshot(self, path, clip_w, clip_h):
        r = self.ws.call("Page.captureScreenshot",
                         {"format": "png", "captureBeyondViewport": False,
                          "clip": {"x": 0, "y": 0, "width": clip_w, "height": clip_h, "scale": 1}})
        with open(path, "wb") as f: f.write(base64.b64decode(r["data"]))

    def __exit__(self, *a):
        try:
            if self.ws: self.ws.s.close()
        except Exception:
            pass
        if self.p:
            subprocess.run(["taskkill", "/F", "/T", "/PID", str(self.p.pid)], capture_output=True)
            try: self.p.wait(5)
            except Exception: pass
        import shutil
        shutil.rmtree(self.prof, ignore_errors=True)
