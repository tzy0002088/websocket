#!/usr/bin/env python3
"""
WSS (WebSocket over TLS) echo server, based on the standard `websockets` library.

Usage:
    pip3 install websockets            # only dependency
    python3 wss_server.py [port]       # default port: 9443

It pairs with the hello-server sample:
    ./build/bin/hello-server wss://127.0.0.1:9443

The certificate (wss_cert.pem) and key (wss_key.pem) are a self-signed
pair for 127.0.0.1, generated with openssl, for local testing ONLY.
The matching CA is embedded in the C client (tls_certificate.h).
"""
import asyncio
import os
import ssl
import sys

try:
    import websockets
except ImportError:
    sys.exit("websockets library not installed, run: pip3 install websockets")

# 证书路径相对脚本所在目录解析, 与当前工作目录无关
BASE_DIR = os.path.dirname(os.path.abspath(__file__))


async def echo(ws):
    print("wss client connected:", ws.remote_address, "| path:", ws.request.path,
          flush=True)
    async for msg in ws:
        print("echo[%d bytes]: %s" % (len(msg), str(msg)[:64]), flush=True)
        await ws.send(msg)
    print("wss client disconnected", flush=True)


async def main():
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 9443
    cert = (os.path.join(BASE_DIR, sys.argv[2]) if len(sys.argv) > 2
            else os.path.join(BASE_DIR, "wss_cert.pem"))
    key = (os.path.join(BASE_DIR, sys.argv[3]) if len(sys.argv) > 3
           else os.path.join(BASE_DIR, "wss_key.pem"))
    ctx = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
    ctx.load_cert_chain(cert, key)
    async with websockets.serve(echo, "127.0.0.1", port, ssl=ctx):
        print("wss echo server listening on wss://127.0.0.1:%d" % port,
              flush=True)
        await asyncio.Future()  # run forever


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        pass
