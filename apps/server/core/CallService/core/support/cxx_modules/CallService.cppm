export module memochat.call.service_algorithms;

export namespace memochat::call::service::modules
{
bool TextEquals(const char* lhs, const char* rhs)
{
    if (lhs == nullptr || rhs == nullptr)
    {
        return lhs == rhs;
    }
    while (*lhs != '\0' && *rhs != '\0')
    {
        if (*lhs != *rhs)
        {
            return false;
        }
        ++lhs;
        ++rhs;
    }
    return *lhs == '\0' && *rhs == '\0';
}

bool IsEnabledText(const char* value)
{
    return TextEquals(value, "1") || TextEquals(value, "true") || TextEquals(value, "TRUE") ||
           TextEquals(value, "True");
}

int NormalizeRingTimeoutSec(int value)
{
    return value < 15 ? 15 : value;
}

int NormalizeBusyKeyTtlSec(int value)
{
    return value < 15 ? 15 : value;
}

int NormalizeTokenTtlSec(int value)
{
    return value < 300 ? 300 : value;
}

bool HasValidAuthRequest(int uid, bool token_empty)
{
    return uid > 0 && !token_empty;
}

bool HasValidStartPeer(int uid, int peer_uid)
{
    return peer_uid > 0 && uid != peer_uid;
}

bool IsSupportedCallType(const char* call_type)
{
    return TextEquals(call_type, "voice") || TextEquals(call_type, "video");
}

const char* RingingState()
{
    return "ringing";
}

const char* AcceptedState()
{
    return "accepted";
}

const char* RejectedState()
{
    return "rejected";
}

const char* CancelledState()
{
    return "cancelled";
}

const char* TimeoutState()
{
    return "timeout";
}

const char* EndedState()
{
    return "ended";
}

const char* JoiningState()
{
    return "joining";
}

const char* InCallState()
{
    return "in_call";
}

const char* HangupReason()
{
    return "hangup";
}

bool IsRingingState(const char* state)
{
    return TextEquals(state, RingingState());
}

bool IsTimeoutState(const char* state)
{
    return TextEquals(state, TimeoutState());
}

bool IsActiveCallState(const char* state)
{
    return TextEquals(state, AcceptedState()) || TextEquals(state, JoiningState()) || TextEquals(state, InCallState());
}

bool HasCancelTimedOut(long long now_ms, long long expires_at_ms)
{
    return now_ms > expires_at_ms;
}

const char* CancelTerminalState(bool timed_out)
{
    return timed_out ? TimeoutState() : CancelledState();
}

const char* CancelTerminalEvent(bool timed_out)
{
    return timed_out ? "call.timeout" : "call.cancel";
}

int PersistentSessionTtlSec()
{
    return 86400;
}

int RingingSessionTtlSec(int ring_timeout_sec)
{
    return ring_timeout_sec + 600;
}

int RingingBusyTtlSec(int ring_timeout_sec, int busy_key_ttl_sec)
{
    const int ringing_ttl = ring_timeout_sec + 5;
    return ringing_ttl > busy_key_ttl_sec ? ringing_ttl : busy_key_ttl_sec;
}

long long TokenNotBeforeSec(long long now_sec)
{
    return now_sec - 5;
}

long long TokenExpiresAtSec(long long now_sec, int token_ttl_sec)
{
    return now_sec + token_ttl_sec;
}

const char* CallerRole()
{
    return "caller";
}

const char* CalleeRole()
{
    return "callee";
}

const char* DefaultParticipantRole(bool is_caller)
{
    return is_caller ? CallerRole() : CalleeRole();
}

const char* StateSyncEvent()
{
    return "call.state_sync";
}

const char* InviteEvent()
{
    return "call.invite";
}

const char* AcceptedEvent()
{
    return "call.accepted";
}

const char* RejectedEvent()
{
    return "call.rejected";
}

const char* HangupEvent()
{
    return "call.hangup";
}
} // namespace memochat::call::service::modules
