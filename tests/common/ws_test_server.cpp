#include "ws_test_server.h"

#include <fcntl.h>
#include <signal.h>
#include <sys/prctl.h>
#include <sys/wait.h>

#include <chrono>
#include <fstream>
#include <sstream>
#include <thread>
#include <vector>

namespace ws_test
{

bool TestServer::Start(const std::string &script, const std::string &args,
                       const std::string &log, const std::string &ready)
{
    pid_t pid = fork();
    if (pid < 0)
    {
        return false;
    }

    if (pid == 0)
    {
        /* 测试进程无论正常退出还是崩溃(如 abort), 内核都会终结 server,
         * 避免遗留孤儿进程 */
        prctl(PR_SET_PDEATHSIG, SIGTERM);

        /* 子进程: stdout/stderr 重定向到日志文件, 拉起 python server */
        int fd = open(log.c_str(), O_CREAT | O_TRUNC | O_WRONLY, 0644);
        if (fd >= 0)
        {
            dup2(fd, STDOUT_FILENO);
            dup2(fd, STDERR_FILENO);
            close(fd);
        }

        std::vector<std::string> tokens;
        std::istringstream iss(args);
        std::string token;
        while (iss >> token)
        {
            tokens.push_back(token);
        }

        std::vector<char *> argv;
        argv.push_back((char *)"python3");
        argv.push_back((char *)"-u");
        argv.push_back((char *)script.c_str());
        for (std::string &t : tokens)
        {
            argv.push_back((char *)t.c_str());
        }
        argv.push_back(nullptr);

        execvp("python3", argv.data());
        _exit(127);
    }

    pid_ = (int)pid;
    return WaitLog(log, ready, 5000);
}

void TestServer::Stop()
{
    if (pid_ < 0)
    {
        return;
    }

    kill(pid_, SIGTERM);
    waitpid(pid_, nullptr, 0);
    pid_ = -1;
}

TestServer::~TestServer()
{
    Stop();
}

bool TestServer::WaitLog(const std::string &log, const std::string &needle,
                         int timeout_ms)
{
    auto deadline = std::chrono::steady_clock::now() +
                    std::chrono::milliseconds(timeout_ms);
    while (std::chrono::steady_clock::now() < deadline)
    {
        std::ifstream f(log);
        if (f.good())
        {
            std::string content((std::istreambuf_iterator<char>(f)),
                                std::istreambuf_iterator<char>());
            if (content.find(needle) != std::string::npos)
            {
                return true;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return false;
}

} // namespace ws_test
