import memochat.call.session_math_algorithms;

namespace memochat::tests::call::session_math
{
short CallEventNotifyMsgId()
{
    return memochat::call::session_math::modules::CallEventNotifyMsgId();
}

unsigned long long RoomShortIdLength()
{
    return memochat::call::session_math::modules::RoomShortIdLength();
}

long long SessionExpiryMs(long long started_at_ms, int ring_timeout_sec)
{
    return memochat::call::session_math::modules::SessionExpiryMs(started_at_ms, ring_timeout_sec);
}

bool ShouldComputeDurationSec(long long accepted_at_ms, long long ended_at_ms)
{
    return memochat::call::session_math::modules::ShouldComputeDurationSec(accepted_at_ms, ended_at_ms);
}

int SessionDurationSec(long long accepted_at_ms, long long ended_at_ms)
{
    return memochat::call::session_math::modules::SessionDurationSec(accepted_at_ms, ended_at_ms);
}
} // namespace memochat::tests::call::session_math
