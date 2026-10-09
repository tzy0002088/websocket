#include "ws_test_client.h"

#include <chrono>

namespace ws_test
{

Client *Client::current_ = nullptr;

Client::Client()
{
    ws_memset(&ws_, 0, sizeof(ws_));
}

Client::~Client()
{
    Disconnect();
}

bool Client::Connect(const char *url, int timeout_ms)
{
    current_ = this;

    app_websocket_worker_init();
    worker_started_ = true;

    if ((app_websocket_init(&ws_) != WEBSOCKET_OK) ||
        (app_websocket_set_url(&ws_, url) != 0))
    {
        return false;
    }
    session_created_ = true;

    app_websocket_message_event(&ws_, OnMessage);
    app_websocket_open_event(&ws_, OnOpen);
    app_websocket_close_event(&ws_, OnClose);
    app_websocket_error_event(&ws_, OnError);

    /* connect_server 只是把会话丢给 worker, 真正的连接在 worker 里异步完成 */
    app_websocket_connect_server(&ws_);

    std::unique_lock<std::mutex> lk(mutex_);
    cv_.wait_for(lk, std::chrono::milliseconds(timeout_ms),
                 [this] { return opened_ || error_; });
    return opened_ && !error_;
}

int Client::Send(const void *data, size_t len, websocket_frame_type_t type)
{
    struct app_websocket_frame frame;
    frame.data = (void *)data;
    frame.length = len;
    frame.type = type;
    return app_websocket_write_data(&ws_, &frame);
}

int Client::WaitMessage(void *buf, size_t buf_len, websocket_frame_type_t *type,
                        int timeout_ms)
{
    std::unique_lock<std::mutex> lk(mutex_);
    if (!cv_.wait_for(lk, std::chrono::milliseconds(timeout_ms),
                      [this] { return !queue_.empty() || closed_ || error_; }))
    {
        return 0; /* 超时 */
    }

    if (queue_.empty())
    {
        return -1; /* 连接已断开 */
    }

    Message msg = queue_.front();
    queue_.pop_front();

    size_t n = msg.data.size() < buf_len ? msg.data.size() : buf_len;
    ws_memcpy(buf, msg.data.data(), n);
    if (type != nullptr)
    {
        *type = msg.type;
    }
    return (int)n;
}

bool Client::IsClosed()
{
    std::lock_guard<std::mutex> lk(mutex_);
    return closed_;
}

bool Client::WaitClosed(int timeout_ms)
{
    std::unique_lock<std::mutex> lk(mutex_);
    return cv_.wait_for(lk, std::chrono::milliseconds(timeout_ms),
                        [this] { return closed_; });
}

void Client::Disconnect()
{
    if (session_created_)
    {
        app_websocket_set_close_reason(&ws_, WEBSOCKET_STATUS_CLOSE_NORMAL, nullptr);
        app_websocket_disconnect_server(&ws_);
        session_created_ = false;
    }

    if (worker_started_)
    {
        /* join worker 线程, 保证 close 帧已发出、会话状态机已走完 */
        app_websocket_worker_deinit();
        worker_started_ = false;
    }

    current_ = nullptr;
}

int Client::OnOpen(struct app_websocket *ws)
{
    (void)ws;
    Client *self = current_;
    if (self == nullptr)
    {
        return WEBSOCKET_OK;
    }

    {
        std::lock_guard<std::mutex> lk(self->mutex_);
        self->opened_ = true;
    }
    self->cv_.notify_all();
    return WEBSOCKET_OK;
}

int Client::OnMessage(struct app_websocket *ws)
{
    Client *self = current_;
    struct app_websocket_frame frame;
    int len = app_websocket_read_data(ws, &frame);
    if (self == nullptr)
    {
        return WEBSOCKET_OK;
    }

    /* 注意: len == 0 可能是控制帧已被库内部处理(此时 frame 未填充,
     * 是野指针), 也可能是空消息/分片中间帧 —— 一律不入队 */
    if (len > 0)
    {
        Message msg;
        /* frame.data 指向库内部缓存, 下次读取会覆盖, 必须立即拷贝 */
        msg.data.assign((const char *)frame.data, frame.length);
        msg.type = frame.type;

        std::lock_guard<std::mutex> lk(self->mutex_);
        self->queue_.push_back(std::move(msg));
    }
    self->cv_.notify_all();
    return len >= 0 ? WEBSOCKET_OK : -WEBSOCKET_ERROR;
}

int Client::OnClose(struct app_websocket *ws)
{
    (void)ws;
    Client *self = current_;
    if (self == nullptr)
    {
        return 0;
    }

    {
        std::lock_guard<std::mutex> lk(self->mutex_);
        self->closed_ = true;
    }
    self->cv_.notify_all();
    return 0;
}

int Client::OnError(struct app_websocket *ws)
{
    (void)ws;
    Client *self = current_;
    if (self == nullptr)
    {
        return 0;
    }

    {
        std::lock_guard<std::mutex> lk(self->mutex_);
        self->error_ = true;
    }
    self->cv_.notify_all();
    return 0;
}

} // namespace ws_test
