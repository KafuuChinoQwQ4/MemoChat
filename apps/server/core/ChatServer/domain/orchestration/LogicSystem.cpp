#include "LogicSystem.hpp"

#include "ChatHandlerRegistrars.hpp"
#include "ChatRuntimeComposition.hpp"
#include "IChatSession.hpp"
#include "LogicNode.hpp"
#include "MessageDeliveryService.hpp"
#include "const.hpp"
#include "logging/Logger.hpp"
#include "logging/TraceContext.hpp"
#include "ports/ILogicSystemConfig.hpp"

#include <algorithm>
#include <exception>
#include <string>

// Worker count resolved by SetWorkerConfig() before the singleton is constructed.
// Falls back to kDefaultWorkerCount if SetWorkerConfig was never called.
static std::size_t s_configured_worker_count = LogicSystem::kDefaultWorkerCount;

void LogicSystem::SetWorkerConfig(const ILogicSystemConfig& cfg)
{
    s_configured_worker_count = cfg.WorkerCount(kDefaultWorkerCount, 1, kMaxWorkerCount);
}

namespace
{
void BindTcpTraceContext(const std::shared_ptr<IChatSession>& session, const std::string& msg_data)
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
            memolog::TraceContext::SetSessionId(session->sessionId());
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
        memolog::TraceContext::SetSessionId(session->sessionId());
    }
}

bool IsPreAuthMessage(short msg_id)
{
    return msg_id == MSG_CHAT_LOGIN || msg_id == ID_HEART_BEAT_REQ;
}

bool RejectUnauthenticatedBusinessMessage(const std::shared_ptr<IChatSession>& session, short msg_id)
{
    if (IsPreAuthMessage(msg_id))
    {
        return false;
    }
    if (session && session->userId() > 0)
    {
        return false;
    }
    memolog::LogWarn(
        "chat.message.unauthenticated",
        "reject unauthenticated chat business message",
        {{"msg_id", std::to_string(msg_id)}, {"session_id", session ? session->sessionId() : std::string()}});
    if (session)
    {
        session->close();
    }
    return true;
}

std::string NormalizeAuthenticatedCallerFields(const std::shared_ptr<IChatSession>& session,
                                               short msg_id,
                                               const std::string& msg_data)
{
    if (IsPreAuthMessage(msg_id) || !session || session->userId() <= 0)
    {
        return msg_data;
    }

    memochat::json::JsonReader reader;
    memochat::json::JsonValue root;
    if (!reader.parse(msg_data, root) || !root.isObject())
    {
        return msg_data;
    }

    const int session_uid = session->userId();
    bool changed = false;
    const char* caller_fields[] = {"uid", "fromuid", "owner_uid", "from_uid", "reviewer_uid", "viewer_uid"};
    for (const char* field : caller_fields)
    {
        if (!root.isMember(field))
        {
            continue;
        }
        const int payload_uid = root[field].asInt();
        if (payload_uid != session_uid)
        {
            memolog::LogWarn("chat.message.uid_mismatch",
                             "overrode client supplied uid with authenticated session uid",
                             {{"msg_id", std::to_string(msg_id)},
                              {"field", field},
                              {"payload_uid", std::to_string(payload_uid)},
                              {"session_uid", std::to_string(session_uid)}});
        }
        root[field] = session_uid;
        changed = true;
    }
    if (!changed)
    {
        return msg_data;
    }

    memochat::json::JsonStreamWriterBuilder builder;
    builder["indentation"] = "";
    return memochat::json::writeString(builder, root);
}
} // namespace

LogicSystem::LogicSystem()
    : _b_stop(false)
{
    _composition = std::make_unique<ChatRuntimeComposition>(*this);
    RegisterCallBacks();
    _num_workers = s_configured_worker_count;
    _worker_conds = std::make_unique<std::condition_variable[]>(_num_workers);
    for (size_t i = 0; i < _num_workers; ++i)
    {
        _worker_threads[i] = std::thread(&LogicSystem::workerLoop, this, i);
    }
    if (_composition)
    {
        _composition->StartDeliveryRuntimeIfEnabled();
    }
}

LogicSystem::~LogicSystem()
{
    _b_stop = true;
    if (_composition)
    {
        _composition->StopDeliveryRuntime();
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

void LogicSystem::PostMsgToQue(std::shared_ptr<LogicNode> msg)
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

void LogicSystem::SetServer(IChatSessionHost* host)
{
    _p_server = host;
}

MessageDeliveryService& LogicSystem::MessageDelivery()
{
    return _composition->MessageDelivery();
}

bool LogicSystem::PublishTask(const std::string& task_type,
                              const std::string& routing_key,
                              const memochat::json::JsonValue& payload,
                              int delay_ms,
                              int max_retries,
                              std::string* error)
{
    if (!_composition)
    {
        if (error)
        {
            *error = "chat_runtime_composition_unavailable";
        }
        return false;
    }
    return _composition->PublishTask(task_type, routing_key, payload, delay_ms, max_retries, error);
}

size_t LogicSystem::dispatchToWorker(const std::shared_ptr<LogicNode>& msg)
{
    const size_t worker_id = _next_worker.fetch_add(1, std::memory_order_relaxed) % _num_workers;
    return worker_id;
}

void LogicSystem::workerLoop(size_t worker_id)
{
    for (;;)
    {
        std::shared_ptr<LogicNode> msg_node;
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
        const std::string raw_msg_data(msg_node->_recvnode->_data, msg_node->_recvnode->_cur_len);
        if (RejectUnauthenticatedBusinessMessage(msg_node->_session, msg_node->_recvnode->_msg_id))
        {
            continue;
        }
        const std::string msg_data =
            NormalizeAuthenticatedCallerFields(msg_node->_session, msg_node->_recvnode->_msg_id, raw_msg_data);
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
    if (!_composition)
    {
        if (error)
        {
            *error = "chat_runtime_composition_unavailable";
        }
        return false;
    }
    return _composition->PublishAsyncEvent(topic, payload, error);
}

void LogicSystem::RegisterCallBacks()
{
    ChatSessionServiceRegistrar().Register(*_composition, _fun_callbacks);
    ChatRelationServiceRegistrar().Register(*_composition, _fun_callbacks);
    PrivateMessageServiceRegistrar().Register(*_composition, _fun_callbacks);
    GroupMessageServiceRegistrar().Register(*_composition, _fun_callbacks);
    AsyncEventDispatcherRegistrar().Register(*_composition, _fun_callbacks);
}
