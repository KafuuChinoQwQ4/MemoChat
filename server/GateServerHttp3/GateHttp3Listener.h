#pragma once
#ifndef MEMOCHAT_GATE_HTTP3_LISTENER_H
#define MEMOCHAT_GATE_HTTP3_LISTENER_H

#include "GateHttp3Connection.h"
#include "HttpConnection.h"

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

#ifdef MEMOCHAT_ENABLE_HTTP3
#include <msquic.h>
#include <nghttp3/nghttp3.h>
#endif

class LogicSystem;

class GateHttp3Listener : public std::enable_shared_from_this<GateHttp3Listener>
{
public:
    GateHttp3Listener(boost::asio::io_context& ioc, LogicSystem& logic, int port);
    ~GateHttp3Listener();

    bool Start(std::string& error);
    void Stop();

    bool IsRunning() const { return running_; }
    int Port() const { return port_; }

    LogicSystem& GetLogicSystem() { return logic_; }

public:
    struct Impl;

private:
    struct Impl;
    std::unique_ptr<Impl> pImpl_;

    boost::asio::io_context& ioc_;
    LogicSystem& logic_;
    int port_;
    std::atomic<bool> running_{false};
};

#endif  // MEMOCHAT_GATE_HTTP3_LISTENER_H
