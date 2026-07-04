#include <gtest/gtest.h>

namespace memochat::tests::ai::impl
{
const char* RpcKind();
const char* ChatSpan();
const char* ChatStreamSpan();
const char* SmartSpan();
const char* GetHistorySpan();
const char* CreateSessionSpan();
const char* ListSessionsSpan();
const char* DeleteSessionSpan();
const char* UpdateSessionSpan();
const char* ListModelsSpan();
const char* RegisterApiProviderSpan();
const char* DeleteApiProviderSpan();
const char* KbUploadSpan();
const char* KbSearchSpan();
const char* KbListSpan();
const char* KbDeleteSpan();
const char* MemoryListSpan();
const char* MemoryCreateSpan();
const char* MemoryDeleteSpan();
const char* AgentTaskCreateSpan();
const char* AgentTaskListSpan();
const char* AgentTaskGetSpan();
const char* AgentTaskCancelSpan();
const char* AgentTaskResumeSpan();
const char* ConfirmSpan();
} // namespace memochat::tests::ai::impl

TEST(AIServiceImplAlgorithmsTest, ExposesRpcKindAndChatSessionSpanNames)
{
    using namespace memochat::tests::ai::impl;

    EXPECT_STREQ(RpcKind(), "gRPC");
    EXPECT_STREQ(ChatSpan(), "AIService.Chat");
    EXPECT_STREQ(ChatStreamSpan(), "AIService.ChatStream");
    EXPECT_STREQ(SmartSpan(), "AIService.Smart");
    EXPECT_STREQ(GetHistorySpan(), "AIService.GetHistory");
    EXPECT_STREQ(CreateSessionSpan(), "AIService.CreateSession");
    EXPECT_STREQ(ListSessionsSpan(), "AIService.ListSessions");
    EXPECT_STREQ(DeleteSessionSpan(), "AIService.DeleteSession");
    EXPECT_STREQ(UpdateSessionSpan(), "AIService.UpdateSession");
    EXPECT_STREQ(ListModelsSpan(), "AIService.ListModels");
}

TEST(AIServiceImplAlgorithmsTest, ExposesProviderKnowledgeMemoryAndAgentSpanNames)
{
    using namespace memochat::tests::ai::impl;

    EXPECT_STREQ(RegisterApiProviderSpan(), "AIService.RegisterApiProvider");
    EXPECT_STREQ(DeleteApiProviderSpan(), "AIService.DeleteApiProvider");
    EXPECT_STREQ(KbUploadSpan(), "AIService.KbUpload");
    EXPECT_STREQ(KbSearchSpan(), "AIService.KbSearch");
    EXPECT_STREQ(KbListSpan(), "AIService.KbList");
    EXPECT_STREQ(KbDeleteSpan(), "AIService.KbDelete");
    EXPECT_STREQ(MemoryListSpan(), "AIService.MemoryList");
    EXPECT_STREQ(MemoryCreateSpan(), "AIService.MemoryCreate");
    EXPECT_STREQ(MemoryDeleteSpan(), "AIService.MemoryDelete");
    EXPECT_STREQ(AgentTaskCreateSpan(), "AIService.AgentTaskCreate");
    EXPECT_STREQ(AgentTaskListSpan(), "AIService.AgentTaskList");
    EXPECT_STREQ(AgentTaskGetSpan(), "AIService.AgentTaskGet");
    EXPECT_STREQ(AgentTaskCancelSpan(), "AIService.AgentTaskCancel");
    EXPECT_STREQ(AgentTaskResumeSpan(), "AIService.AgentTaskResume");
    EXPECT_STREQ(ConfirmSpan(), "AIService.Confirm");
}
