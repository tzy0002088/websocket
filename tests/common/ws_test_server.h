#ifndef __WS_TEST_SERVER_H__
#define __WS_TEST_SERVER_H__

#include <string>
#include <unistd.h>

namespace ws_test
{

/* 按 pid 分配测试端口, 避免 ctest -j 并行时端口冲突 */
static inline int ServerPort(int base)
{
    return base + (getpid() % 100);
}

/* 在测试进程里拉起仓库自带的 python echo server(echo_server.py/wss_server.py),
 * 通过日志文件探测就绪, Stop/析构时结束进程 */
class TestServer
{
public:
    /* script: 脚本绝对路径; args: 命令行参数(空格分隔, 不含脚本名);
     * log: 输出日志路径; ready: 就绪标记(默认等服务端打印 "listening") */
    bool Start(const std::string &script, const std::string &args,
               const std::string &log, const std::string &ready = "listening");

    void Stop();

    /* 轮询日志文件, 直到出现 needle 或超时 */
    static bool WaitLog(const std::string &log, const std::string &needle,
                        int timeout_ms = 5000);

    ~TestServer();

private:
    int pid_ = -1;
};

} // namespace ws_test

#endif // __WS_TEST_SERVER_H__
