#include "LogicSystem.h"
#include "ConfigMgr.h"
#include "PostgresMgr.h"
#include "const.h"
#include "UserMgr.h"
#include "ChatMessageRepository.h"
#include "MessageServiceConfig.h"
#include "ChatRelationRepository.h"
#include "RelationQueryServiceConfig.h"
#include "RelationQueryServiceFactory.h"
#include "RelationServiceConfig.h"
#include "ChatSessionConfig.h"
#include "ChatSessionRepository.h"
#include "RedisRelationBootstrapCache.h"
#include "ChatRuntime.h"
#include "ChatHandlerRegistrars.h"
#include "AsyncEventDispatcher.h"
#include "InlineTaskBus.h"
#include "KafkaAsyncEventBus.h"
#include "RedisAsyncEventBus.h"
#include "RabbitMqTaskBus.h"
#include "ChatDeliveryRuntime.h"
#include "RelationServiceFactory.h"
#include "ChatRelationSessionAdapter.h"
#include "ChatSessionService.h"
#include "MessageServiceFactory.h"
#include "MessageDeliveryService.h"
#include "TaskDispatcher.h"
#include "RedisOnlineRouteStore.h"
#include "logging/Logger.h"
#include "logging/TraceContext.h"
#include <string>
#include <algorithm>
#include "CServer.h"
using namespace std;

namespace
{
size_t ConfigSizeLocal(const std::string& section,
                       const std::string& key,
                       size_t default_value,
                       size_t min_value,
                       size_t max_value)
{
    const auto raw = ConfigMgr::Inst().GetValue(section, key);
    if (raw.empty())
    {
        return default_value;
    }
    try
    {
        const auto value = static_cast<size_t>(std::stoul(raw));
        return std::clamp(value, min_value, max_value);
    }
    catch (...)
    {
        return default_value;
    }
}

void BindTcpTraceContext(const std::shared_ptr<CSession>& session, const std::string& msg_data)
{
    memochat::json::JsonReader reader;
    memochat::json::JsonValue root;
    if (!reader.parse(msg_data, root) || !root.isObject())
    {
        memolog::TraceContext::SetTraceId(memolog::TraceContext::NewId());
        memolog::TraceContext::SetRequestId(memolog::TraceContext::NewId());
        memolog::TraceContext::SetSpanId("");
        if (session)
        {
            memolog::TraceContext::SetSessionId(session->GetSessionId());
        }
        return;
    }

    std::string trace_id = root.get("trace_id", "").asString();
    if (trace_id.empty())
    {
        trace_id = memolog::TraceContext::NewId();
    }
    std::string request_id = root.get("request_id", "").asString();
    if (request_id.empty())
    {
        request_id = memolog::TraceContext::NewId();
    }
    memolog::TraceContext::SetTraceId(trace_id);
    memolog::TraceContext::SetRequestId(request_id);
    memolog::TraceContext::SetSpanId(root.get("span_id", "").asString());
    if (root.isMember("uid"))
    {
        memolog::TraceContext::SetUid(std::to_string(root["uid"].asInt()));
    }
    else if (root.isMember("fromuid"))
    {
        memolog::TraceContext::SetUid(std::to_string(root["fromuid"].asInt()));
    }
    if (session)
    {
        memolog::TraceContext::SetSessionId(session->GetSessionId());
    }
}
} // namespace

LogicSystem::LogicSystem()
    : _b_stop(false)
    , _p_server(nullptr)
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
            return _delivery_runtime && _delivery_runtime->StopRequested();
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
            return _delivery_runtime && _delivery_runtime->StopRequested();
        },
        _message_delivery_service.get(),
        this);
    _message_delivery_service->SetTaskPublisher(_task_dispatcher.get());
    _private_message_service = CreatePrivateMessageService(*_message_service_config,
                                                           UserMgr::GetInstance().get(),
                                                           _online_route_store.get(),
                                                           _message_repository.get(),
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
    _chat_session_service = std::make_unique<ChatSessionService>(*this,
                                                                 UserMgr::GetInstance().get(),
                                                                 _online_route_store.get(),
                                                                 relation_query_service,
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
    RegisterCallBacks();
    _num_workers = ConfigSizeLocal("LogicSystem", "WorkerCount", kDefaultWorkerCount, 1, kMaxWorkerCount);
    _worker_conds = std::make_unique<std::condition_variable[]>(_num_workers);
    for (size_t i = 0; i < _num_workers; ++i)
    {
        _worker_threads[i] = std::thread(&LogicSystem::workerLoop, this, i);
    }
    if (memochat::chatruntime::IsWorkerEnabled())
    {
        _delivery_runtime->Start();
    }
}

LogicSystem::~LogicSystem()
{
    _b_stop = true;
    if (_delivery_runtime)
    {
        _delivery_runtime->StopAndJoin();
    }
    for (size_t i = 0; i < _num_workers; ++i)
    {
        _worker_conds[i].notify_one();
    }
    for (size_t i = 0; i < _num_workers; ++i)
    {
        if (_worker_threads[i].joinable())
        {
            _worker_threads[i].join();
        }
    }
}

void LogicSystem::PostMsgToQue(shared_ptr<LogicNode> msg)
{
    if (_msg_que_size.load(std::memory_order_relaxed) >= MAX_MSG_QUE_SIZE)
    {
        uint64_t dropped = ++_msg_dropped_total;
        uint64_t counter = ++_backpressure_log_counter;
        if (counter % BACKPRESSURE_DROP_LOG_INTERVAL == 1)
        {
            memolog::LogWarn("logic.msg_que.backpressure",
                             "LogicSystem message queue backpressure, messages dropped",
                             {{"dropped_total", std::to_string(dropped)},
                              {"queue_size", std::to_string(_msg_que_size.load(std::memory_order_relaxed))},
                              {"counter", std::to_string(counter)}});
        }
        return;
    }
    size_t worker_id = dispatchToWorker(msg);
    {
        std::lock_guard<std::mutex> lock(_worker_mutexes[worker_id]);
        _worker_queues[worker_id].push(msg);
        ++_msg_que_size;
        _worker_conds[worker_id].notify_one();
    }
}

void LogicSystem::SetServer(std::shared_ptr<CServer> pserver)
{
    _p_server = pserver;
}

MessageDeliveryService& LogicSystem::MessageDelivery()
{
    return *_message_delivery_service;
}

bool LogicSystem::PublishTask(const std::string& task_type,
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

bool LogicSystem::ExpediteOutboxRepair(int64_t outbox_id)
{
    return PostgresMgr::GetInstance()->ExpediteChatOutboxEventRetry(outbox_id);
}

size_t LogicSystem::dispatchToWorker(const shared_ptr<LogicNode>& msg)
{
    const size_t worker_id = _next_worker.fetch_add(1, std::memory_order_relaxed) % _num_workers;
    return worker_id;
}

void LogicSystem::workerLoop(size_t worker_id)
{
    for (;;)
    {
        shared_ptr<LogicNode> msg_node;
        {
            std::unique_lock<std::mutex> lock(_worker_mutexes[worker_id]);
            _worker_conds[worker_id].wait(lock,
                                          [this, worker_id]()
                                          {
                                              return _b_stop || !_worker_queues[worker_id].empty();
                                          });
            if (_b_stop && _worker_queues[worker_id].empty())
            {
                break;
            }
            msg_node = std::move(_worker_queues[worker_id].front());
            _worker_queues[worker_id].pop();
            --_msg_que_size;
        }

        if (!msg_node || !msg_node->_recvnode)
        {
            continue;
        }

        auto call_back_iter = _fun_callbacks.find(msg_node->_recvnode->_msg_id);
        if (call_back_iter == _fun_callbacks.end())
        {
            continue;
        }
        const std::string msg_data(msg_node->_recvnode->_data, msg_node->_recvnode->_cur_len);
        BindTcpTraceContext(msg_node->_session, msg_data);
        Defer clear_trace(
            []()
            {
                memolog::TraceContext::Clear();
            });
        try
        {
            call_back_iter->second(msg_node->_session, msg_node->_recvnode->_msg_id, msg_data);
        }
        catch (const std::exception& e)
        {
            memolog::LogError("logic.callback.exception",
                              "LogicSystem callback threw exception",
                              {{"msg_id", std::to_string(msg_node->_recvnode->_msg_id)}, {"error", e.what()}});
        }
        catch (...)
        {
            memolog::LogError("logic.callback.exception",
                              "LogicSystem callback threw unknown exception",
                              {{"msg_id", std::to_string(msg_node->_recvnode->_msg_id)}});
        }
    }
}

bool LogicSystem::PublishAsyncEvent(const std::string& topic,
                                    const memochat::json::JsonValue& payload,
                                    std::string* error)
{
    return _async_event_dispatcher->PublishEvent(topic, payload, error);
}

void LogicSystem::DealTasks()
{
    if (_task_dispatcher)
    {
        _task_dispatcher->DealTasks();
    }
}

void LogicSystem::DealAsyncEvents()
{
    _async_event_dispatcher->DealAsyncEvents();
}

void LogicSystem::RegisterCallBacks()
{
    ChatSessionServiceRegistrar().Register(*this, _fun_callbacks);
    ChatRelationServiceRegistrar().Register(*this, _fun_callbacks);
    PrivateMessageServiceRegistrar().Register(*this, _fun_callbacks);
    GroupMessageServiceRegistrar().Register(*this, _fun_callbacks);
    AsyncEventDispatcherRegistrar().Register(*this, _fun_callbacks);
}
