#include "StatusAsyncSideEffects.h"
#include "json/GlazeCompat.h"

#include "ConfigMgr.h"
#include "RedisMgr.h"
#include "const.h"
#include "logging/Logger.h"
#include "logging/TraceContext.h"

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <chrono>
#ifndef MEMOCHAT_ENABLE_KAFKA
#define MEMOCHAT_ENABLE_KAFKA 0
#endif

#ifndef MEMOCHAT_ENABLE_RABBITMQ
#define MEMOCHAT_ENABLE_RABBITMQ 0
#endif

#if MEMOCHAT_ENABLE_KAFKA
#include <cppkafka/cppkafka.h>
#endif

#if MEMOCHAT_ENABLE_RABBITMQ
#include "../common/WinsockCompat.h"
#include <rabbitmq-c/amqp.h>
#include <rabbitmq-c/framing.h>
#include <rabbitmq-c/tcp_socket.h>
#endif

namespace {
int ParseIntOrStatus(const std::string& raw, int fallback)
{
    if (raw.empty()) {
        return fallback;
    }
    try {
        return std::stoi(raw);
    }
    catch (...) {
        return fallback;
    }
}

int64_t NowMsStatus()
{
    return static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
}

std::string WriteCompactJsonStatus(const memochat::json::JsonValue& value)
{
    return memochat::json::glaze_stringify(value);
}

#if MEMOCHAT_ENABLE_RABBITMQ
bool StatusRpcReplyOk(amqp_rpc_reply_t reply, std::string* error)
{
    if (reply.reply_type == AMQP_RESPONSE_NORMAL) {
        return true;
    }
    if (error) {
        *error = "amqp_rpc_reply_failed";
    }
    return false;
}

std::string BytesToStringStatus(amqp_bytes_t bytes)
{
    return std::string(static_cast<const char*>(bytes.bytes), bytes.len);
}
#endif
}

StatusAsyncSideEffects::StatusAsyncSideEffects(const std::unordered_set<std::string>& known_servers)
    : _known_servers(known_servers)
{
    auto& cfg = ConfigMgr::Inst();
    _kafka_brokers = cfg.GetValue("Kafka", "Brokers");
    _kafka_client_id = cfg.GetValue("Kafka", "ClientId");
    _rabbit_host = cfg.GetValue("RabbitMQ", "Host");
    _rabbit_port = ParseIntOrStatus(cfg.GetValue("RabbitMQ", "Port"), _rabbit_port);
    _rabbit_username = cfg.GetValue("RabbitMQ", "Username");
    _rabbit_password = cfg.GetValue("RabbitMQ", "Password");
    _rabbit_prefetch_count = ParseIntOrStatus(cfg.GetValue("RabbitMQ", "PrefetchCount"), _rabbit_prefetch_count);
    _rabbit_retry_delay_ms = ParseIntOrStatus(cfg.GetValue("RabbitMQ", "RetryDelayMs"), _rabbit_retry_delay_ms);
    _rabbit_max_retries = ParseIntOrStatus(cfg.GetValue("RabbitMQ", "MaxRetries"), _rabbit_max_retries);
    const auto vhost = cfg.GetValue("RabbitMQ", "VHost");
    if (!vhost.empty()) {
        _rabbit_vhost = vhost;
    }
    const auto exchange_direct = cfg.GetValue("RabbitMQ", "ExchangeDirect");
    if (!exchange_direct.empty()) {
        _rabbit_exchange_direct = exchange_direct;
    }
    const auto exchange_dlx = cfg.GetValue("RabbitMQ", "ExchangeDlx");
    if (!exchange_dlx.empty()) {
        _rabbit_exchange_dlx = exchange_dlx;
    }
}

StatusAsyncSideEffects::~StatusAsyncSideEffects()
{
    Stop();
}

void StatusAsyncSideEffects::Start()
{
    if (_worker.joinable()) {
        return;
    }
    _stop = false;
    _worker = std::thread([this]() {
        ConsumePresenceRefreshLoop();
    });
}

void StatusAsyncSideEffects::Stop()
{
    _stop = true;
    if (_worker.joinable()) {
        _worker.join();
    }
    CloseRabbit();
}

void StatusAsyncSideEffects::PublishAuditLogin(int uid,
    const std::string& server_name,
    const std::string& host,
    const std::string& port,
    const std::string& event_type)
{
    memochat::json::JsonValue payload(memochat::json::JsonValue{});
    payload["uid"] = uid;
    payload["server_name"] = server_name;
    payload["host"] = host;
    payload["port"] = port;
    std::string error;
    if (!PublishKafka("audit.login.v1", std::to_string(uid), event_type, payload, &error)) {
        memolog::LogWarn("status.side_effect.audit_publish_failed", "failed to publish audit login event",
            {{"uid", std::to_string(uid)}, {"event_type", event_type}, {"error", error}});
    }
}

void StatusAsyncSideEffects::PublishPresenceRefresh(int uid, const std::string& selected_server, const std::string& reason)
{
    memochat::json::JsonValue payload(memochat::json::JsonValue{});
    payload["uid"] = uid;
    payload["selected_server"] = selected_server;
    payload["reason"] = reason;
    std::string error;
    if (!PublishRabbit("status.presence.refresh", "status_presence_refresh", payload, &error)) {
        memolog::LogWarn("status.side_effect.presence_publish_failed", "failed to publish presence refresh task",
            {{"uid", std::to_string(uid)}, {"selected_server", selected_server}, {"reason", reason}, {"error", error}});
    }
}

bool StatusAsyncSideEffects::PublishKafka(const std::string& topic,
    const std::string& partition_key,
    const std::string& event_type,
    const memochat::json::JsonValue& payload,
    std::string* error)
{
#if MEMOCHAT_ENABLE_KAFKA
    if (_kafka_brokers.empty()) {
        if (error) {
            *error = "kafka_not_configured";
        }
        return false;
    }
    memochat::json::JsonValue envelope(memochat::json::JsonValue{});
    envelope["event_id"] = boost::uuids::to_string(boost::uuids::random_generator()());
    envelope["topic"] = topic;
    envelope["partition_key"] = partition_key;
    envelope["event_type"] = event_type;
    envelope["trace_id"] = memolog::TraceContext::GetTraceId();
    envelope["request_id"] = memolog::TraceContext::GetRequestId();
    envelope["created_at_ms"] = static_cast<int64_t>(NowMsStatus());
    envelope["retry_count"] = 0;
    envelope["payload"] = payload;
    try {
        cppkafka::Configuration config = {
            { "metadata.broker.list", _kafka_brokers },
            { "client.id", _kafka_client_id.empty() ? "memochat-status" : _kafka_client_id }
        };
        cppkafka::Producer producer(config);
        cppkafka::MessageBuilder builder(topic);
        const auto serialized = WriteCompactJsonStatus(envelope);
        builder.key(partition_key);
        builder.payload(serialized);
        producer.produce(builder);
        producer.flush();
        return true;
    }
    catch (const std::exception& ex) {
        if (error) {
            *error = ex.what();
        }
        return false;
    }
#else
    (void)topic;
    (void)partition_key;
    (void)event_type;
    (void)payload;
    if (error) {
        *error = "kafka_build_disabled";
    }
    return false;
#endif
}

bool StatusAsyncSideEffects::PublishRabbit(const std::string& routing_key,
    const std::string& task_type,
    const memochat::json::JsonValue& payload,
    std::string* error)
{
#if MEMOCHAT_ENABLE_RABBITMQ
    std::lock_guard<std::mutex> guard(_mutex);
    if (!EnsureRabbitConnected(error)) {
        return false;
    }
    memochat::json::JsonValue envelope(memochat::json::JsonValue{});
    envelope["task_id"] = boost::uuids::to_string(boost::uuids::random_generator()());
    envelope["task_type"] = task_type;
    envelope["trace_id"] = memolog::TraceContext::GetTraceId();
    envelope["request_id"] = memolog::TraceContext::GetRequestId();
    envelope["created_at_ms"] = static_cast<int64_t>(NowMsStatus());
    envelope["retry_count"] = 0;
    envelope["max_retries"] = _rabbit_max_retries;
    envelope["routing_key"] = routing_key;
    envelope["payload"] = payload;
    amqp_basic_properties_t props;
    props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
    props.content_type = amqp_cstring_bytes("application/json");
    props.delivery_mode = 2;
    const auto serialized = WriteCompactJsonStatus(envelope);
    const auto status = amqp_basic_publish(static_cast<amqp_connection_state_t>(_rabbit_connection), 1,
        amqp_cstring_bytes(_rabbit_exchange_direct.c_str()), amqp_cstring_bytes(routing_key.c_str()), 0, 0,
        &props, amqp_cstring_bytes(serialized.c_str()));
    if (status != AMQP_STATUS_OK) {
        if (error) {
            *error = "rabbitmq_publish_failed";
        }
        return false;
    }
    return true;
#else
    (void)routing_key;
    (void)task_type;
    (void)payload;
    if (error) {
        *error = "rabbitmq_build_disabled";
    }
    return false;
#endif
}

void StatusAsyncSideEffects::ConsumePresenceRefreshLoop()
{
#if MEMOCHAT_ENABLE_RABBITMQ
    bool consume_started = false;
    while (!_stop.load()) {
        std::string error;
        bool connected = false;
        {
            std::lock_guard<std::mutex> guard(_mutex);
            connected = EnsureRabbitConnected(&error);
        }
        if (!connected) {
            if (error == "rabbitmq_not_configured") {
                memolog::LogInfo("status.presence_worker", "RabbitMQ not configured, presence worker disabled");
                break;
            }
            memolog::LogWarn("status.presence_worker.connect_failed", "failed to connect rabbitmq presence worker",
                {{"error", error}});
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            continue;
        }
        if (!_rabbit_connection) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            continue;
        }

        if (!consume_started) {
            amqp_basic_consume(static_cast<amqp_connection_state_t>(_rabbit_connection), 1,
                amqp_cstring_bytes("status.presence.refresh.q"), amqp_empty_bytes, 0, 0, 0, amqp_empty_table);
            amqp_get_rpc_reply(static_cast<amqp_connection_state_t>(_rabbit_connection));
            consume_started = true;
        }

        timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000;
        amqp_envelope_t envelope;
        amqp_maybe_release_buffers(static_cast<amqp_connection_state_t>(_rabbit_connection));
        const auto reply = amqp_consume_message(static_cast<amqp_connection_state_t>(_rabbit_connection), &envelope, &timeout, 0);
        if (reply.reply_type != AMQP_RESPONSE_NORMAL) {
            continue;
        }

        memochat::json::JsonValue root;
        const auto serialized = BytesToStringStatus(envelope.message.body);
        if (memochat::json::glaze_parse(root, serialized) && memochat::json::glaze_is_object(root)) {
            HandlePresenceRefresh(root["payload"]);
        }
        amqp_basic_ack(static_cast<amqp_connection_state_t>(_rabbit_connection), 1, envelope.delivery_tag, false);
        amqp_destroy_envelope(&envelope);
    }
#endif
}

bool StatusAsyncSideEffects::EnsureRabbitConnected(std::string* error)
{
#if MEMOCHAT_ENABLE_RABBITMQ
    if (_rabbit_connection) {
        return true;
    }
    if (_rabbit_host.empty()) {
        if (error) {
            *error = "rabbitmq_not_configured";
        }
        return false;
    }
    auto connection = amqp_new_connection();
    auto* socket = amqp_tcp_socket_new(connection);
    if (!socket) {
        if (error) {
            *error = "rabbitmq_socket_create_failed";
        }
        amqp_destroy_connection(connection);
        return false;
    }
    if (amqp_socket_open(socket, _rabbit_host.c_str(), _rabbit_port) != AMQP_STATUS_OK) {
        if (error) {
            *error = "rabbitmq_socket_open_failed";
        }
        amqp_destroy_connection(connection);
        return false;
    }
    if (!StatusRpcReplyOk(amqp_login(connection, _rabbit_vhost.c_str(), 0, 131072, 0, AMQP_SASL_METHOD_PLAIN,
        _rabbit_username.c_str(), _rabbit_password.c_str()), error)) {
        amqp_destroy_connection(connection);
        return false;
    }
    amqp_channel_open(connection, 1);
    if (!StatusRpcReplyOk(amqp_get_rpc_reply(connection), error)) {
        amqp_connection_close(connection, AMQP_REPLY_SUCCESS);
        amqp_destroy_connection(connection);
        return false;
    }
    amqp_basic_qos(connection, 1, 0, static_cast<uint16_t>(_rabbit_prefetch_count), false);
    StatusRpcReplyOk(amqp_get_rpc_reply(connection), error);
    _rabbit_connection = connection;
    return EnsureRabbitTopology(error);
#else
    (void)error;
    return false;
#endif
}

bool StatusAsyncSideEffects::EnsureRabbitTopology(std::string* error)
{
#if MEMOCHAT_ENABLE_RABBITMQ
    auto connection = static_cast<amqp_connection_state_t>(_rabbit_connection);
    amqp_exchange_declare(connection, 1, amqp_cstring_bytes(_rabbit_exchange_direct.c_str()),
        amqp_cstring_bytes("direct"), 0, 1, 0, 0, amqp_empty_table);
    if (!StatusRpcReplyOk(amqp_get_rpc_reply(connection), error)) {
        return false;
    }
    amqp_exchange_declare(connection, 1, amqp_cstring_bytes(_rabbit_exchange_dlx.c_str()),
        amqp_cstring_bytes("direct"), 0, 1, 0, 0, amqp_empty_table);
    if (!StatusRpcReplyOk(amqp_get_rpc_reply(connection), error)) {
        return false;
    }

    amqp_queue_declare(connection, 1, amqp_cstring_bytes("status.presence.refresh.q"), 0, 1, 0, 0, amqp_empty_table);
    if (!StatusRpcReplyOk(amqp_get_rpc_reply(connection), error)) {
        return false;
    }
    amqp_queue_bind(connection, 1, amqp_cstring_bytes("status.presence.refresh.q"),
        amqp_cstring_bytes(_rabbit_exchange_direct.c_str()), amqp_cstring_bytes("status.presence.refresh"), amqp_empty_table);
    if (!StatusRpcReplyOk(amqp_get_rpc_reply(connection), error)) {
        return false;
    }
    return true;
#else
    (void)error;
    return false;
#endif
}

void StatusAsyncSideEffects::CloseRabbit()
{
#if MEMOCHAT_ENABLE_RABBITMQ
    std::lock_guard<std::mutex> guard(_mutex);
    if (_rabbit_connection) {
        auto connection = static_cast<amqp_connection_state_t>(_rabbit_connection);
        amqp_channel_close(connection, 1, AMQP_REPLY_SUCCESS);
        amqp_connection_close(connection, AMQP_REPLY_SUCCESS);
        amqp_destroy_connection(connection);
        _rabbit_connection = nullptr;
    }
#endif
}

void StatusAsyncSideEffects::HandlePresenceRefresh(const memochat::json::JsonValue& payload)
{
    if (!memochat::json::glaze_is_object(payload)) {
        return;
    }
    const int uid = memochat::json::glaze_safe_get<int>(payload, "uid", 0);
    const auto selected_server = memochat::json::glaze_safe_get<std::string>(payload, "selected_server", "");
    if (uid <= 0) {
        return;
    }
    if (!_known_servers.empty() && !selected_server.empty() && _known_servers.find(selected_server) == _known_servers.end()) {
        const auto uid_str = std::to_string(uid);
        RedisMgr::GetInstance()->Del(USERIPPREFIX + uid_str);
        memolog::LogWarn("status.presence_refresh.stale_route", "removed stale presence route",
            {{"uid", uid_str}, {"selected_server", selected_server}});
        return;
    }
    memolog::LogInfo("status.presence_refresh.processed", "processed presence refresh task",
        {{"uid", std::to_string(uid)}, {"selected_server", selected_server}, {"reason", memochat::json::glaze_safe_get<std::string>(payload, "reason", "")}});
}

