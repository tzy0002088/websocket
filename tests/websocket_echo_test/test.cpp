#include <gtest/gtest.h>
#include <unistd.h>

#include <chrono>
#include <cstdio>
#include <cstring>
#include <thread>

#include "ws_test_client.h"
#include "ws_test_server.h"

#define WS_SERVER_BASE_PORT (9101)

namespace
{

static int ServerPort()
{
    return ws_test::ServerPort(WS_SERVER_BASE_PORT);
}

static int RejectServerPort()
{
    /* 远离各测试端口段, 避免并行冲突 */
    return 9901 + getpid() % 100;
}

static int PingServerPort()
{
    return 9501 + getpid() % 100;
}

static std::string ServerLog()
{
    return "/tmp/ws_echo_test_" + std::to_string(getpid()) + ".log";
}

static std::string RejectServerLog()
{
    return "/tmp/ws_reject_test_" + std::to_string(getpid()) + ".log";
}

static std::string PingServerLog()
{
    return "/tmp/ws_ping_test_" + std::to_string(getpid()) + ".log";
}

/* ws 功能测试: 覆盖 sample 使用的 service 层 API ——
 * onopen/onmessage/onclose/onerror 回调, 文本/二进制收发, close 握手 */
class EchoTest : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        ASSERT_TRUE(server.Start(
            std::string(WS_SOURCE_DIR) + "/sample/hello-server/echo_server.py",
            std::to_string(ServerPort()), ServerLog()))
            << "启动 echo_server.py 失败, 请确认已安装 websockets(pip3 install websockets)";

        /* 握手拒绝服务端(回 400), 用于测试连接失败的 onerror 路径 */
        ASSERT_TRUE(reject_server.Start(
            std::string(WS_SOURCE_DIR) + "/tests/common/reject_server.py",
            std::to_string(RejectServerPort()), RejectServerLog()));

        /* 高频 ping 服务端: 1 秒一个 ping, 3 秒收不到 pong 就断开,
         * 用于测试客户端自动应答 ping */
        ASSERT_TRUE(ping_server.Start(
            std::string(WS_SOURCE_DIR) + "/sample/hello-server/echo_server.py",
            std::to_string(PingServerPort()) + " 1 3", PingServerLog()));
    }

    static void TearDownTestSuite()
    {
        server.Stop();
        reject_server.Stop();
        ping_server.Stop();
    }

    static ws_test::TestServer server;
    static ws_test::TestServer reject_server;
    static ws_test::TestServer ping_server;
};

ws_test::TestServer EchoTest::server;
ws_test::TestServer EchoTest::reject_server;
ws_test::TestServer EchoTest::ping_server;

/* 连接回调(onopen) + 文本回显 + close 握手 */
TEST_F(EchoTest, ConnectAndTextEcho)
{
    ws_test::Client client;
    std::string url = "ws://127.0.0.1:" + std::to_string(ServerPort());

    ASSERT_TRUE(client.Connect(url.c_str())) << "onopen 未触发, 连接失败";
    ASSERT_TRUE(client.IsOpen());

    const char *msg = "hello world!!!!";
    ASSERT_GT(client.Send(msg, strlen(msg)), 0) << "发送失败";

    char buf[256] = {0};
    websocket_frame_type_t type = WEBSOCKET_CONTINUE_FRAME;
    int len = client.WaitMessage(buf, sizeof(buf), &type);
    ASSERT_GT(len, 0) << "未收到回显";
    EXPECT_EQ(type, WEBSOCKET_TEXT_FRAME);
    EXPECT_STREQ(buf, msg);

    client.Disconnect();
    /* close 握手: 服务端应正常记录客户端断开 */
    ASSERT_TRUE(ws_test::TestServer::WaitLog(ServerLog(), "client disconnected"))
        << "close 握手未完成";
}

/* 连续多轮收发, 验证消息顺序 */
TEST_F(EchoTest, MultipleRoundTrips)
{
    ws_test::Client client;
    std::string url = "ws://127.0.0.1:" + std::to_string(ServerPort());
    ASSERT_TRUE(client.Connect(url.c_str()));

    for (int i = 0; i < 3; i++)
    {
        char msg[64];
        snprintf(msg, sizeof(msg), "message %d", i);
        ASSERT_GT(client.Send(msg, strlen(msg)), 0);
    }

    for (int i = 0; i < 3; i++)
    {
        char expect[64];
        char buf[64] = {0};
        snprintf(expect, sizeof(expect), "message %d", i);
        ASSERT_GT(client.WaitMessage(buf, sizeof(buf), nullptr), 0);
        EXPECT_STREQ(buf, expect);
    }

    client.Disconnect();
}

/* 二进制帧回显, 帧类型保持 BIN */
TEST_F(EchoTest, BinaryEcho)
{
    ws_test::Client client;
    std::string url = "ws://127.0.0.1:" + std::to_string(ServerPort());
    ASSERT_TRUE(client.Connect(url.c_str()));

    const unsigned char bin[] = {0x00, 0x01, 0xFE, 0xFF, 0x7F, 0x80};
    ASSERT_GT(client.Send(bin, sizeof(bin), WEBSOCKET_BIN_FRAME), 0);

    unsigned char buf[64] = {0};
    websocket_frame_type_t type = WEBSOCKET_CONTINUE_FRAME;
    int len = client.WaitMessage(buf, sizeof(buf), &type);
    ASSERT_EQ(len, (int)sizeof(bin));
    EXPECT_EQ(type, WEBSOCKET_BIN_FRAME);
    EXPECT_EQ(memcmp(buf, bin, sizeof(bin)), 0);

    client.Disconnect();
}

/* 服务端主动断开时触发 onclose 回调 */
TEST_F(EchoTest, ServerInitiatedCloseFiresOnCloseCallback)
{
    ws_test::Client client;
    std::string url = "ws://127.0.0.1:" + std::to_string(ServerPort());
    ASSERT_TRUE(client.Connect(url.c_str()));

    /* 服务端收到该消息后会主动 close */
    ASSERT_GT(client.Send("__server_close__", strlen("__server_close__")), 0);
    ASSERT_TRUE(client.WaitClosed()) << "onclose 回调未触发";

    /* 断开后等待消息应返回 -1(而不是超时) */
    char buf[64] = {0};
    EXPECT_EQ(client.WaitMessage(buf, sizeof(buf), nullptr, 1000), -1)
        << "服务端断开后 WaitMessage 应返回 -1";

    client.Disconnect();
}

/* 400 字节长文本回显(触发 16 位扩展长度帧, 收发双向) */
TEST_F(EchoTest, LongTextEcho)
{
    ws_test::Client client;
    std::string url = "ws://127.0.0.1:" + std::to_string(ServerPort());
    ASSERT_TRUE(client.Connect(url.c_str()));

    /* 400 > 125, 帧长度字段走 16 位扩展长度分支 */
    std::string msg(400, 'A');
    for (int i = 0; i < 400; i += 100)
    {
        msg[i] = '0' + i / 100; /* 加几个标记位, 便于发现错位 */
    }
    ASSERT_GT(client.Send(msg.data(), msg.size()), 0);

    std::string buf;
    buf.resize(512);
    websocket_frame_type_t type = WEBSOCKET_CONTINUE_FRAME;
    int len = client.WaitMessage(&buf[0], buf.size(), &type);
    ASSERT_EQ(len, (int)msg.size());
    EXPECT_EQ(type, WEBSOCKET_TEXT_FRAME);
    EXPECT_EQ(buf.compare(0, msg.size(), msg), 0);

    client.Disconnect();
}

/* UTF-8 多字节文本回显 */
TEST_F(EchoTest, Utf8TextEcho)
{
    ws_test::Client client;
    std::string url = "ws://127.0.0.1:" + std::to_string(ServerPort());
    ASSERT_TRUE(client.Connect(url.c_str()));

    const std::string msg = u8"你好 websocket 🚀 中文测试";
    ASSERT_GT(client.Send(msg.data(), msg.size()), 0);

    std::string buf;
    buf.resize(128);
    websocket_frame_type_t type = WEBSOCKET_CONTINUE_FRAME;
    int len = client.WaitMessage(&buf[0], buf.size(), &type);
    ASSERT_EQ(len, (int)msg.size());
    EXPECT_EQ(type, WEBSOCKET_TEXT_FRAME);
    EXPECT_EQ(buf.compare(0, msg.size(), msg), 0);

    client.Disconnect();
}

/* 服务端每 1 秒 ping 一次(3 秒收不到 pong 就断连),
 * 连接持续存活说明客户端自动应答了 ping */
TEST_F(EchoTest, ServerPingAnsweredAutomatically)
{
    ws_test::Client client;
    std::string url = "ws://127.0.0.1:" + std::to_string(PingServerPort());
    ASSERT_TRUE(client.Connect(url.c_str()));

    /* 若客户端不自动回 pong, 服务端会在几秒内断开连接(onclose 触发) */
    std::this_thread::sleep_for(std::chrono::seconds(5));
    EXPECT_FALSE(client.IsClosed()) << "连接被服务端关闭, 客户端可能未自动应答 ping";

    client.Disconnect();
}

/* 服务端拒绝握手(HTTP 400)时触发 onerror, 未连接写数据返回错误 */
TEST_F(EchoTest, ConnectToRejectingServerFiresErrorCallback)
{
    ws_test::Client client;
    std::string url = "ws://127.0.0.1:" + std::to_string(RejectServerPort());

    EXPECT_FALSE(client.Connect(url.c_str())) << "握手被拒应触发 onerror";
    EXPECT_FALSE(client.IsOpen());

    const char *msg = "should fail";
    EXPECT_LT(client.Send(msg, strlen(msg)), 0) << "未连接时写数据应返回错误";

    client.Disconnect();
}

} // namespace
