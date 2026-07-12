#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <queue>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace gateauthsupport
{

enum class AuthLocalFallbackCounterStatus
{
    Incremented,
    CapacityExhausted,
};

struct AuthLocalFallbackCounterResult
{
    AuthLocalFallbackCounterStatus status = AuthLocalFallbackCounterStatus::CapacityExhausted;
    long long count = 0;
    int ttl_sec = 0;
};

class AuthLocalFallbackCounterStore
{
public:
    using Clock = std::chrono::steady_clock;

    explicit AuthLocalFallbackCounterStore(std::size_t capacity);

    AuthLocalFallbackCounterResult Increment(std::string_view key, int window_sec, Clock::time_point now);

private:
    struct CounterState
    {
        long long count = 0;
        Clock::time_point expires_at{};
        std::uint64_t generation = 0;
    };

    struct ExpiryEntry
    {
        Clock::time_point expires_at{};
        std::string key;
        std::uint64_t generation = 0;
    };

    struct ExpiryEntryLater
    {
        bool operator()(const ExpiryEntry& left, const ExpiryEntry& right) const;
    };

    void PruneExpired(Clock::time_point now);

    const std::size_t _capacity;
    std::uint64_t _next_generation = 1;
    std::unordered_map<std::string, CounterState> _counters;
    std::priority_queue<ExpiryEntry, std::vector<ExpiryEntry>, ExpiryEntryLater> _expirations;
    std::mutex _mutex;
};

} // namespace gateauthsupport
