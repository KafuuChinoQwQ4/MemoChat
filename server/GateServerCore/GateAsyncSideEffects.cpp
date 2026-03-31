#include "GateAsyncSideEffects.h"

#include "AuthLoginSupport.h"
#include "ConfigMgr.h"
#include "logging/Logger.h"
#include "logging/TraceContext.h"

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <chrono>
#include <json/json.h>
#include <mutex>

#ifndef MEMOCHAT_ENABLE_KAFKA
#define MEMOCHAT_ENABLE_KAFKA 0
#endif

#ifndef MEMOCHAT_ENABLE_RABBITMQ
#define MEMOCHAT_ENABLE_RABBITMQ 0
#endif

#if MEMOCHAT_ENABLE_KAFKA
#include <cppkafka/cppkafka.h>
#endif

#include <memory>

#if MEMOCHAT_ENABLE_RABBITMQ
#include <winsock2.h>
#include <rabbitmq-c/amqp.h>
#include <rabbitmq-c/framing.h>
#include <rabbitmq-c/tcp_socket.h>
#endif

namespace {
int ParseIntOrGate(const std::string& raw, int fallback)
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

int64_t NowMsGate()
{
    return static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
}

std::string WriteCompactJsonGate(const Json::Value& value)
{
    Json::StreamWriterBuilder builder;
    builder["indentation"] = "";
    return Json::writeString(builder, value);
}

#if MEMOCHAT_ENABLE_RABBITMQ
std::string BytesToStringGate(amqp_bytes_t bytes)
{
    return std::string(static_cast<const char*>(bytes.bytes), bytes.len);
}

bool GateRpcReplyOk(amqp_rpc_reply_t reply, std::string* error)
{
    if (reply.reply_type == AMQP_RESPONSE_NORMAL) {
        return true;
    }
    if (error) {
        *error = "amqp_rpc_reply_failed";
    }
    return false;
}
#endif
}

GateAsyncSideEffects& GateAsyncSideEffects::Instance()
{
    static GateAsyncSideEffects instance;
    return instance;
}

GateAsyncSideEffects::GateAsyncSideEffects()
{
    auto& cfg = ConfigMgr::Inst();
    _kafka_brokers = cfg.GetValue("Kafka", "Brokers");
    _kafka_client_id = cfg.GetValue("Kafka", "ClientId");
    _rabbit_host = cfg.GetValue("RabbitMQ", "Host");
    _rabbit_port = ParseIntOrGate(cfg.GetValue("RabbitMQ", "Port"), _rabbit_port);
    _rabbit_username = cfg.GetValue("RabbitMQ", "Username");
    _rabbit_password = cfg.GetValue("RabbitMQ", "Password");
    _rabbit_prefetch_count = ParseIntOrGate(cfg.GetValue("RabbitMQ", "PrefetchCount"), _rabbit_prefetch_count);
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

GateAsyncSideEffects::~GateAsyncSideEffects()
{
    Stop();
}

void GateAsyncSideEffects::Start()
{
    if (_worker.joinable()) {
        return;
    }
    _stop = false;
    _worker = std::thread([this]() {
        ConsumeCacheInvalidateLoop();
    });
}

void GateAsyncSideEffects::Stop()
{
    _stop = true;
    if (_worker.joinable()) {
        _worker.join();
    }
    CloseKafka();
    CloseRabbit();
}

void GateAsyncSideEffects::PublishUserProfileChanged(int uid,
    const std::string& user_id,
    const std::string& email,
    const std::string& name,
    const std::string& nick,
    const std::string& icon,
    int sex)
{
    Json::Value payload(Json::objectValue);
    payload["uid"] = uid;
    payload["user_id"] = user_id;
    payload["email"] = email;
    payload["name"] = name;
    payload["nick"] = nick;
    payload["icon"] = icon;
    payload["sex"] = sex;
    std::string error;
    if (!PublishKafka("user.profile.changed.v1", std::to_string(uid), "user_profile_changed", payload, &error)) {
        memolog::LogWarn("gate.side_effect.profile_publish_failed", "failed to publish user profile change",
            {{"uid", std::to_string(uid)}, {"error", error}});
    }
}

void GateAsyncSideEffects::PublishAuditLogin(int uid,
    const std::string& user_id,
    const std::string& email,
    const std::string& chat_server,
    const std::string& chat_host,
    const std::string& chat_port,
    bool login_cache_hit)
{
    Json::Value payload(Json::objectValue);
    payload["uid"] = uid;
    payload["user_id"] = user_id;
    payload["email"] = email;
    payload["chat_server"] = chat_server;
    payload["chat_host"] = chat_host;
    payload["chat_port"] = chat_port;
    payload["login_cache_hit"] = login_cache_hit;
    std::string error;
    if (!PublishKafka("audit.login.v1", std::to_string(uid), "gate_login_succeeded", payload, &error)) {
        memolog::LogWarn("gate.side_effect.audit_publish_failed", "failed to publish login audit event",
            {{"uid", std::to_string(uid)}, {"error", error}});
    }
}

void GateAsyncSideEffects::PublishCacheInvalidate(const std::string& email, const std::string& user_name, const std::string& reason)
{
    Json::Value payload(Json::objectValue);
    payload["email"] = email;
    payload["user_name"] = user_name;
    payload["reason"] = reason;
    payload["cache_domain"] = "login_profile";
    std::string error;
    if (!PublishRabbit("gate.cache.invalidate", "gate_cache_invalidate", payload, &error)) {
        memolog::LogWarn("gate.side_effect.cache_invalidate_publish_failed", "failed to publish cache invalidate task",
            {{"email", email}, {"reason", reason}, {"error", error}});
        return;
    }
    memolog::LogInfo("gate.side_effect.cache_invalidate_published", "published cache invalidate task",
        {{"email", email}, {"reason", reason}});
}

bool GateAsyncSideEffects::PublishKafka(const std::string& topic,
    const std::string& partition_key,
    const std::string& event_type,
    const Json::Value& payload,
    std::string* error)
{
#if MEMOCHAT_ENABLE_KAFKA
    if (_kafka_brokers.empty()) {
        if (error) {
            *error = "kafka_not_configured";
        }
        return false;
    }
    Json::Value envelope(Json::objectValue);
    envelope["event_id"] = boost::uuids::to_string(boost::uuids::random_generator()());
    envelope["topic"] = topic;
    envelope["partition_key"] = partition_key;
    envelope["event_type"] = event_type;
    envelope["trace_id"] = memolog::TraceContext::GetTraceId();
    envelope["request_id"] = memolog::TraceContext::GetRequestId();
    envelope["created_at_ms"] = static_cast<Json::Int64>(NowMsGate());
    envelope["retry_count"] = 0;
    envelope["payload"] = payload;
    const auto serialized = WriteCompactJsonGate(envelope);
    try {
        std::lock_guard<std::mutex> guard(_kafka_mutex);
        if (!_kafka_producer) {
            cppkafka::Configuration config = {
                { "metadata.broker.list", _kafka_brokers },
                { "client.id", _kafka_client_id.empty() ? "memochat-gate" : _kafka_client_id },
                { "socket.timeout.ms", "3000" },
                { "message.timeout.ms", "5000" }
            };
            _kafka_producer = std::make_shared<cppkafka::Producer>(config);
        }
        auto producer = std::static_pointer_cast<cppkafka::Producer>(_kafka_producer);
        cppkafka::MessageBuilder builder(topic);
        builder.key(partition_key);
        builder.payload(serialized);
        producer->produce(builder);
        producer->flush(std::chrono::milliseconds(3000));
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

bool GateAsyncSideEffects::PublishRabbit(const std::string& routing_key,
    const std::string& task_type,
    const Json::Value& payload,
    std::string* error)
{
#if MEMOCHAT_ENABLE_RABBITMQ
    std::lock_guard<std::mutex> guard(_mutex);
    if (!EnsureRabbitConnected(error)) {
        return false;
    }
    Json::Value envelope(Json::objectValue);
    envelope["task_id"] = boost::uuids::to_string(boost::uuids::random_generator()());
    envelope["task_type"] = task_type;
    envelope["trace_id"] = memolog::TraceContext::GetTraceId();
    envelope["request_id"] = memolog::TraceContext::GetRequestId();
    envelope["created_at_ms"] = static_cast<Json::Int64>(NowMsGate());
    envelope["retry_count"] = 0;
    envelope["max_retries"] = 5;
    envelope["routing_key"] = routing_key;
    envelope["payload"] = payload;
    amqp_basic_properties_t props;
    props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
    props.content_type = amqp_cstring_bytes("application/json");
    props.delivery_mode = 2;
    const auto serialized = WriteCompactJsonGate(envelope);
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

void GateAsyncSideEffects::ConsumeCacheInvalidateLoop()
{
#if MEMOCHAT_ENABLE_RABBITMQ
    bool consume_started = false;
    while (!_stop.load()) {
        std::string error;
        {
            std::lock_guard<std::mutex> guard(_mutex);
            if (!EnsureRabbitConnected(&error)) {
                memolog::LogWarn("gate.cache_invalidate_worker.connect_failed", "failed to connect rabbitmq cache invalidate worker",
                    {{"error", error}});
            } else if (!consume_started) {
                amqp_basic_consume(static_cast<amqp_connection_state_t>(_rabbit_connection), 1,
                    amqp_cstring_bytes("gate.cache.invalidate.q"), amqp_empty_bytes, 0, 0, 0, amqp_empty_table);
                amqp_get_rpc_reply(static_cast<amqp_connection_state_t>(_rabbit_connection));
                consume_started = true;
            }
        }
        if (!_rabbit_connection) {
            consume_started = false;
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            continue;
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

        Json::CharReaderBuilder builder;
        std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
        Json::Value root;
        std::string parse_errors;
        const auto serialized = BytesToStringGate(envelope.message.body);
        if (reader->parse(serialized.data(), serialized.data() + serialized.size(), &root, &parse_errors) && root.isObject()) {
            HandleCacheInvalidate(root["payload"]);
        }
        amqp_basic_ack(static_cast<amqp_connection_state_t>(_rabbit_connection), 1, envelope.delivery_tag, false);
        amqp_destroy_envelope(&envelope);
    }
#endif
}

bool GateAsyncSideEffects::EnsureRabbitConnected(std::string* error)
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
    if (!GateRpcReplyOk(amqp_login(connection, _rabbit_vhost.c_str(), 0, 131072, 0, AMQP_SASL_METHOD_PLAIN,
        _rabbit_username.c_str(), _rabbit_password.c_str()), error)) {
        amqp_destroy_connection(connection);
        return false;
    }
    amqp_channel_open(connection, 1);
    if (!GateRpcReplyOk(amqp_get_rpc_reply(connection), error)) {
        amqp_connection_close(connection, AMQP_REPLY_SUCCESS);
        amqp_destroy_connection(connection);
        return false;
    }
    amqp_basic_qos(connection, 1, 0, static_cast<uint16_t>(_rabbit_prefetch_count), false);
    GateRpcReplyOk(amqp_get_rpc_reply(connection), error);
    _rabbit_connection = connection;
    return EnsureRabbitTopology(error);
#else
    (void)error;
    return false;
#endif
}

bool GateAsyncSideEffects::EnsureRabbitTopology(std::string* error)
{
#if MEMOCHAT_ENABLE_RABBITMQ
    auto connection = static_cast<amqp_connection_state_t>(_rabbit_connection);
    amqp_exchange_declare(connection, 1, amqp_cstring_bytes(_rabbit_exchange_direct.c_str()),
        amqp_cstring_bytes("direct"), 0, 1, 0, 0, amqp_empty_table);
    if (!GateRpcReplyOk(amqp_get_rpc_reply(connection), error)) {
        return false;
    }
    amqp_exchange_declare(connection, 1, amqp_cstring_bytes(_rabbit_exchange_dlx.c_str()),
        amqp_cstring_bytes("direct"), 0, 1, 0, 0, amqp_empty_table);
    if (!GateRpcReplyOk(amqp_get_rpc_reply(connection), error)) {
        return false;
    }
    amqp_queue_declare(connection, 1, amqp_cstring_bytes("gate.cache.invalidate.q"), 0, 1, 0, 0, amqp_empty_table);
    if (!GateRpcReplyOk(amqp_get_rpc_reply(connection), error)) {
        return false;
    }
    amqp_queue_bind(connection, 1, amqp_cstring_bytes("gate.cache.invalidate.q"),
        amqp_cstring_bytes(_rabbit_exchange_direct.c_str()), amqp_cstring_bytes("gate.cache.invalidate"), amqp_empty_table);
    if (!GateRpcReplyOk(amqp_get_rpc_reply(connection), error)) {
        return false;
    }
    return true;
#else
    (void)error;
    return false;
#endif
}

void GateAsyncSideEffects::HandleCacheInvalidate(const Json::Value& payload)
{
    if (!payload.isObject()) {
        return;
    }
    const auto email = payload.get("email", "").asString();
    const auto reason = payload.get("reason", "").asString();
    if (email.empty()) {
        memolog::LogWarn("gate.cache_invalidate.invalid_payload", "cache invalidate payload missing email");
        return;
    }
    gateauthsupport::InvalidateLoginCacheByEmail(email);
    memolog::LogInfo("gate.cache_invalidate.processed", "processed cache invalidate task",
        {{"email", email}, {"reason", reason}});
}

void GateAsyncSideEffects::CloseRabbit()
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

void GateAsyncSideEffects::CloseKafka()
{
#if MEMOCHAT_ENABLE_KAFKA
    std::lock_guard<std::mutex> guard(_kafka_mutex);
    if (_kafka_producer) {
        auto producer = std::static_pointer_cast<cppkafka::Producer>(_kafka_producer);
        producer->flush(std::chrono::milliseconds(1000));
        _kafka_producer.reset();
    }
#endif
}
