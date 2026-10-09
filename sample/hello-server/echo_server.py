#!/usr/bin/env python3
"""
RFC 6455 WebSocket echo server, based on the standard `websockets` library.

Usage:
    pip3 install websockets            # only dependency
    python3 echo_server.py [port]      # default port: 9010

It pairs with the hello-server sample:
    ./build/bin/hello-server ws://127.0.0.1:9010

Behaviour: echoes every message back. The handshake, masking,
ping/pong and the close handshake are handled by the websockets library.
"""
import asyncio
import sys

try:
    import websockets
except ImportError:
    sys.exit("websockets library not installed, run: pip3 install websockets")


async def echo(ws):
    print("client connected:", ws.remote_address, "| path:", ws.request.path,
          flush=True)
    async for msg in ws:
        print("echo[%d bytes]: %s" % (len(msg), str(msg)[:64]), flush=True)
        # 测试用: 收到该消息时服务端主动断开, 用于验证客户端 onclose 回调
        if msg == "__server_close__":
            await ws.close(code=1000, reason="server close for test")
            print("server close sent", flush=True)
            return
        await ws.send(msg)
    print("client disconnected", flush=True)


async def main():
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 9010
    # 可选参数(测试用): ping 间隔/超时, 默认 20s(websockets 库默认值)
    ping_interval = float(sys.argv[2]) if len(sys.argv) > 2 else 20
    ping_timeout = float(sys.argv[3]) if len(sys.argv) > 3 else 20
    async with websockets.serve(echo, "0.0.0.0", port,
                                ping_interval=ping_interval,
                                ping_timeout=ping_timeout):
        print("websocket echo server listening on 0.0.0.0:%d" % port,
              flush=True)
        await asyncio.Future()  # run forever


if __name__ == "__main__":
    try:
        asyncio.run(main())
    except KeyboardInterrupt:
        pass
