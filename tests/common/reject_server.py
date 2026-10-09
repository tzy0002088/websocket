#!/usr/bin/env python3
"""
测试用: 接受 TCP 连接后立即回 400 拒绝 websocket 握手, 然后关闭连接。
用于验证客户端握手失败时的错误处理(onerror 路径)。
纯 python3 标准库, 无第三方依赖。

Usage:
    python3 reject_server.py [port]      # default port: 9102
"""
import socket
import sys


def main():
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 9102
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind(("127.0.0.1", port))
    server.listen(4)
    print("reject server listening on 127.0.0.1:%d" % port, flush=True)

    while True:
        conn, addr = server.accept()
        # 读完请求头后直接拒绝
        request = b""
        while b"\r\n\r\n" not in request:
            chunk = conn.recv(4096)
            if not chunk:
                break
            request += chunk
        conn.sendall(b"HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n")
        conn.close()


if __name__ == "__main__":
    main()
