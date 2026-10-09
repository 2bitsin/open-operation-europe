#!/usr/bin/env python3
"""Serve the web build (port/build/web) for local testing.

    python3 port/tools/serve_web.py [--port 8765] [--dir port/build/web]
    open http://localhost:8765/opeu.html

Same as `python3 -m http.server`, plus the MIME types the build needs on
every Python version (.wasm as application/wasm, which Python < 3.10's
table lacks; .data as application/octet-stream) and `Cache-Control:
no-store`, so a rebuilt opeu.wasm/opeu.data is never mixed with a cached
opeu.js. The build uses neither threads nor SharedArrayBuffer, so no
cross-origin isolation (COOP/COEP) headers are needed.
"""
import argparse
import functools
import http.server
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


class Handler(http.server.SimpleHTTPRequestHandler):
    extensions_map = {
        **http.server.SimpleHTTPRequestHandler.extensions_map,
        '.wasm': 'application/wasm',
        '.data': 'application/octet-stream',
        '.js': 'text/javascript',
        '.html': 'text/html',
    }

    def end_headers(self):
        self.send_header('Cache-Control', 'no-store')
        super().end_headers()


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument('--port', type=int, default=8765)
    ap.add_argument('--bind', default='127.0.0.1')
    ap.add_argument('--dir', default=str(ROOT/'port/build/web'))
    args = ap.parse_args()
    handler = functools.partial(Handler, directory=args.dir)
    with http.server.ThreadingHTTPServer((args.bind, args.port), handler) as httpd:
        print(f'serving {args.dir} at http://localhost:{args.port}/opeu.html (Ctrl-C stops)')
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            pass


if __name__ == '__main__':
    main()
