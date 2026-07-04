export module memochat.call.session_math_algorithms;

export namespace memochat::call::session_math::modules
{
// TCP message id used when notifying peers about a call event.
short CallEventNotifyMsgId()
{
    return 1085;
}

// Number of leading call-id characters used to build the room short suffix.
unsigned long long RoomShortIdLength()
{
    return 8;
}

// Absolute ring-timeout expiry timestamp in milliseconds. The int64 promotion of
// ring_timeout_sec before the multiply preserves the original overflow-avoidance cast.
long long SessionExpiryMs(long long started_at_ms, int ring_timeout_sec)
{
    return started_at_ms + static_cast<long long>(ring_timeout_sec) * 1000;
}

// Whether a hangup should record an elapsed duration: the call was accepted and
// ended strictly after it was accepted.
bool ShouldComputeDurationSec(long long accepted_at_ms, long long ended_at_ms)
{
    return accepted_at_ms > 0 && ended_at_ms > accepted_at_ms;
}

// Elapsed call duration in whole seconds (truncating), from accepted to ended.
int SessionDurationSec(long long accepted_at_ms, long long ended_at_ms)
{
    return static_cast<int>((ended_at_ms - accepted_at_ms) / 1000);
}
} // namespace memochat::call::session_math::modules
