"""A minimal Chrome DevTools Protocol client.

There is no websocket package on the build machines, and pulling one in for
two calls is not worth a dependency, so the RFC 6455 client half is written
out here: HTTP upgrade, masked frames out, unmasked frames in. It only has to
carry a handful of JSON messages and one screenshot.

Used to photograph WebView2 UIs. The PrintWindow route that capture_window.ps1
uses cannot see them - WebView2 draws through DirectComposition and PrintWindow
returns a solid black bitmap, which the pipeline would happily annotate and
ship. Talking to the view's own debugger is the only way to get its pixels.
"""
from __future__ import annotations

import base64
import json
import secrets
import socket
import struct
import time
import urllib.error
import urllib.request


def list_targets(port: int):
    req = urllib.request.Request(f"http://127.0.0.1:{port}/json",
                                 headers={"User-Agent": "rone-manuals"})
    with urllib.request.urlopen(req, timeout=5) as r:
        return json.load(r)


def wait_for_page(port: int, timeout: float = 30.0):
    """Blocks until the view's debugger is up and has a page. Returns the target."""
    deadline = time.time() + timeout
    last = None
    while time.time() < deadline:
        try:
            pages = [t for t in list_targets(port) if t.get("type") == "page"]
            if pages:
                return pages[0]
        except (urllib.error.URLError, OSError, ConnectionError) as e:
            last = e
        time.sleep(0.5)
    raise RuntimeError(f"no debuggable page on port {port} within {timeout:g}s ({last})")


class Connection:
    """One websocket to one DevTools target."""

    def __init__(self, ws_url: str):
        if not ws_url.startswith("ws://"):
            raise ValueError(f"not a ws url: {ws_url}")
        hostport, _, path = ws_url[5:].partition("/")
        host, _, port = hostport.partition(":")
        self.sock = socket.create_connection((host, int(port or 80)), timeout=30)
        key = base64.b64encode(secrets.token_bytes(16)).decode()
        self.sock.sendall(
            (f"GET /{path} HTTP/1.1\r\n"
             f"Host: {hostport}\r\n"
             f"Upgrade: websocket\r\n"
             f"Connection: Upgrade\r\n"
             f"Sec-WebSocket-Key: {key}\r\n"
             f"Sec-WebSocket-Version: 13\r\n\r\n").encode())

        buf = b""
        while b"\r\n\r\n" not in buf:
            chunk = self.sock.recv(4096)
            if not chunk:
                raise RuntimeError("handshake closed early")
            buf += chunk
        status = buf.split(b"\r\n", 1)[0]
        if b"101" not in status:
            raise RuntimeError(f"upgrade refused: {status.decode(errors='replace')}")
        self._buf = buf.split(b"\r\n\r\n", 1)[1]
        self._next_id = 0

    # -- framing ----------------------------------------------------------
    def _read(self, n: int) -> bytes:
        while len(self._buf) < n:
            chunk = self.sock.recv(1 << 16)
            if not chunk:
                raise RuntimeError("connection closed")
            self._buf += chunk
        out, self._buf = self._buf[:n], self._buf[n:]
        return out

    def _send(self, obj) -> None:
        data = json.dumps(obj).encode()
        mask = secrets.token_bytes(4)
        n = len(data)
        head = b"\x81"
        if n < 126:
            head += bytes([0x80 | n])
        elif n < 65536:
            head += bytes([0x80 | 126]) + struct.pack(">H", n)
        else:
            head += bytes([0x80 | 127]) + struct.pack(">Q", n)
        self.sock.sendall(head + mask + bytes(b ^ mask[i % 4] for i, b in enumerate(data)))

    def _recv(self):
        payload = b""
        while True:
            b0, b1 = self._read(2)
            fin = b0 & 0x80
            n = b1 & 0x7F
            if n == 126:
                n = struct.unpack(">H", self._read(2))[0]
            elif n == 127:
                n = struct.unpack(">Q", self._read(8))[0]
            payload += self._read(n)
            if fin:
                return json.loads(payload.decode())

    # -- protocol ---------------------------------------------------------
    def call(self, method: str, params: dict | None = None) -> dict:
        self._next_id += 1
        mid = self._next_id
        self._send({"id": mid, "method": method, "params": params or {}})
        while True:
            msg = self._recv()
            if msg.get("id") == mid:
                if "error" in msg:
                    raise RuntimeError(f"{method}: {msg['error']}")
                return msg.get("result", {})

    def evaluate(self, expression: str):
        """Evaluates in the page and returns the value, raising what the page raised."""
        res = self.call("Runtime.evaluate",
                        {"expression": expression, "returnByValue": True,
                         "awaitPromise": True})
        if res.get("exceptionDetails"):
            d = res["exceptionDetails"]
            raise RuntimeError("page threw: "
                               + str(d.get("exception", {}).get("description") or d.get("text")))
        return res.get("result", {}).get("value")

    def close(self) -> None:
        try:
            self.sock.close()
        except OSError:
            pass
