#include <gtest/gtest.h>

namespace memochat::tests::gate::h1
{
bool ShouldReadConnectionFields(bool has_connection);
bool ShouldSetContentType(bool content_type_empty);
bool ShouldApplyFileResponse(bool file_body);
} // namespace memochat::tests::gate::h1

TEST(H1RouteAdapterAlgorithmsTest, ReadsConnectionFieldsOnlyWhenConnectionExists)
{
    EXPECT_FALSE(memochat::tests::gate::h1::ShouldReadConnectionFields(false));
    EXPECT_TRUE(memochat::tests::gate::h1::ShouldReadConnectionFields(true));
}

TEST(H1RouteAdapterAlgorithmsTest, SetsContentTypeOnlyWhenRouteProvidesOne)
{
    EXPECT_FALSE(memochat::tests::gate::h1::ShouldSetContentType(true));
    EXPECT_TRUE(memochat::tests::gate::h1::ShouldSetContentType(false));
}

TEST(H1RouteAdapterAlgorithmsTest, AppliesFileResponseOnlyForFileBody)
{
    EXPECT_FALSE(memochat::tests::gate::h1::ShouldApplyFileResponse(false));
    EXPECT_TRUE(memochat::tests::gate::h1::ShouldApplyFileResponse(true));
}
