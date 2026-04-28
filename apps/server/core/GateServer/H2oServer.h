#pragma once
#include <atomic>
#include <thread>
#include <memory>

#ifdef MEMOCHAT_HAVE_H2O

class H2oServer {
public:
    static H2oServer& GetInstance();
    ~H2oServer();

    H2oServer(const H2oServer&) = delete;
    H2oServer& operator=(const H2oServer&) = delete;

    void SetPort(int port) { _h2_port = port; }
    bool Initialize();
    void Run();    // blocking
    void Stop();

    int h2Port() const { return _h2_port; }

private:
    H2oServer();

    std::atomic<bool> _running{false};
    std::thread _thread;
    int _h2_port = 8080;
};

#else  // stub when h2o is not available

class H2oServer {
public:
    static H2oServer& GetInstance() {
        static H2oServer instance;
        return instance;
    }
    bool Initialize() { return true; }
    void Run() { }  // no-op
    void Stop() { }
    int h2Port() const { return 0; }
private:
    H2oServer() = default;
};

#endif

