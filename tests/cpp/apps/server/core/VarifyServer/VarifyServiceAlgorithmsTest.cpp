#include <gtest/gtest.h>

namespace memochat::tests::varify::service
{
const char* Ipv4PeerPrefix();
const char* Ipv6PeerPrefix();
int PeerAddressPrefixLength();
const char* GrpcLogModuleName();
bool HasGrpcContext(bool context_present);
bool HasPeerText(bool peer_empty);
bool ShouldSendEmailForSyntheticAccount(bool synthetic_email);
} // namespace memochat::tests::varify::service

TEST(VarifyServiceAlgorithmsTest, ExposesStableGrpcPeerPrefixes)
{
    EXPECT_STREQ(memochat::tests::varify::service::Ipv4PeerPrefix(), "ipv4:");
    EXPECT_STREQ(memochat::tests::varify::service::Ipv6PeerPrefix(), "ipv6:");
    EXPECT_EQ(memochat::tests::varify::service::PeerAddressPrefixLength(), 5);
}

TEST(VarifyServiceAlgorithmsTest, ExposesStableGrpcLogModule)
{
    EXPECT_STREQ(memochat::tests::varify::service::GrpcLogModuleName(), "grpc");
}

TEST(VarifyServiceAlgorithmsTest, PreservesContextPeerAndSyntheticEmailGuards)
{
    EXPECT_FALSE(memochat::tests::varify::service::HasGrpcContext(false));
    EXPECT_TRUE(memochat::tests::varify::service::HasGrpcContext(true));

    EXPECT_FALSE(memochat::tests::varify::service::HasPeerText(true));
    EXPECT_TRUE(memochat::tests::varify::service::HasPeerText(false));

    EXPECT_TRUE(memochat::tests::varify::service::ShouldSendEmailForSyntheticAccount(false));
    EXPECT_FALSE(memochat::tests::varify::service::ShouldSendEmailForSyntheticAccount(true));
}
