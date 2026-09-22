#!/usr/bin/env python3
# SPDX-License-Identifier: Apache-2.0
"""Build and serve the actual firmware UI as a loopback-only, in-memory simulator."""

import argparse
import json
import os
from pathlib import Path
import subprocess
import tempfile
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer

ROOT = Path(__file__).resolve().parents[1]
FRAME_BYTES = 320 * 480 * 2


class Simulator:
    def __init__(self, executable):
        self.process = subprocess.Popen(
            [str(executable)],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            bufsize=FRAME_BYTES * 2,
        )
        self.lock = threading.Lock()
        self.started = time.monotonic()

    def render(self, command="frame", a=0, b=0):
        with self.lock:
            now = int((time.monotonic() - self.started) * 1000)
            self.process.stdin.write(f"{command} {a} {b} {now}\n".encode())
            self.process.stdin.flush()
            metadata = self.process.stdout.readline(2048)
            if not metadata.endswith(b"\n"):
                raise RuntimeError("Simulator worker stopped responding")
            state = json.loads(metadata)
            frame = self.process.stdout.read(FRAME_BYTES)
            if len(frame) != FRAME_BYTES:
                raise RuntimeError("Incomplete simulator frame")
            return state, frame

    def close(self):
        self.process.terminate()
        self.process.wait(timeout=5)


def serve(executable, port):
    simulator = Simulator(executable)

    class Handler(BaseHTTPRequestHandler):
        def log_message(self, *_):
            pass

        def respond(self, status, data, kind="text/plain", state=None):
            self.send_response(status)
            self.send_header("Content-Type", kind)
            self.send_header("Content-Length", str(len(data)))
            self.send_header("Cache-Control", "no-store")
            self.send_header("X-Content-Type-Options", "nosniff")
            self.send_header(
                "Content-Security-Policy",
                "default-src 'self'; style-src 'self'; script-src 'self'; frame-ancestors 'none'",
            )
            if state is not None:
                self.send_header(
                    "X-Sniffer-State", json.dumps(state, separators=(",", ":"))
                )
            self.end_headers()
            self.wfile.write(data)

        def valid_host(self):
            return self.headers.get("Host") in (
                f"127.0.0.1:{port}",
                f"localhost:{port}",
            )

        def do_GET(self):
            if not self.valid_host():
                return self.respond(403, b"Local host required")
            if self.path == "/api/frame":
                try:
                    state, frame = simulator.render()
                    return self.respond(200, frame, "application/octet-stream", state)
                except (RuntimeError, BrokenPipeError):
                    return self.respond(503, b"Simulator unavailable")
            files = {
                "/": ("index.html", "text/html; charset=utf-8"),
                "/app.js": ("app.js", "text/javascript; charset=utf-8"),
                "/style.css": ("style.css", "text/css; charset=utf-8"),
            }
            if self.path not in files:
                return self.respond(404, b"Not found")
            filename, kind = files[self.path]
            self.respond(200, (ROOT / "tools/simulator" / filename).read_bytes(), kind)

        def do_POST(self):
            origins = (None, f"http://127.0.0.1:{port}", f"http://localhost:{port}")
            if not self.valid_host() or self.headers.get("Origin") not in origins:
                return self.respond(403, b"Local origin required")
            if (
                self.path != "/api/action"
                or self.headers.get("Content-Type") != "application/json"
            ):
                return self.respond(400, b"Expected a JSON simulator action")
            try:
                size = int(self.headers.get("Content-Length", "0"))
                if not 1 <= size <= 512:
                    raise ValueError("Invalid request size")
                request = json.loads(self.rfile.read(size))
                command = request["action"]
                if command not in (
                    "tap",
                    "rotate",
                    "screen",
                    "inject",
                    "boot",
                    "reset",
                    "onboard",
                ):
                    raise ValueError("Unknown action")
                a, b = request.get("a", 0), request.get("b", 0)
                if (
                    type(a) is not int
                    or type(b) is not int
                    or not (0 <= a < 480 and 0 <= b < 480)
                ):
                    raise ValueError("Invalid coordinates")
                state, frame = simulator.render(command, a, b)
                self.respond(200, frame, "application/octet-stream", state)
            except (ValueError, KeyError, TypeError):
                self.respond(400, b"Invalid simulator action")
            except (RuntimeError, BrokenPipeError):
                self.respond(503, b"Simulator unavailable")

    server = ThreadingHTTPServer(("127.0.0.1", port), Handler)
    print(f"Surveillance Hound simulator: http://127.0.0.1:{port}", flush=True)
    print(
        "Actual firmware UI; synthetic events only; all state is in memory. Ctrl+C stops it.",
        flush=True,
    )
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()
        simulator.close()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--port", type=int, default=8765)
    parser.add_argument(
        "--build-dir",
        type=Path,
        default=Path(tempfile.gettempdir()) / f"surveillance-hound-local-{os.getuid()}",
    )
    args = parser.parse_args()
    if not 1024 <= args.port <= 65535:
        parser.error("Choose a port between 1024 and 65535")
    subprocess.run(
        [
            "cmake",
            "-S",
            str(ROOT),
            "-B",
            str(args.build_dir),
            "-G",
            "Ninja",
            "-DSNIFFER_HOST=ON",
            "-DSNIFFER_SANITIZE=OFF",
        ],
        check=True,
    )
    subprocess.run(
        ["cmake", "--build", str(args.build_dir), "--target", "simulator_worker"],
        check=True,
    )
    serve(args.build_dir / "simulator_worker", args.port)
