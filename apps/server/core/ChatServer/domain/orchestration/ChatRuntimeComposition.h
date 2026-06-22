#pragma once

#include "json/GlazeCompat.h"

#include <memory>
#include <string>

class AsyncEventDispatcher;
class ChatDeliveryRuntime;
class ChatSessionService;
class IAsyncEventBus;
class IAsyncTaskBus;
class IChatSessionConfig;
class IChatSessionRepository;
class IGroupMessageService;
class IMessageRepository;
class IMessageServiceConfig;
class IOnlineRouteStore;
class IPrivateMessageService;
class IRelationBootstrapCache;
class IRelationQueryService;
class IRelationQueryServiceConfig;
class IRelationRepository;
class IRelationService;
class IRelationServiceConfig;
class IRelationSessionService;
class LogicSystem;
class MessageDeliveryService;
class TaskDispatcher;

class ChatRuntimeComposition
{
public:
    explicit ChatRuntimeComposition(LogicSystem& logic);
    ~ChatRuntimeComposition();

    ChatRuntimeComposition(const ChatRuntimeComposition&) = delete;
    ChatRuntimeComposition& operator=(const ChatRuntimeComposition&) = delete;

    ChatSessionService& SessionService();
    IRelationSessionService& RelationSessionService();
    IPrivateMessageService& PrivateMessageService();
    IGroupMessageService& GroupMessageService();
    MessageDeliveryService& MessageDelivery();

    bool PublishTask(const std::string& task_type,
                     const std::string& routing_key,
                     const memochat::json::JsonValue& payload,
                     int delay_ms,
                     int max_retries,
                     std::string* error = nullptr);
    bool
    PublishAsyncEvent(const std::string& topic, const memochat::json::JsonValue& payload, std::string* error = nullptr);
    void DealTasks();
    void DealAsyncEvents();
    void StartDeliveryRuntimeIfEnabled();
    void StopDeliveryRuntime();

private:
    bool DeliveryRuntimeStopRequested() const;

    LogicSystem& _logic;

    std::unique_ptr<IOnlineRouteStore> _online_route_store;
    std::unique_ptr<IChatSessionConfig> _session_config;
    std::unique_ptr<IChatSessionRepository> _session_repository;
    std::unique_ptr<IMessageServiceConfig> _message_service_config;
    std::unique_ptr<IMessageRepository> _message_repository;
    std::unique_ptr<IRelationRepository> _relation_repository;
    std::unique_ptr<IRelationBootstrapCache> _relation_bootstrap_cache;
    std::unique_ptr<IRelationQueryServiceConfig> _relation_query_service_config;
    std::unique_ptr<IRelationServiceConfig> _relation_service_config;
    std::shared_ptr<IAsyncTaskBus> _task_bus;
    std::shared_ptr<IAsyncEventBus> _async_event_bus;

    std::unique_ptr<MessageDeliveryService> _message_delivery_service;
    std::unique_ptr<TaskDispatcher> _task_dispatcher;
    std::unique_ptr<AsyncEventDispatcher> _async_event_dispatcher;
    std::unique_ptr<IRelationService> _chat_relation_service;
    std::unique_ptr<IRelationSessionService> _chat_relation_session_service;
    std::unique_ptr<IRelationQueryService> _relation_query_service_remote;
    std::unique_ptr<IPrivateMessageService> _private_message_service;
    std::unique_ptr<IGroupMessageService> _group_message_service;
    std::unique_ptr<ChatSessionService> _chat_session_service;
    std::unique_ptr<ChatDeliveryRuntime> _delivery_runtime;
};
