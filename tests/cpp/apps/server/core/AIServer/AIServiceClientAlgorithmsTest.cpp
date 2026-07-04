#include <gtest/gtest.h>

namespace memochat::tests::ai::client
{
int DefaultTimeoutSec();
int DefaultAgentTaskLimit();
int HttpVersion();
int SuccessCode();
const char* JsonContentType();
const char* EmptyBody();
const char* EmptyJsonArray();
const char* ChatPath();
const char* ChatStreamPath();
const char* SmartPath();
const char* KbUploadPath();
const char* KbSearchPath();
const char* ModelsPath();
const char* RegisterApiProviderPath();
const char* DeleteApiProviderPath();
const char* KbListPathPrefix();
const char* KbPathPrefix();
const char* UidQueryPrefix();
const char* AgentMemoryListPathPrefix();
const char* AgentMemoryPath();
const char* AgentMemoryItemPathPrefix();
const char* AgentTasksPath();
const char* AgentTasksListPathPrefix();
const char* LimitQueryPrefix();
const char* CancelSuffix();
const char* ResumeSuffix();
const char* DefaultDeploymentPreference();
const char* DefaultApiProviderAdapter();
unsigned long SseDataPrefixSize();
bool IsSseDataLine(const char* line, unsigned long line_size);
bool IsSseDoneSentinel(const char* text, unsigned long text_size);
bool ShouldSetJsonBody(bool has_body);
bool ShouldAttachMetadata(bool metadata_empty);
bool ShouldUseParsedMetadata(bool parse_ok, bool metadata_is_object);
bool ShouldKeepUrlByte(bool is_alnum, unsigned char ch);
bool ShouldUseDefaultDeploymentPreference(bool preference_empty);
bool ShouldUseDefaultApiProviderAdapter(bool adapter_empty);
bool ShouldUpdateCurrentMessageId(bool msg_id_empty);
bool ShouldUpdateTotalTokens(long long tokens);
bool ShouldEmitChunk(bool callback_present);
bool ShouldStoreFinalChunkResult(bool final_flag, bool out_result_present);
bool ShouldFallbackStreamResult(bool saw_final, bool out_result_present);
bool IsLineTrimChar(char ch);
} // namespace memochat::tests::ai::client

TEST(AIServiceClientAlgorithmsTest, ExposesHttpDefaultsAndRoutePaths)
{
    using namespace memochat::tests::ai::client;

    EXPECT_EQ(DefaultTimeoutSec(), 300);
    EXPECT_EQ(DefaultAgentTaskLimit(), 50);
    EXPECT_EQ(HttpVersion(), 11);
    EXPECT_EQ(SuccessCode(), 0);
    EXPECT_STREQ(JsonContentType(), "application/json");
    EXPECT_STREQ(EmptyBody(), "");
    EXPECT_STREQ(EmptyJsonArray(), "[]");
    EXPECT_STREQ(ChatPath(), "/chat");
    EXPECT_STREQ(ChatStreamPath(), "/chat/stream");
    EXPECT_STREQ(SmartPath(), "/smart");
    EXPECT_STREQ(KbUploadPath(), "/kb/upload");
    EXPECT_STREQ(KbSearchPath(), "/kb/search");
    EXPECT_STREQ(ModelsPath(), "/models");
    EXPECT_STREQ(RegisterApiProviderPath(), "/models/api-provider");
    EXPECT_STREQ(DeleteApiProviderPath(), "/models/api-provider/delete");
    EXPECT_STREQ(KbListPathPrefix(), "/kb/list?uid=");
    EXPECT_STREQ(KbPathPrefix(), "/kb/");
    EXPECT_STREQ(UidQueryPrefix(), "?uid=");
    EXPECT_STREQ(AgentMemoryListPathPrefix(), "/agent/memory?uid=");
    EXPECT_STREQ(AgentMemoryPath(), "/agent/memory");
    EXPECT_STREQ(AgentMemoryItemPathPrefix(), "/agent/memory/");
    EXPECT_STREQ(AgentTasksPath(), "/agent/tasks");
    EXPECT_STREQ(AgentTasksListPathPrefix(), "/agent/tasks?uid=");
    EXPECT_STREQ(LimitQueryPrefix(), "&limit=");
    EXPECT_STREQ(CancelSuffix(), "/cancel");
    EXPECT_STREQ(ResumeSuffix(), "/resume");
    EXPECT_STREQ(DefaultDeploymentPreference(), "any");
    EXPECT_STREQ(DefaultApiProviderAdapter(), "openai_compatible");
}

TEST(AIServiceClientAlgorithmsTest, ClassifiesMetadataUrlAndSseGuards)
{
    using namespace memochat::tests::ai::client;

    EXPECT_EQ(SseDataPrefixSize(), 6U);
    EXPECT_TRUE(IsSseDataLine("data: {}", 8));
    EXPECT_FALSE(IsSseDataLine("event: ping", 11));
    EXPECT_FALSE(IsSseDataLine("data:", 5));
    EXPECT_TRUE(IsSseDoneSentinel("[DONE]", 6));
    EXPECT_FALSE(IsSseDoneSentinel("[DONE] ", 7));

    EXPECT_TRUE(ShouldSetJsonBody(true));
    EXPECT_FALSE(ShouldSetJsonBody(false));
    EXPECT_FALSE(ShouldAttachMetadata(true));
    EXPECT_TRUE(ShouldAttachMetadata(false));
    EXPECT_TRUE(ShouldUseParsedMetadata(true, true));
    EXPECT_FALSE(ShouldUseParsedMetadata(true, false));
    EXPECT_FALSE(ShouldUseParsedMetadata(false, true));

    EXPECT_TRUE(ShouldKeepUrlByte(true, static_cast<unsigned char>('A')));
    EXPECT_TRUE(ShouldKeepUrlByte(false, static_cast<unsigned char>('-')));
    EXPECT_TRUE(ShouldKeepUrlByte(false, static_cast<unsigned char>('_')));
    EXPECT_TRUE(ShouldKeepUrlByte(false, static_cast<unsigned char>('.')));
    EXPECT_TRUE(ShouldKeepUrlByte(false, static_cast<unsigned char>('~')));
    EXPECT_FALSE(ShouldKeepUrlByte(false, static_cast<unsigned char>(' ')));

    EXPECT_TRUE(ShouldUseDefaultDeploymentPreference(true));
    EXPECT_FALSE(ShouldUseDefaultDeploymentPreference(false));
    EXPECT_TRUE(ShouldUseDefaultApiProviderAdapter(true));
    EXPECT_FALSE(ShouldUseDefaultApiProviderAdapter(false));
    EXPECT_TRUE(ShouldUpdateCurrentMessageId(false));
    EXPECT_FALSE(ShouldUpdateCurrentMessageId(true));
    EXPECT_FALSE(ShouldUpdateTotalTokens(0));
    EXPECT_FALSE(ShouldUpdateTotalTokens(-1));
    EXPECT_TRUE(ShouldUpdateTotalTokens(42));
    EXPECT_TRUE(ShouldEmitChunk(true));
    EXPECT_FALSE(ShouldEmitChunk(false));
    EXPECT_TRUE(ShouldStoreFinalChunkResult(true, true));
    EXPECT_FALSE(ShouldStoreFinalChunkResult(true, false));
    EXPECT_FALSE(ShouldStoreFinalChunkResult(false, true));
    EXPECT_TRUE(ShouldFallbackStreamResult(false, true));
    EXPECT_FALSE(ShouldFallbackStreamResult(true, true));
    EXPECT_FALSE(ShouldFallbackStreamResult(false, false));
    EXPECT_TRUE(IsLineTrimChar(' '));
    EXPECT_TRUE(IsLineTrimChar('\t'));
    EXPECT_TRUE(IsLineTrimChar('\r'));
    EXPECT_FALSE(IsLineTrimChar('\n'));
}
