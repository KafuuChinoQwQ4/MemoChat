#include <gtest/gtest.h>

#include <string>

namespace memochat::tests::gate::h3_adapter
{
bool ShouldProcessConnection(bool has_connection);
bool ShouldAppendQueryToTarget(bool query_empty);
int FileBodyNotFoundStatus();
std::string FileBodyNotFoundBody();
std::string FileBodyNotFoundContentType();
std::string DefaultFileContentType();
std::string DefaultInlineContentType();
std::string
ResolveContentType(bool content_type_empty, const char* default_content_type, const char* route_content_type);
} // namespace memochat::tests::gate::h3_adapter

TEST(H3RouteAdapterAlgorithmsTest, ProcessesOnlyWhenConnectionExists)
{
    EXPECT_FALSE(memochat::tests::gate::h3_adapter::ShouldProcessConnection(false));
    EXPECT_TRUE(memochat::tests::gate::h3_adapter::ShouldProcessConnection(true));
}

TEST(H3RouteAdapterAlgorithmsTest, AppendsQueryOnlyWhenNonEmpty)
{
    EXPECT_FALSE(memochat::tests::gate::h3_adapter::ShouldAppendQueryToTarget(true));
    EXPECT_TRUE(memochat::tests::gate::h3_adapter::ShouldAppendQueryToTarget(false));
}

TEST(H3RouteAdapterAlgorithmsTest, FileBodyNotFoundMetadataIsStable)
{
    EXPECT_EQ(memochat::tests::gate::h3_adapter::FileBodyNotFoundStatus(), 404);
    EXPECT_EQ(memochat::tests::gate::h3_adapter::FileBodyNotFoundBody(),
              R"({"error":404,"message":"file response body not found"})");
    EXPECT_EQ(memochat::tests::gate::h3_adapter::FileBodyNotFoundContentType(), "application/json");
}

TEST(H3RouteAdapterAlgorithmsTest, DefaultContentTypes)
{
    EXPECT_EQ(memochat::tests::gate::h3_adapter::DefaultFileContentType(), "application/octet-stream");
    EXPECT_EQ(memochat::tests::gate::h3_adapter::DefaultInlineContentType(), "application/json");
}

TEST(H3RouteAdapterAlgorithmsTest, ResolveContentTypePicksDefaultWhenEmpty)
{
    EXPECT_EQ(memochat::tests::gate::h3_adapter::ResolveContentType(true, "application/json", "text/plain"),
              "application/json");
    EXPECT_EQ(memochat::tests::gate::h3_adapter::ResolveContentType(false, "application/json", "text/plain"),
              "text/plain");
}
