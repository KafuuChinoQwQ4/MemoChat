#include <gtest/gtest.h>

#include <cstring>

namespace memochat::tests::account::auth_service
{
int ResponseOkStatus();
const char* JsonContentType();
bool FeatureGroupChatEnabled();
bool IsRegisteredUidValid(int uid);
const char* FallbackTransport();
const char* QuicTransport();
const char* TcpTransport();
const char* PreferredTransport(bool has_quic_host, bool has_quic_port);
} // namespace memochat::tests::account::auth_service

TEST(AuthServiceAlgorithmsTest, ResponseMetadataMatchesCurrentValues)
{
    using namespace memochat::tests::account::auth_service;

    EXPECT_EQ(ResponseOkStatus(), 200);
    EXPECT_STREQ(JsonContentType(), "application/json");
    EXPECT_TRUE(FeatureGroupChatEnabled());
}

TEST(AuthServiceAlgorithmsTest, RegisteredUidGuardRejectsSentinels)
{
    using namespace memochat::tests::account::auth_service;

    EXPECT_FALSE(IsRegisteredUidValid(0));
    EXPECT_FALSE(IsRegisteredUidValid(-1));
    EXPECT_TRUE(IsRegisteredUidValid(1));
    EXPECT_TRUE(IsRegisteredUidValid(42));
    // Other negatives (not the -1 sentinel) are treated as valid uids, matching
    // the original `uid == 0 || uid == -1` guard exactly.
    EXPECT_TRUE(IsRegisteredUidValid(-2));
}

TEST(AuthServiceAlgorithmsTest, TransportLiteralsAndSelection)
{
    using namespace memochat::tests::account::auth_service;

    EXPECT_STREQ(FallbackTransport(), "tcp");
    EXPECT_STREQ(QuicTransport(), "quic");
    EXPECT_STREQ(TcpTransport(), "tcp");

    // QUIC is preferred only when both a QUIC host and QUIC port are present.
    EXPECT_STREQ(PreferredTransport(true, true), "quic");
    EXPECT_STREQ(PreferredTransport(true, false), "tcp");
    EXPECT_STREQ(PreferredTransport(false, true), "tcp");
    EXPECT_STREQ(PreferredTransport(false, false), "tcp");
}
