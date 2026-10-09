#ifndef __WS_TEST_CLIENT_H__
#define __WS_TEST_CLIENT_H__

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <mutex>
#include <string>
#include "websocket_service.h"

namespace ws_test
{

struct Message
{
    std::string data;
    websocket_frame_type_t type;
};

/* 事件驱动的测试客户端封装。
 * 库的回调(onopen/onmessage/onclose/onerror)运行在 worker 线程,
 * 本类通过条件变量把事件同步给测试线程, 收到的消息进入队列。
 * 注意: 库的 worker 是全局单例, 测试里同一时刻只允许存在一个 Client 实例。 */
class Client
{
public:
    Client();
    ~Client();

    /* 连接并等待 onopen 触发; onerror 先触发(连接失败)或超时返回 false */
    bool Connect(const char *url, int timeout_ms = 5000);

    /* 发送数据, 成功返回发送字节数, <0 表示失败(如未连接) */
    int Send(const void *data, size_t len,
             websocket_frame_type_t type = WEBSOCKET_TEXT_FRAME);

    /* 等待一条消息: 成功返回拷贝到 buf 的字节数,
     * 超时返回 0, 连接已断开返回 -1 */
    int WaitMessage(void *buf, size_t buf_len, websocket_frame_type_t *type,
                    int timeout_ms = 5000);

    /* 等待 onclose 回调触发(服务端主动断开场景) */
    bool WaitClosed(int timeout_ms = 5000);

    /* 客户端主动断开(close 握手), join worker 保证状态机走完 */
    void Disconnect();

    bool IsOpen() const { return opened_; }

    /* 服务端是否已断开(onclose 回调已触发) */
    bool IsClosed();

private:
    static int OnOpen(struct app_websocket *ws);
    static int OnMessage(struct app_websocket *ws);
    static int OnClose(struct app_websocket *ws);
    static int OnError(struct app_websocket *ws);

    static Client *current_;

    struct app_websocket ws_;
    bool worker_started_ = false;
    bool session_created_ = false;
    std::mutex mutex_;
    std::condition_variable cv_;
    bool opened_ = false;
    bool closed_ = false;
    bool error_ = false;
    std::deque<Message> queue_;
};

} // namespace ws_test

#endif // __WS_TEST_CLIENT_H__
