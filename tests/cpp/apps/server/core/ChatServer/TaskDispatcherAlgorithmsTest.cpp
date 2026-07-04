#include <gtest/gtest.h>

#include <string>

bool MemoChatTestTaskDispatcherShouldPublish(bool has_task_bus);
bool MemoChatTestTaskDispatcherIsDeliveryTaskType(const std::string& task_type);
bool MemoChatTestTaskDispatcherIsOutboxRepairTaskType(const std::string& task_type);
bool MemoChatTestTaskDispatcherShouldExpediteOutboxRepair(bool has_scheduler, long long outbox_id);
bool MemoChatTestTaskDispatcherShouldAckInvalidPayload();
const char* MemoChatTestTaskDispatcherTaskBusUnavailableError();
const char* MemoChatTestTaskDispatcherTaskHandlerFailedError();
const char* MemoChatTestTaskDispatcherUnknownTaskTypeEventName();
const char* MemoChatTestTaskDispatcherUnknownTaskTypeMessage();
int MemoChatTestTaskDispatcherNoTaskSleepMs();

TEST(TaskDispatcherAlgorithmsTest, KeepsStableErrorAndLogLiterals)
{
    EXPECT_EQ(std::string(MemoChatTestTaskDispatcherTaskBusUnavailableError()), "task_bus_unavailable");
    EXPECT_EQ(std::string(MemoChatTestTaskDispatcherTaskHandlerFailedError()), "task_handler_failed");
    EXPECT_EQ(std::string(MemoChatTestTaskDispatcherUnknownTaskTypeEventName()), "chat.task.unknown_type");
    EXPECT_EQ(std::string(MemoChatTestTaskDispatcherUnknownTaskTypeMessage()), "task type is not registered");
}

TEST(TaskDispatcherAlgorithmsTest, ClassifiesDeliveryTaskTypes)
{
    EXPECT_TRUE(MemoChatTestTaskDispatcherIsDeliveryTaskType("message_delivery_retry"));
    EXPECT_TRUE(MemoChatTestTaskDispatcherIsDeliveryTaskType("offline_notify"));
    EXPECT_TRUE(MemoChatTestTaskDispatcherIsDeliveryTaskType("relation_notify"));
    EXPECT_FALSE(MemoChatTestTaskDispatcherIsDeliveryTaskType("outbox_repair"));
    EXPECT_FALSE(MemoChatTestTaskDispatcherIsDeliveryTaskType("message_delivery_retry_extra"));
}

TEST(TaskDispatcherAlgorithmsTest, ClassifiesOutboxRepairTaskType)
{
    EXPECT_TRUE(MemoChatTestTaskDispatcherIsOutboxRepairTaskType("outbox_repair"));
    EXPECT_FALSE(MemoChatTestTaskDispatcherIsOutboxRepairTaskType("outbox_repair_extra"));
    EXPECT_FALSE(MemoChatTestTaskDispatcherIsOutboxRepairTaskType("relation_notify"));
}

TEST(TaskDispatcherAlgorithmsTest, GuardsPublishAndOutboxRepair)
{
    EXPECT_TRUE(MemoChatTestTaskDispatcherShouldPublish(true));
    EXPECT_FALSE(MemoChatTestTaskDispatcherShouldPublish(false));
    EXPECT_TRUE(MemoChatTestTaskDispatcherShouldExpediteOutboxRepair(true, 1));
    EXPECT_FALSE(MemoChatTestTaskDispatcherShouldExpediteOutboxRepair(false, 1));
    EXPECT_FALSE(MemoChatTestTaskDispatcherShouldExpediteOutboxRepair(true, 0));
    EXPECT_FALSE(MemoChatTestTaskDispatcherShouldExpediteOutboxRepair(true, -1));
}

TEST(TaskDispatcherAlgorithmsTest, KeepsInvalidPayloadAckAndSleepDelay)
{
    EXPECT_TRUE(MemoChatTestTaskDispatcherShouldAckInvalidPayload());
    EXPECT_EQ(MemoChatTestTaskDispatcherNoTaskSleepMs(), 50);
}
