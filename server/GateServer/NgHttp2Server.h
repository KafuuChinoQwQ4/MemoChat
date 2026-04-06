#pragma once

#include <atomic>
#include <thread>
#include <memory>

#ifdef MEMOCHAT_HAVE_NGHTTP2

class NgHttp2Server {
public:
    static NgHttp2Server& GetInstance();
    ~NgHttp2Server();

    NgHttp2Server(const NgHttp2Server&) = delete;
    NgHttp2Server& operator=(const NgHttp2Server&) = delete;

    void SetPort(int port) { _h2_port = port; }
    bool Initialize();
    void Run();    // blocking
    void Stop();

    int h2Port() const { return _h2_port; }

private:
    NgHttp2Server();

    std::atomic<bool> _running{false};
    std::thread _thread;
    int _h2_port = 8080;
};

#else  // stub when nghttp2 is not available

class NgHttp2Server {
public:
    static NgHttp2Server& GetInstance() {
        static NgHttp2Server instance;
        return instance;
    }
    bool Initialize() { return true; }
    void Run() { }  // no-op
    void Stop() { }
    int h2Port() const { return 0; }
private:
    NgHttp2Server() = default;
};

#endif
