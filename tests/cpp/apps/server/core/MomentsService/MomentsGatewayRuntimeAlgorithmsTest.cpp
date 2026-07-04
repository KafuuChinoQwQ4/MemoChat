#include <gtest/gtest.h>

namespace memochat::tests::moments::gateway_runtime
{
const char* ServerName();
const char* GatewayName();
int DefaultListenPort();
bool ShouldInitAws();
} // namespace memochat::tests::moments::gateway_runtime

TEST(MomentsGatewayRuntimeAlgorithmsTest, ExposesEntrypointNamesAndPort)
{
    using namespace memochat::tests::moments::gateway_runtime;

    EXPECT_STREQ(ServerName(), "MomentsGatewayServer");
    EXPECT_STREQ(GatewayName(), "MomentsGateway");
    EXPECT_EQ(DefaultListenPort(), 8099);
}

TEST(MomentsGatewayRuntimeAlgorithmsTest, DoesNotInitAws)
{
    using namespace memochat::tests::moments::gateway_runtime;

    EXPECT_FALSE(ShouldInitAws());
}
