#pragma once

#include "random/Uuid.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <string>

namespace memolog
{

struct TraceSnapshot
{
    std::string trace_id;
    std::string request_id;
    std::string span_id;
    std::string uid;
    std::string session_id;
};

class TraceContext
{
public:
    static void SetTraceId(const std::string& trace_id)
    {
        data().trace_id = trace_id;
    }

    static void SetRequestId(const std::string& request_id)
    {
        data().request_id = request_id;
    }

    static void SetSpanId(const std::string& span_id)
    {
        data().span_id = span_id;
    }

    static void SetUid(const std::string& uid)
    {
        data().uid = uid;
    }

    static void SetSessionId(const std::string& session_id)
    {
        data().session_id = session_id;
    }

    static const std::string& GetTraceId()
    {
        return data().trace_id;
    }

    static const std::string& GetRequestId()
    {
        return data().request_id;
    }

    static const std::string& GetSpanId()
    {
        return data().span_id;
    }

    static const std::string& GetUid()
    {
        return data().uid;
    }

    static const std::string& GetSessionId()
    {
        return data().session_id;
    }

    static std::string NewId()
    {
        std::string value;
        if (memochat::random::GenerateUuid(value))
        {
            value.erase(std::remove(value.begin(), value.end(), '-'), value.end());
            return value;
        }

        static std::atomic<std::uint64_t> sequence{0};
        const auto now = static_cast<unsigned long long>(
            std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now().time_since_epoch())
                .count());
        const auto next = static_cast<unsigned long long>(sequence.fetch_add(1, std::memory_order_relaxed));
        char fallback[33]{};
        std::snprintf(fallback, sizeof(fallback), "%016llx%016llx", now, next);
        return fallback;
    }

    static std::string NewSpanId()
    {
        const std::string id = NewId();
        return id.size() >= 16 ? id.substr(0, 16) : id;
    }

    static std::string EnsureTraceId()
    {
        if (data().trace_id.empty())
        {
            data().trace_id = NewId();
        }
        return data().trace_id;
    }

    static TraceSnapshot Capture()
    {
        return data();
    }

    static void Restore(const TraceSnapshot& snapshot)
    {
        data() = snapshot;
    }

    static void Clear()
    {
        data() = TraceSnapshot{};
    }

private:
    static TraceSnapshot& data()
    {
        static thread_local TraceSnapshot ctx;
        return ctx;
    }
};

class TraceScope
{
public:
    explicit TraceScope(const std::string& trace_id)
        : old_(TraceContext::Capture())
    {
        TraceContext::SetTraceId(trace_id.empty() ? TraceContext::NewId() : trace_id);
        TraceContext::SetRequestId(TraceContext::NewId());
        TraceContext::SetSpanId("");
    }

    ~TraceScope()
    {
        TraceContext::Restore(old_);
    }

private:
    TraceSnapshot old_;
};

} // namespace memolog
