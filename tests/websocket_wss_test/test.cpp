#include <gtest/gtest.h>
#include <unistd.h>

#include <cstring>

#include "ws_test_client.h"
#include "ws_test_server.h"

#define WSS_SERVER_BASE_PORT (9143)

namespace
{

static int ServerPort()
{
    return ws_test::ServerPort(WSS_SERVER_BASE_PORT);
}

static int EvilServerPort()
{
    return ws_test::ServerPort(WSS_SERVER_BASE_PORT + 50);
}

static std::string ServerLog()
{
    return "/tmp/wss_echo_test_" + std::to_string(getpid()) + ".log";
}

static std::string EvilServerLog()
{
    return "/tmp/wss_evil_test_" + std::to_string(getpid()) + ".log";
}

/* wss 功能测试: TLS 握手 + 证书校验 + 加密收发; 以及拒绝不受信任证书 */
class WssTest : public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        ASSERT_TRUE(server.Start(
            std::string(WS_SOURCE_DIR) + "/sample/hello-server/wss_server.py",
            std::to_string(ServerPort()), ServerLog()))
            << "启动 wss_server.py 失败, 请确认已安装 websockets(pip3 install websockets)";

        /* 使用不受客户端信任的证书(CN=evil.example.com)启动第二个服务端 */
        std::string evil_args = std::to_string(EvilServerPort()) + " " +
                                std::string(WS_SOURCE_DIR) + "/tests/common/evil_cert.pem " +
                                std::string(WS_SOURCE_DIR) + "/tests/common/evil_key.pem";
        ASSERT_TRUE(evil_server.Start(
            std::string(WS_SOURCE_DIR) + "/sample/hello-server/wss_server.py",
            evil_args, EvilServerLog()));
    }

    static void TearDownTestSuite()
    {
        server.Stop();
        evil_server.Stop();
    }

    static ws_test::TestServer server;
    static ws_test::TestServer evil_server;
};

ws_test::TestServer WssTest::server;
ws_test::TestServer WssTest::evil_server;

/* TLS 连接(证书验证通过) + 加密回显 + close 握手 */
TEST_F(WssTest, TlsConnectAndEcho)
{
    ws_test::Client client;
    std::string url = "wss://127.0.0.1:" + std::to_string(ServerPort());

    ASSERT_TRUE(client.Connect(url.c_str(), 10000)) << "TLS 握手或证书验证失败";
    ASSERT_TRUE(client.IsOpen());

    const char *msg = "hello wss server";
    ASSERT_GT(client.Send(msg, strlen(msg)), 0) << "发送失败";

    char buf[256] = {0};
    websocket_frame_type_t type = WEBSOCKET_CONTINUE_FRAME;
    int len = client.WaitMessage(buf, sizeof(buf), &type);
    ASSERT_GT(len, 0) << "未收到回显";
    EXPECT_EQ(type, WEBSOCKET_TEXT_FRAME);
    EXPECT_STREQ(buf, msg);

    client.Disconnect();
    ASSERT_TRUE(ws_test::TestServer::WaitLog(ServerLog(), "wss client disconnected"))
        << "close 握手未完成";
}

/* 不受信任的证书应被拒绝(onerror 触发), 且失败路径不崩溃 */
TEST_F(WssTest, RejectUntrustedCertificate)
{
    ws_test::Client client;
    std::string url = "wss://127.0.0.1:" + std::to_string(EvilServerPort());

    EXPECT_FALSE(client.Connect(url.c_str(), 10000)) << "不受信任的证书应被拒绝";
    EXPECT_FALSE(client.IsOpen());

    /* 未连接状态下写数据应返回错误 */
    const char *msg = "should fail";
    EXPECT_LT(client.Send(msg, strlen(msg)), 0);

    client.Disconnect();
}

} // namespace
