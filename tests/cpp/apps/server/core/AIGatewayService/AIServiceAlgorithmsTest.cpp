#include <gtest/gtest.h>

namespace memochat::tests::ai::gateway_service
{
int SuccessStatusCode();
const char* JsonContentType();
int InvalidJsonErrorCode();
const char* InvalidJsonMessage();
int ContentEmptyErrorCode();
const char* ContentEmptyMessage();
int UnauthorizedStatusCode();
int ForbiddenStatusCode();
int TokenInvalidErrorCode();
const char* TokenInvalidMessage();
const char* ProviderAdminAuthRequiredMessage();
const char* DefaultProviderAdminAuthHeader();
const char* DefaultProviderAdminKeyEnv();
int DefaultHistoryLimit();
int DefaultHistoryOffset();
int DefaultTaskListLimit();
const char* DefaultModelType();
bool ShouldRejectEmptyContent(bool content_empty);
bool ShouldRejectUserAuth(int uid, bool token_empty);
bool ShouldRejectProviderAdminAuth(bool key_configured, bool supplied_empty, bool token_matches);
} // namespace memochat::tests::ai::gateway_service

TEST(AIServiceAlgorithmsTest, ExposesGatewayJsonResponseConstants)
{
    EXPECT_EQ(memochat::tests::ai::gateway_service::SuccessStatusCode(), 200);
    EXPECT_STREQ(memochat::tests::ai::gateway_service::JsonContentType(), "application/json");
}

TEST(AIServiceAlgorithmsTest, ExposesStableGatewayErrorPayloads)
{
    EXPECT_EQ(memochat::tests::ai::gateway_service::InvalidJsonErrorCode(), 1);
    EXPECT_STREQ(memochat::tests::ai::gateway_service::InvalidJsonMessage(), "invalid json");
    EXPECT_EQ(memochat::tests::ai::gateway_service::ContentEmptyErrorCode(), 1);
    EXPECT_STREQ(memochat::tests::ai::gateway_service::ContentEmptyMessage(), "content is empty");
    EXPECT_EQ(memochat::tests::ai::gateway_service::UnauthorizedStatusCode(), 401);
    EXPECT_EQ(memochat::tests::ai::gateway_service::ForbiddenStatusCode(), 403);
    EXPECT_EQ(memochat::tests::ai::gateway_service::TokenInvalidErrorCode(), 401);
    EXPECT_STREQ(memochat::tests::ai::gateway_service::TokenInvalidMessage(), "token invalid or missing");
    EXPECT_STREQ(memochat::tests::ai::gateway_service::ProviderAdminAuthRequiredMessage(),
                 "provider admin auth required");
    EXPECT_STREQ(memochat::tests::ai::gateway_service::DefaultProviderAdminAuthHeader(),
                 "X-MemoChat-AI-Provider-Admin-Key");
    EXPECT_STREQ(memochat::tests::ai::gateway_service::DefaultProviderAdminKeyEnv(), "MEMOCHAT_AI_PROVIDER_ADMIN_KEY");
}

TEST(AIServiceAlgorithmsTest, ExposesGatewayQueryDefaults)
{
    EXPECT_EQ(memochat::tests::ai::gateway_service::DefaultHistoryLimit(), 20);
    EXPECT_EQ(memochat::tests::ai::gateway_service::DefaultHistoryOffset(), 0);
    EXPECT_EQ(memochat::tests::ai::gateway_service::DefaultTaskListLimit(), 50);
    EXPECT_STREQ(memochat::tests::ai::gateway_service::DefaultModelType(), "ollama");
}

TEST(AIServiceAlgorithmsTest, RejectsOnlyEmptyContent)
{
    EXPECT_TRUE(memochat::tests::ai::gateway_service::ShouldRejectEmptyContent(true));
    EXPECT_FALSE(memochat::tests::ai::gateway_service::ShouldRejectEmptyContent(false));
}

TEST(AIServiceAlgorithmsTest, RejectsMissingUserAuthInput)
{
    EXPECT_TRUE(memochat::tests::ai::gateway_service::ShouldRejectUserAuth(0, false));
    EXPECT_TRUE(memochat::tests::ai::gateway_service::ShouldRejectUserAuth(-1, false));
    EXPECT_TRUE(memochat::tests::ai::gateway_service::ShouldRejectUserAuth(42, true));
    EXPECT_FALSE(memochat::tests::ai::gateway_service::ShouldRejectUserAuth(42, false));
}

TEST(AIServiceAlgorithmsTest, RejectsMissingProviderAdminAuthInput)
{
    EXPECT_TRUE(memochat::tests::ai::gateway_service::ShouldRejectProviderAdminAuth(false, false, true));
    EXPECT_TRUE(memochat::tests::ai::gateway_service::ShouldRejectProviderAdminAuth(true, true, true));
    EXPECT_TRUE(memochat::tests::ai::gateway_service::ShouldRejectProviderAdminAuth(true, false, false));
    EXPECT_FALSE(memochat::tests::ai::gateway_service::ShouldRejectProviderAdminAuth(true, false, true));
}
