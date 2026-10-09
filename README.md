## websocket client

### 1. 如何编译
1. mkdir build
2. cd build && cmake ..
3. cmake --build . --target hello-server

### 2. 运行示例
1. 启动本地 echo server（基于标准 websockets 库）:
```
pip3 install websockets
python3 sample/hello-server/echo_server.py
```
2. 运行客户端。默认连接 ws://127.0.0.1:9010，也可通过命令行参数指定其他服务器:
```
./build/bin/hello-server [ws://xxx.xxx.xxx.xxx:port/path]
```
示例输出（服务器会把收到的每一条消息原样回显）:
```c
tzy@LAPTOP-595ER4EN:~/websocket/build/bin$ ./hello-server 
connect websocket server success!!!
frame type is: text frame, fram data is: hello server.
hello world!!!!
cmdline len = 15
write [hello world!!!!] success!!!!
frame type is: text frame, fram data is: hello world!!!!.
hello websocket!!!!!!
cmdline len = 21
write [hello websocket!!!!!!] success!!!!
frame type is: text frame, fram data is: hello websocket!!!!!!.
exit
cmdline len = 4
```

### 3. 运行 wss 示例（WebSocket over TLS）
1. 启动本地 wss echo server（证书/私钥为本地测试用自签名对，见 sample/hello-server/wss_cert.pem）:
```
python3 sample/hello-server/wss_server.py
```
2. 运行客户端连接 wss://127.0.0.1:9443:
```
./build/bin/hello-server wss://127.0.0.1:9443
```
输出与 ws 示例一致，多了 mbedtls 握手日志和 "Certificate verified success..."。

说明：客户端信任的 CA 证书硬编码在 crypt/mbedtls/ports/inc/tls_certificate.h（CN=127.0.0.1，
有效期至 2036 年）。证书校验失败（如服务器证书不是该 CA 签发）会被拒绝连接。
重新生成证书的方法见该头文件注释。

### 4. 运行测试
测试会自动拉起本地 python echo server（依赖 python3 + websockets: pip3 install websockets）:
```
cmake --build . --target echo_test wss_test
ctest -R "EchoTest|WssTest" --output-on-failure    # ctest 方式(按用例粒度报告)
# 或直接运行 gtest 二进制: ./build/bin/echo_test --gtest_filter=EchoTest.BinaryEcho
```
测试覆盖：
- onopen 回调与文本回显、close 握手
- 连续多轮收发（消息顺序）
- 二进制帧回显（帧类型保持 BIN）
- 长文本（400 字节，16 位扩展长度帧）与 UTF-8 中文回显
- 服务端高频 ping 自动应答（不答 pong 即断连的验证方式）
- 服务端主动断开触发 onclose 回调
- 服务端拒绝握手（HTTP 400）触发 onerror
- wss: TLS 握手 + 证书验证 + 加密回显
- wss: 不受信任的证书被拒绝（onerror 路径）