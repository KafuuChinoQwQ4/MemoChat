#include <gtest/gtest.h>

namespace memochat::tests::call::service
{
bool TextEquals(const char* lhs, const char* rhs);
bool IsEnabledText(const char* value);
int NormalizeRingTimeoutSec(int value);
int NormalizeBusyKeyTtlSec(int value);
int NormalizeTokenTtlSec(int value);
bool HasValidAuthRequest(int uid, bool token_empty);
bool HasValidStartPeer(int uid, int peer_uid);
bool IsSupportedCallType(const char* call_type);
const char* RingingState();
const char* AcceptedState();
const char* RejectedState();
const char* CancelledState();
const char* TimeoutState();
const char* EndedState();
const char* JoiningState();
const char* InCallState();
const char* HangupReason();
bool IsRingingState(const char* state);
bool IsTimeoutState(const char* state);
bool IsActiveCallState(const char* state);
bool HasCancelTimedOut(long long now_ms, long long expires_at_ms);
const char* CancelTerminalState(bool timed_out);
const char* CancelTerminalEvent(bool timed_out);
int PersistentSessionTtlSec();
int RingingSessionTtlSec(int ring_timeout_sec);
int RingingBusyTtlSec(int ring_timeout_sec, int busy_key_ttl_sec);
long long TokenNotBeforeSec(long long now_sec);
long long TokenExpiresAtSec(long long now_sec, int token_ttl_sec);
const char* CallerRole();
const char* CalleeRole();
const char* DefaultParticipantRole(bool is_caller);
const char* StateSyncEvent();
const char* InviteEvent();
const char* AcceptedEvent();
const char* RejectedEvent();
const char* HangupEvent();
} // namespace memochat::tests::call::service

TEST(CallServiceAlgorithmsTest, ExposesConfigNormalizationAndRequestGuards)
{
    using namespace memochat::tests::call::service;

    EXPECT_TRUE(TextEquals("voice", "voice"));
    EXPECT_FALSE(TextEquals("voice", "video"));
    EXPECT_TRUE(TextEquals(nullptr, nullptr));
    EXPECT_FALSE(TextEquals(nullptr, "voice"));

    EXPECT_TRUE(IsEnabledText("1"));
    EXPECT_TRUE(IsEnabledText("true"));
    EXPECT_TRUE(IsEnabledText("TRUE"));
    EXPECT_TRUE(IsEnabledText("True"));
    EXPECT_FALSE(IsEnabledText(""));
    EXPECT_FALSE(IsEnabledText("false"));

    EXPECT_EQ(NormalizeRingTimeoutSec(0), 15);
    EXPECT_EQ(NormalizeRingTimeoutSec(16), 16);
    EXPECT_EQ(NormalizeBusyKeyTtlSec(0), 15);
    EXPECT_EQ(NormalizeBusyKeyTtlSec(20), 20);
    EXPECT_EQ(NormalizeTokenTtlSec(0), 300);
    EXPECT_EQ(NormalizeTokenTtlSec(301), 301);

    EXPECT_TRUE(HasValidAuthRequest(1, false));
    EXPECT_FALSE(HasValidAuthRequest(0, false));
    EXPECT_FALSE(HasValidAuthRequest(1, true));
    EXPECT_TRUE(HasValidStartPeer(1, 2));
    EXPECT_FALSE(HasValidStartPeer(1, 1));
    EXPECT_FALSE(HasValidStartPeer(1, 0));
    EXPECT_TRUE(IsSupportedCallType("voice"));
    EXPECT_TRUE(IsSupportedCallType("video"));
    EXPECT_FALSE(IsSupportedCallType("screen"));
}

TEST(CallServiceAlgorithmsTest, ExposesStateAndEventLiterals)
{
    using namespace memochat::tests::call::service;

    EXPECT_STREQ(RingingState(), "ringing");
    EXPECT_STREQ(AcceptedState(), "accepted");
    EXPECT_STREQ(RejectedState(), "rejected");
    EXPECT_STREQ(CancelledState(), "cancelled");
    EXPECT_STREQ(TimeoutState(), "timeout");
    EXPECT_STREQ(EndedState(), "ended");
    EXPECT_STREQ(JoiningState(), "joining");
    EXPECT_STREQ(InCallState(), "in_call");
    EXPECT_STREQ(HangupReason(), "hangup");
    EXPECT_TRUE(IsRingingState("ringing"));
    EXPECT_FALSE(IsRingingState("accepted"));
    EXPECT_TRUE(IsTimeoutState("timeout"));
    EXPECT_FALSE(IsTimeoutState("cancelled"));
    EXPECT_TRUE(IsActiveCallState("accepted"));
    EXPECT_TRUE(IsActiveCallState("joining"));
    EXPECT_TRUE(IsActiveCallState("in_call"));
    EXPECT_FALSE(IsActiveCallState("ringing"));

    EXPECT_STREQ(StateSyncEvent(), "call.state_sync");
    EXPECT_STREQ(InviteEvent(), "call.invite");
    EXPECT_STREQ(AcceptedEvent(), "call.accepted");
    EXPECT_STREQ(RejectedEvent(), "call.rejected");
    EXPECT_STREQ(HangupEvent(), "call.hangup");
}

TEST(CallServiceAlgorithmsTest, ExposesCancelSessionAndTokenDecisions)
{
    using namespace memochat::tests::call::service;

    EXPECT_FALSE(HasCancelTimedOut(100, 100));
    EXPECT_TRUE(HasCancelTimedOut(101, 100));
    EXPECT_STREQ(CancelTerminalState(false), "cancelled");
    EXPECT_STREQ(CancelTerminalState(true), "timeout");
    EXPECT_STREQ(CancelTerminalEvent(false), "call.cancel");
    EXPECT_STREQ(CancelTerminalEvent(true), "call.timeout");

    EXPECT_EQ(PersistentSessionTtlSec(), 86400);
    EXPECT_EQ(RingingSessionTtlSec(45), 645);
    EXPECT_EQ(RingingBusyTtlSec(45, 40), 50);
    EXPECT_EQ(RingingBusyTtlSec(45, 70), 70);
    EXPECT_EQ(TokenNotBeforeSec(1000), 995);
    EXPECT_EQ(TokenExpiresAtSec(1000, 300), 1300);

    EXPECT_STREQ(CallerRole(), "caller");
    EXPECT_STREQ(CalleeRole(), "callee");
    EXPECT_STREQ(DefaultParticipantRole(true), "caller");
    EXPECT_STREQ(DefaultParticipantRole(false), "callee");
}
