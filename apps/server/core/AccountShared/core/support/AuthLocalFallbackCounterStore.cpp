#include "AuthLocalFallbackCounterStore.hpp"

#include <algorithm>
#include <utility>

namespace gateauthsupport
{

AuthLocalFallbackCounterStore::AuthLocalFallbackCounterStore(std::size_t capacity)
    : _capacity(capacity)
{
}

bool AuthLocalFallbackCounterStore::ExpiryEntryLater::operator()(const ExpiryEntry& left,
                                                                 const ExpiryEntry& right) const
{
    return left.expires_at > right.expires_at;
}

void AuthLocalFallbackCounterStore::PruneExpired(Clock::time_point now)
{
    while (!_expirations.empty() && _expirations.top().expires_at <= now)
    {
        const auto expired = _expirations.top();
        _expirations.pop();
        const auto current = _counters.find(expired.key);
        if (current != _counters.end() && current->second.generation == expired.generation &&
            current->second.expires_at <= now)
        {
            _counters.erase(current);
        }
    }
}

AuthLocalFallbackCounterResult
AuthLocalFallbackCounterStore::Increment(std::string_view key, int window_sec, Clock::time_point now)
{
    std::lock_guard<std::mutex> lock(_mutex);
    PruneExpired(now);

    const std::string owned_key(key);
    if (auto current = _counters.find(owned_key); current != _counters.end())
    {
        ++current->second.count;
        return {.status = AuthLocalFallbackCounterStatus::Incremented,
                .count = current->second.count,
                .ttl_sec = std::max(
                    1,
                    static_cast<int>(
                        std::chrono::duration_cast<std::chrono::seconds>(current->second.expires_at - now).count()))};
    }

    if (_counters.size() >= _capacity)
    {
        return {.status = AuthLocalFallbackCounterStatus::CapacityExhausted};
    }

    const auto ttl = std::chrono::seconds(std::max(1, window_sec));
    CounterState state;
    state.count = 1;
    state.expires_at = now + ttl;
    state.generation = _next_generation++;
    const auto generation = state.generation;
    const auto expires_at = state.expires_at;
    _counters.emplace(owned_key, std::move(state));
    _expirations.push({.expires_at = expires_at, .key = owned_key, .generation = generation});
    return {.status = AuthLocalFallbackCounterStatus::Incremented,
            .count = 1,
            .ttl_sec = static_cast<int>(ttl.count())};
}

} // namespace gateauthsupport
