#include "ChatRuntimeComposition.h"

#include "AsyncEventDispatcher.h"
#include "ChatDeliveryRuntime.h"
#include "ChatMessageRepository.h"
#include "ChatRelationRepository.h"
#include "ChatRelationSessionAdapter.h"
#include "ChatRuntime.h"
#include "ChatSessionConfig.h"
#include "ChatSessionRepository.h"
#include "ChatSessionService.h"
#include "InlineTaskBus.h"
#include "KafkaAsyncEventBus.h"
#include "LogicSystem.h"
#include "MessageDeliveryService.h"
#include "MessageServiceConfig.h"
#include "MessageServiceFactory.h"
#include "RabbitMqTaskBus.h"
#include "RedisAsyncEventBus.h"
#include "RedisOnlineRouteStore.h"
#include "RedisRelationBootstrapCache.h"
#include "RelationQueryServiceConfig.h"
#include "RelationQueryServiceFactory.h"
#include "RelationServiceConfig.h"
#include "RelationServiceFactory.h"
#include "TaskDispatcher.h"
#include "UserMgr.h"
#include "logging/Logger.h"

ChatRuntimeComposition::ChatRuntimeComposition(LogicSystem& logic)
    : _logic(logic)
{
    _online_route_store = std::make_unique<RedisOnlineRouteStore>();
    _session_config = std::make_unique<ChatSessionConfig>();
    _session_repository = std::make_unique<ChatSessionRepository>();
    _relation_repository = std::make_unique<ChatRelationRepository>();
    _relation_bootstrap_cache = std::make_unique<RedisRelationBootstrapCache>();
    _relation_query_service_config = std::make_unique<RelationQueryServiceConfig>();
    _relation_service_config = std::make_unique<RelationServiceConfig>();
    _message_service_config = std::make_unique<MessageServiceConfig>();
    _message_repository = std::make_unique<ChatMessageRepository>();

    const auto task_bus_backend = memochat::chatruntime::TaskBusBackend();
    if (task_bus_backend == "rabbitmq" && RabbitMqTaskBus::BuildAvailable())
    {
        _task_bus = std::make_shared<RabbitMqTaskBus>();
        memolog::LogInfo("chat.task_bus.rabbitmq",
                         "chat task bus backend selected",
                         {{"configured_backend", task_bus_backend}});
    }
    else
    {
        if (task_bus_backend == "rabbitmq" && !RabbitMqTaskBus::BuildAvailable())
        {
            memolog::LogWarn("chat.task_bus.rabbitmq_unavailable",
                             "chat task bus rabbitmq backend is not available in current build, fallback to inline",
                             {{"configured_backend", task_bus_backend}, {"fallback_backend", "inline"}});
        }
        else if (task_bus_backend != "inline")
        {
            memolog::LogWarn("chat.task_bus.unsupported_backend",
                             "task bus backend is not implemented yet, fallback to inline",
                             {{"configured_backend", task_bus_backend}, {"fallback_backend", "inline"}});
        }
        _task_bus = std::make_shared<InlineTaskBus>();
    }

    const auto async_event_bus_backend = memochat::chatruntime::AsyncEventBusBackend();
    if (async_event_bus_backend == "kafka" && KafkaAsyncEventBus::BuildAvailable())
    {
        _async_event_bus = std::make_shared<KafkaAsyncEventBus>(
            [this](int64_t outbox_id, int delay_ms, int max_retries, std::string* error)
            {
                memochat::json::JsonValue payload(memochat::json::object_t{});
                payload["outbox_id"] = static_cast<int64_t>(outbox_id);
                return PublishTask("outbox_repair",
                                   memochat::chatruntime::TaskRoutingOutboxRepair(),
                                   payload,
                                   delay_ms,
                                   max_retries,
                                   error);
            });
        memolog::LogInfo("chat.async_event_bus.kafka",
                         "chat async event bus backend selected",
                         {{"configured_backend", async_event_bus_backend}});
    }
    else
    {
        if (async_event_bus_backend == "kafka" && !KafkaAsyncEventBus::BuildAvailable())
        {
            memolog::LogWarn("chat.async_event_bus.kafka_unavailable",
                             "chat async event bus kafka backend is not available in current build, fallback to redis",
                             {{"configured_backend", async_event_bus_backend}, {"fallback_backend", "redis"}});
        }
        else if (async_event_bus_backend != "redis")
        {
            memolog::LogWarn("chat.async_event_bus.unsupported_backend",
                             "async event bus backend is not implemented yet, fallback to redis",
                             {{"configured_backend", async_event_bus_backend}, {"fallback_backend", "redis"}});
        }
        _async_event_bus = std::make_shared<RedisAsyncEventBus>();
    }

    _message_delivery_service =
        std::make_unique<MessageDeliveryService>(nullptr, UserMgr::GetInstance().get(), _online_route_store.get());
    _async_event_dispatcher = std::make_unique<AsyncEventDispatcher>(
        _async_event_bus,
        [this]()
        {
            return DeliveryRuntimeStopRequested();
        },
        _message_delivery_service.get(),
        _message_repository.get(),
        _relation_repository.get(),
        UserMgr::GetInstance().get(),
        _online_route_store.get(),
        _relation_bootstrap_cache.get());
    _task_dispatcher = std::make_unique<TaskDispatcher>(
        _task_bus,
        [this]()
        {
            return DeliveryRuntimeStopRequested();
        },
        _message_delivery_service.get(),
        &_logic);
    _message_delivery_service->SetTaskPublisher(_task_dispatcher.get());

    _private_message_service = CreatePrivateMessageService(*_message_service_config,
                                                           UserMgr::GetInstance().get(),
                                                           _online_route_store.get(),
                                                           _message_repository.get(),
                                                           _relation_repository.get(),
                                                           _message_delivery_service.get(),
                                                           _async_event_dispatcher.get());
    _group_message_service = CreateGroupMessageService(*_message_service_config,
                                                       _message_repository.get(),
                                                       _relation_repository.get(),
                                                       _message_delivery_service.get(),
                                                       _async_event_dispatcher.get());
    _chat_relation_service = CreateRelationService(*_relation_service_config,
                                                   _relation_repository.get(),
                                                   _relation_bootstrap_cache.get(),
                                                   _message_delivery_service.get(),
                                                   _task_dispatcher.get(),
                                                   _async_event_dispatcher.get());
    _chat_relation_session_service = std::make_unique<ChatRelationSessionAdapter>(_chat_relation_service.get());

    auto* relation_query_service = SelectRelationQueryService(*_relation_query_service_config,
                                                              _chat_relation_service.get(),
                                                              _relation_query_service_remote);
    _chat_session_service = std::make_unique<ChatSessionService>(_logic,
                                                                 UserMgr::GetInstance().get(),
                                                                 _online_route_store.get(),
                                                                 relation_query_service,
                                                                 _relation_repository.get(),
                                                                 _session_config.get(),
                                                                 _session_repository.get());
    _delivery_runtime = std::make_unique<ChatDeliveryRuntime>(
        [this]()
        {
            DealAsyncEvents();
        },
        [this]()
        {
            DealTasks();
        });
}

ChatRuntimeComposition::~ChatRuntimeComposition()
{
    StopDeliveryRuntime();
}

ChatSessionService& ChatRuntimeComposition::SessionService()
{
    return *_chat_session_service;
}

IRelationSessionService& ChatRuntimeComposition::RelationSessionService()
{
    return *_chat_relation_session_service;
}

IPrivateMessageService& ChatRuntimeComposition::PrivateMessageService()
{
    return *_private_message_service;
}

IGroupMessageService& ChatRuntimeComposition::GroupMessageService()
{
    return *_group_message_service;
}

MessageDeliveryService& ChatRuntimeComposition::MessageDelivery()
{
    return *_message_delivery_service;
}

bool ChatRuntimeComposition::PublishTask(const std::string& task_type,
                                         const std::string& routing_key,
                                         const memochat::json::JsonValue& payload,
                                         int delay_ms,
                                         int max_retries,
                                         std::string* error)
{
    if (!_task_dispatcher)
    {
        if (error)
        {
            *error = "task_dispatcher_unavailable";
        }
        return false;
    }
    return _task_dispatcher->PublishTask(task_type, routing_key, payload, delay_ms, max_retries, error);
}

bool ChatRuntimeComposition::PublishAsyncEvent(const std::string& topic,
                                               const memochat::json::JsonValue& payload,
                                               std::string* error)
{
    if (!_async_event_dispatcher)
    {
        if (error)
        {
            *error = "async_event_dispatcher_unavailable";
        }
        return false;
    }
    return _async_event_dispatcher->PublishEvent(topic, payload, error);
}

void ChatRuntimeComposition::DealTasks()
{
    if (_task_dispatcher)
    {
        _task_dispatcher->DealTasks();
    }
}

void ChatRuntimeComposition::DealAsyncEvents()
{
    if (_async_event_dispatcher)
    {
        _async_event_dispatcher->DealAsyncEvents();
    }
}

void ChatRuntimeComposition::StartDeliveryRuntimeIfEnabled()
{
    if (_delivery_runtime && memochat::chatruntime::IsWorkerEnabled())
    {
        _delivery_runtime->Start();
    }
}

void ChatRuntimeComposition::StopDeliveryRuntime()
{
    if (_delivery_runtime)
    {
        _delivery_runtime->StopAndJoin();
    }
}

bool ChatRuntimeComposition::DeliveryRuntimeStopRequested() const
{
    return !_delivery_runtime || _delivery_runtime->StopRequested();
}
