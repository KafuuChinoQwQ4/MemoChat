import memochat.call.service_algorithms;

namespace memochat::tests::call::service
{
bool TextEquals(const char* lhs, const char* rhs)
{
    return memochat::call::service::modules::TextEquals(lhs, rhs);
}

bool IsEnabledText(const char* value)
{
    return memochat::call::service::modules::IsEnabledText(value);
}

int NormalizeRingTimeoutSec(int value)
{
    return memochat::call::service::modules::NormalizeRingTimeoutSec(value);
}

int NormalizeBusyKeyTtlSec(int value)
{
    return memochat::call::service::modules::NormalizeBusyKeyTtlSec(value);
}

int NormalizeTokenTtlSec(int value)
{
    return memochat::call::service::modules::NormalizeTokenTtlSec(value);
}

bool HasValidStartPeer(int uid, int peer_uid)
{
    return memochat::call::service::modules::HasValidStartPeer(uid, peer_uid);
}

bool IsSupportedCallType(const char* call_type)
{
    return memochat::call::service::modules::IsSupportedCallType(call_type);
}

const char* RingingState()
{
    return memochat::call::service::modules::RingingState();
}

const char* AcceptedState()
{
    return memochat::call::service::modules::AcceptedState();
}

const char* RejectedState()
{
    return memochat::call::service::modules::RejectedState();
}

const char* CancelledState()
{
    return memochat::call::service::modules::CancelledState();
}

const char* TimeoutState()
{
    return memochat::call::service::modules::TimeoutState();
}

const char* EndedState()
{
    return memochat::call::service::modules::EndedState();
}

const char* JoiningState()
{
    return memochat::call::service::modules::JoiningState();
}

const char* InCallState()
{
    return memochat::call::service::modules::InCallState();
}

const char* HangupReason()
{
    return memochat::call::service::modules::HangupReason();
}

bool IsRingingState(const char* state)
{
    return memochat::call::service::modules::IsRingingState(state);
}

bool IsTimeoutState(const char* state)
{
    return memochat::call::service::modules::IsTimeoutState(state);
}

bool IsActiveCallState(const char* state)
{
    return memochat::call::service::modules::IsActiveCallState(state);
}

bool HasCancelTimedOut(long long now_ms, long long expires_at_ms)
{
    return memochat::call::service::modules::HasCancelTimedOut(now_ms, expires_at_ms);
}

const char* CancelTerminalState(bool timed_out)
{
    return memochat::call::service::modules::CancelTerminalState(timed_out);
}

const char* CancelTerminalEvent(bool timed_out)
{
    return memochat::call::service::modules::CancelTerminalEvent(timed_out);
}

int PersistentSessionTtlSec()
{
    return memochat::call::service::modules::PersistentSessionTtlSec();
}

int RingingSessionTtlSec(int ring_timeout_sec)
{
    return memochat::call::service::modules::RingingSessionTtlSec(ring_timeout_sec);
}

int RingingBusyTtlSec(int ring_timeout_sec, int busy_key_ttl_sec)
{
    return memochat::call::service::modules::RingingBusyTtlSec(ring_timeout_sec, busy_key_ttl_sec);
}

long long TokenNotBeforeSec(long long now_sec)
{
    return memochat::call::service::modules::TokenNotBeforeSec(now_sec);
}

long long TokenExpiresAtSec(long long now_sec, int token_ttl_sec)
{
    return memochat::call::service::modules::TokenExpiresAtSec(now_sec, token_ttl_sec);
}

const char* CallerRole()
{
    return memochat::call::service::modules::CallerRole();
}

const char* CalleeRole()
{
    return memochat::call::service::modules::CalleeRole();
}

const char* DefaultParticipantRole(bool is_caller)
{
    return memochat::call::service::modules::DefaultParticipantRole(is_caller);
}

const char* StateSyncEvent()
{
    return memochat::call::service::modules::StateSyncEvent();
}

const char* InviteEvent()
{
    return memochat::call::service::modules::InviteEvent();
}

const char* AcceptedEvent()
{
    return memochat::call::service::modules::AcceptedEvent();
}

const char* RejectedEvent()
{
    return memochat::call::service::modules::RejectedEvent();
}

const char* HangupEvent()
{
    return memochat::call::service::modules::HangupEvent();
}
} // namespace memochat::tests::call::service
