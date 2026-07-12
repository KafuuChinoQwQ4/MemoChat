#include <gtest/gtest.h>

namespace memochat::tests::ai::core
{
int SuccessCode();
int BadRequestCode();
int NotFoundCode();
int UpstreamFailureCode();
int ServiceUnavailableCode();
int DefaultHistoryLimit();
int SelectHistoryLimit(int requested_limit);
const char* OkMessage();
const char* UserRole();
const char* AssistantRole();
const char* EmptyJsonObject();
const char* EmptyJsonArray();
const char* RequiredSessionUpdateMessage();
const char* SessionNotFoundMessage();
const char* SessionCreateFailedMessage();
const char* DefaultApiProviderAdapter();
bool IsAsciiTrimChar(char ch);
bool ShouldCreateSessionFromRequest(bool model_type_empty, bool model_name_empty);
bool ShouldRejectSessionUpdate(int uid, bool session_id_empty, bool title_empty);
bool ShouldUseDefaultApiProviderAdapter(bool adapter_empty);
} // namespace memochat::tests::ai::core

TEST(AIServiceCoreAlgorithmsTest, ExposesCoreResponseCodesAndLiterals)
{
    using namespace memochat::tests::ai::core;

    EXPECT_EQ(SuccessCode(), 0);
    EXPECT_EQ(BadRequestCode(), 400);
    EXPECT_EQ(NotFoundCode(), 404);
    EXPECT_EQ(UpstreamFailureCode(), 500);
    EXPECT_EQ(ServiceUnavailableCode(), 503);
    EXPECT_STREQ(OkMessage(), "ok");
    EXPECT_STREQ(UserRole(), "user");
    EXPECT_STREQ(AssistantRole(), "assistant");
    EXPECT_STREQ(EmptyJsonObject(), "{}");
    EXPECT_STREQ(EmptyJsonArray(), "[]");
    EXPECT_STREQ(RequiredSessionUpdateMessage(), "uid, session_id and title are required");
    EXPECT_STREQ(SessionNotFoundMessage(), "session not found");
    EXPECT_STREQ(SessionCreateFailedMessage(), "session creation failed");
    EXPECT_STREQ(DefaultApiProviderAdapter(), "openai_compatible");
}

TEST(AIServiceCoreAlgorithmsTest, NormalizesHistoryLimitAndCoreGuards)
{
    using namespace memochat::tests::ai::core;

    EXPECT_EQ(DefaultHistoryLimit(), 20);
    EXPECT_EQ(SelectHistoryLimit(50), 50);
    EXPECT_EQ(SelectHistoryLimit(0), 20);
    EXPECT_EQ(SelectHistoryLimit(-10), 20);

    EXPECT_TRUE(IsAsciiTrimChar(' '));
    EXPECT_TRUE(IsAsciiTrimChar('\t'));
    EXPECT_TRUE(IsAsciiTrimChar('\r'));
    EXPECT_TRUE(IsAsciiTrimChar('\n'));
    EXPECT_FALSE(IsAsciiTrimChar('x'));

    EXPECT_TRUE(ShouldCreateSessionFromRequest(false, false));
    EXPECT_FALSE(ShouldCreateSessionFromRequest(true, false));
    EXPECT_FALSE(ShouldCreateSessionFromRequest(false, true));

    EXPECT_FALSE(ShouldRejectSessionUpdate(1, false, false));
    EXPECT_TRUE(ShouldRejectSessionUpdate(0, false, false));
    EXPECT_TRUE(ShouldRejectSessionUpdate(1, true, false));
    EXPECT_TRUE(ShouldRejectSessionUpdate(1, false, true));

    EXPECT_TRUE(ShouldUseDefaultApiProviderAdapter(true));
    EXPECT_FALSE(ShouldUseDefaultApiProviderAdapter(false));
}
