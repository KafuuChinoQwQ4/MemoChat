#include <gtest/gtest.h>

bool MemoChatTestAsyncDispatchShouldRejectPublish(bool event_bus_present);
bool MemoChatTestAsyncDispatchShouldPauseWhenWorkerDisabled(bool worker_enabled);
bool MemoChatTestAsyncDispatchShouldPollEventBus(bool event_bus_present);
bool MemoChatTestAsyncDispatchShouldAckInvalidEvent(bool parsed);
bool MemoChatTestAsyncDispatchShouldSleepAfterPoll(bool handled);
bool MemoChatTestAsyncDispatchShouldRejectJsonObject(bool is_object);
bool MemoChatTestAsyncDispatchShouldInvalidateRelationBootstrapCache(int uid, bool cache_present);
bool MemoChatTestAsyncDispatchShouldAppendUniqueUid(int uid, bool already_present);
bool MemoChatTestAsyncDispatchShouldNotifyMessageStatus(int uid);
bool MemoChatTestAsyncDispatchShouldRejectPrivateMessagePayload(int from_uid,
                                                                int to_uid,
                                                                bool text_array,
                                                                bool text_empty);
bool MemoChatTestAsyncDispatchShouldKeepPrivateMessage(bool msg_id_empty, bool content_empty);
bool MemoChatTestAsyncDispatchShouldUseCurrentTimestamp(long long created_at);
bool MemoChatTestAsyncDispatchShouldCaptureFirstMessageId(bool first_msg_id_empty);
bool MemoChatTestAsyncDispatchShouldPublishDialogSync(bool client_msg_id_empty);
bool MemoChatTestAsyncDispatchShouldRejectGroupMessagePayload(int from_uid, long long group_id, bool msg_object);
bool MemoChatTestAsyncDispatchShouldKeepGroupMessage(bool msg_id_empty, bool content_empty);
bool MemoChatTestAsyncDispatchShouldPushGroupDelivery(bool delivery_gateway_present);
bool MemoChatTestAsyncDispatchShouldRefreshDialogWarning(bool repository_present, bool refresh_ok);
bool MemoChatTestAsyncDispatchShouldLogPrivateRouteWarning(bool stale_route, bool notify_delivered);
int MemoChatTestAsyncDispatchWorkerDisabledSleepMs();
int MemoChatTestAsyncDispatchIdlePollSleepMs();
const char* MemoChatTestAsyncDispatchEventBusUnavailableError();
const char* MemoChatTestAsyncDispatchInvalidEventLogEvent();
const char* MemoChatTestAsyncDispatchInvalidEventLogMessage();
const char* MemoChatTestAsyncDispatchUnknownTopicLogEvent();
const char* MemoChatTestAsyncDispatchUnknownTopicLogMessage();
const char* MemoChatTestAsyncDispatchPrivateRouteEvent();
const char* MemoChatTestAsyncDispatchPrivateNotifyDeliveredMessage();
const char* MemoChatTestAsyncDispatchPrivateNotifyNotDeliveredMessage();
const char* MemoChatTestAsyncDispatchDialogSyncPublishFailedEvent();
const char* MemoChatTestAsyncDispatchPrivateDialogSyncPublishFailedMessage();
const char* MemoChatTestAsyncDispatchGroupDialogSyncPublishFailedMessage();
const char* MemoChatTestAsyncDispatchDialogSyncRefreshFailedEvent();
const char* MemoChatTestAsyncDispatchDialogSyncRefreshFailedMessage();
const char* MemoChatTestAsyncDispatchRelationStateRefreshFailedEvent();
const char* MemoChatTestAsyncDispatchRelationStateRefreshFailedMessage();

TEST(AsyncEventDispatcherAlgorithmsTest, KeepsPublishLoopAndPollGuards)
{
    EXPECT_TRUE(MemoChatTestAsyncDispatchShouldRejectPublish(false));
    EXPECT_FALSE(MemoChatTestAsyncDispatchShouldRejectPublish(true));

    EXPECT_TRUE(MemoChatTestAsyncDispatchShouldPauseWhenWorkerDisabled(false));
    EXPECT_FALSE(MemoChatTestAsyncDispatchShouldPauseWhenWorkerDisabled(true));

    EXPECT_TRUE(MemoChatTestAsyncDispatchShouldPollEventBus(true));
    EXPECT_FALSE(MemoChatTestAsyncDispatchShouldPollEventBus(false));

    EXPECT_TRUE(MemoChatTestAsyncDispatchShouldAckInvalidEvent(false));
    EXPECT_FALSE(MemoChatTestAsyncDispatchShouldAckInvalidEvent(true));

    EXPECT_TRUE(MemoChatTestAsyncDispatchShouldSleepAfterPoll(false));
    EXPECT_FALSE(MemoChatTestAsyncDispatchShouldSleepAfterPoll(true));

    EXPECT_EQ(MemoChatTestAsyncDispatchWorkerDisabledSleepMs(), 200);
    EXPECT_EQ(MemoChatTestAsyncDispatchIdlePollSleepMs(), 50);
}

TEST(AsyncEventDispatcherAlgorithmsTest, KeepsUidObjectAndRouteGuards)
{
    EXPECT_TRUE(MemoChatTestAsyncDispatchShouldRejectJsonObject(false));
    EXPECT_FALSE(MemoChatTestAsyncDispatchShouldRejectJsonObject(true));

    EXPECT_TRUE(MemoChatTestAsyncDispatchShouldInvalidateRelationBootstrapCache(1, true));
    EXPECT_FALSE(MemoChatTestAsyncDispatchShouldInvalidateRelationBootstrapCache(0, true));
    EXPECT_FALSE(MemoChatTestAsyncDispatchShouldInvalidateRelationBootstrapCache(1, false));

    EXPECT_TRUE(MemoChatTestAsyncDispatchShouldAppendUniqueUid(12, false));
    EXPECT_FALSE(MemoChatTestAsyncDispatchShouldAppendUniqueUid(12, true));
    EXPECT_FALSE(MemoChatTestAsyncDispatchShouldAppendUniqueUid(0, false));

    EXPECT_TRUE(MemoChatTestAsyncDispatchShouldNotifyMessageStatus(10));
    EXPECT_FALSE(MemoChatTestAsyncDispatchShouldNotifyMessageStatus(0));

    EXPECT_TRUE(MemoChatTestAsyncDispatchShouldLogPrivateRouteWarning(true, true));
    EXPECT_TRUE(MemoChatTestAsyncDispatchShouldLogPrivateRouteWarning(false, false));
    EXPECT_FALSE(MemoChatTestAsyncDispatchShouldLogPrivateRouteWarning(false, true));
}

TEST(AsyncEventDispatcherAlgorithmsTest, KeepsPrivateMessagePayloadGuards)
{
    EXPECT_FALSE(MemoChatTestAsyncDispatchShouldRejectPrivateMessagePayload(1, 2, true, false));
    EXPECT_TRUE(MemoChatTestAsyncDispatchShouldRejectPrivateMessagePayload(0, 2, true, false));
    EXPECT_TRUE(MemoChatTestAsyncDispatchShouldRejectPrivateMessagePayload(1, 0, true, false));
    EXPECT_TRUE(MemoChatTestAsyncDispatchShouldRejectPrivateMessagePayload(1, 2, false, false));
    EXPECT_TRUE(MemoChatTestAsyncDispatchShouldRejectPrivateMessagePayload(1, 2, true, true));

    EXPECT_TRUE(MemoChatTestAsyncDispatchShouldKeepPrivateMessage(false, false));
    EXPECT_FALSE(MemoChatTestAsyncDispatchShouldKeepPrivateMessage(true, false));
    EXPECT_FALSE(MemoChatTestAsyncDispatchShouldKeepPrivateMessage(false, true));

    EXPECT_TRUE(MemoChatTestAsyncDispatchShouldUseCurrentTimestamp(0));
    EXPECT_TRUE(MemoChatTestAsyncDispatchShouldUseCurrentTimestamp(-1));
    EXPECT_FALSE(MemoChatTestAsyncDispatchShouldUseCurrentTimestamp(1));

    EXPECT_TRUE(MemoChatTestAsyncDispatchShouldCaptureFirstMessageId(true));
    EXPECT_FALSE(MemoChatTestAsyncDispatchShouldCaptureFirstMessageId(false));
}

TEST(AsyncEventDispatcherAlgorithmsTest, KeepsGroupDialogAndRefreshGuards)
{
    EXPECT_TRUE(MemoChatTestAsyncDispatchShouldPublishDialogSync(false));
    EXPECT_FALSE(MemoChatTestAsyncDispatchShouldPublishDialogSync(true));

    EXPECT_FALSE(MemoChatTestAsyncDispatchShouldRejectGroupMessagePayload(1, 10, true));
    EXPECT_TRUE(MemoChatTestAsyncDispatchShouldRejectGroupMessagePayload(0, 10, true));
    EXPECT_TRUE(MemoChatTestAsyncDispatchShouldRejectGroupMessagePayload(1, 0, true));
    EXPECT_TRUE(MemoChatTestAsyncDispatchShouldRejectGroupMessagePayload(1, 10, false));

    EXPECT_TRUE(MemoChatTestAsyncDispatchShouldKeepGroupMessage(false, false));
    EXPECT_FALSE(MemoChatTestAsyncDispatchShouldKeepGroupMessage(true, false));
    EXPECT_FALSE(MemoChatTestAsyncDispatchShouldKeepGroupMessage(false, true));

    EXPECT_TRUE(MemoChatTestAsyncDispatchShouldPushGroupDelivery(true));
    EXPECT_FALSE(MemoChatTestAsyncDispatchShouldPushGroupDelivery(false));

    EXPECT_FALSE(MemoChatTestAsyncDispatchShouldRefreshDialogWarning(true, true));
    EXPECT_TRUE(MemoChatTestAsyncDispatchShouldRefreshDialogWarning(true, false));
    EXPECT_TRUE(MemoChatTestAsyncDispatchShouldRefreshDialogWarning(false, false));
}

TEST(AsyncEventDispatcherAlgorithmsTest, KeepsStableErrorAndLogLiterals)
{
    EXPECT_STREQ(MemoChatTestAsyncDispatchEventBusUnavailableError(), "event_bus_unavailable");
    EXPECT_STREQ(MemoChatTestAsyncDispatchInvalidEventLogEvent(), "chat.async.invalid_event");
    EXPECT_STREQ(MemoChatTestAsyncDispatchInvalidEventLogMessage(), "async chat event parse failed");
    EXPECT_STREQ(MemoChatTestAsyncDispatchUnknownTopicLogEvent(), "chat.async.unknown_topic");
    EXPECT_STREQ(MemoChatTestAsyncDispatchUnknownTopicLogMessage(), "async chat event topic is not registered");
    EXPECT_STREQ(MemoChatTestAsyncDispatchPrivateRouteEvent(), "chat.private.route.async");
    EXPECT_STREQ(MemoChatTestAsyncDispatchPrivateNotifyDeliveredMessage(), "private message notify delivered");
    EXPECT_STREQ(MemoChatTestAsyncDispatchPrivateNotifyNotDeliveredMessage(), "private message notify not delivered");
    EXPECT_STREQ(MemoChatTestAsyncDispatchDialogSyncPublishFailedEvent(), "chat.dialog_sync.publish_failed");
    EXPECT_STREQ(MemoChatTestAsyncDispatchPrivateDialogSyncPublishFailedMessage(),
                 "failed to publish private dialog sync event");
    EXPECT_STREQ(MemoChatTestAsyncDispatchGroupDialogSyncPublishFailedMessage(),
                 "failed to publish group dialog sync event");
    EXPECT_STREQ(MemoChatTestAsyncDispatchDialogSyncRefreshFailedEvent(), "chat.dialog_sync.refresh_failed");
    EXPECT_STREQ(MemoChatTestAsyncDispatchDialogSyncRefreshFailedMessage(), "failed to refresh dialog runtime");
    EXPECT_STREQ(MemoChatTestAsyncDispatchRelationStateRefreshFailedEvent(), "chat.relation_state.refresh_failed");
    EXPECT_STREQ(MemoChatTestAsyncDispatchRelationStateRefreshFailedMessage(),
                 "failed to refresh dialogs after relation event");
}
