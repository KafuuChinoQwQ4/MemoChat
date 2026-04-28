#include "RabbitMqTaskBus.h"

#include "ChatRuntime.h"
#include "logging/Logger.h"

#include <chrono>
#include <memory>

#ifndef MEMOCHAT_ENABLE_RABBITMQ
#define MEMOCHAT_ENABLE_RABBITMQ 0
#endif

#if MEMOCHAT_ENABLE_RABBITMQ
#include "../common/WinsockCompat.h"
#include <rabbitmq-c/amqp.h>
#include <rabbitmq-c/framing.h>
#include <rabbitmq-c/tcp_socket.h>
#endif

namespace {
#if MEMOCHAT_ENABLE_RABBITMQ
std::string BytesToString(amqp_bytes_t bytes)
{
    return std::string(static_cast<const char*>(bytes.bytes), bytes.len);
}

bool RpcReplyOk(amqp_rpc_reply_t reply, std::string* error)
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

bool RabbitMqTaskBus::BuildAvailable()
{
#if MEMOCHAT_ENABLE_RABBITMQ
    return true;
#else
    return false;
#endif
}

RabbitMqTaskBus::RabbitMqTaskBus()
    : _config(LoadRabbitMqConfig()) {
#if MEMOCHAT_ENABLE_RABBITMQ
    std::string error;
    if (!Connect(&error)) {
        memolog::LogWarn("chat.taskbus.rabbitmq_connect_failed", "rabbitmq task bus connect failed",
            {{"error", error}, {"host", _config.host}, {"port", std::to_string(_config.port)}});
    }
#endif
}

RabbitMqTaskBus::~RabbitMqTaskBus()
{
    Close();
}

bool RabbitMqTaskBus::Publish(const TaskEnvelope& task, std::string* error)
{
#if MEMOCHAT_ENABLE_RABBITMQ
    std::lock_guard<std::mutex> guard(_mutex);
    if (!_connection && !Connect(error)) {
        return false;
    }
    return PublishSerialized(_config.exchange_direct, task.routing_key, SerializeTaskEnvelope(task), error);
#else
    (void)task;
    if (error) {
        *error = "rabbitmq_build_disabled";
    }
    return false;
#endif
}

bool RabbitMqTaskBus::ConsumeOnce(const std::vector<std::string>& routing_keys, ConsumedTask& task, std::string* error)
{
#if MEMOCHAT_ENABLE_RABBITMQ
    std::lock_guard<std::mutex> guard(_mutex);
    if (!_connection && !Connect(error)) {
        return false;
    }
    if (_subscribed_routing_keys != routing_keys) {
        _subscribed_routing_keys = routing_keys;
        for (const auto& routing_key : _subscribed_routing_keys) {
            const auto queue_name = QueueNameForRoutingKey(routing_key);
            amqp_basic_consume(_connection, 1, amqp_cstring_bytes(queue_name.c_str()), amqp_empty_bytes, 0, 0, 0, amqp_empty_table);
            if (!RpcReplyOk(amqp_get_rpc_reply(_connection), error)) {
                return false;
            }
        }
    }

    timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 50000;
    auto* envelope = new amqp_envelope_t();
    amqp_maybe_release_buffers(_connection);
    const auto reply = amqp_consume_message(_connection, envelope, &timeout, 0);
    if (reply.reply_type != AMQP_RESPONSE_NORMAL) {
        delete envelope;
        return false;
    }

    task = ConsumedTask();
    task.serialized = BytesToString(envelope->message.body);
    task.parsed = ParseTaskEnvelope(task.serialized, task.envelope);
    _last_envelope = envelope;
    _last_consumed = task;
    return true;
#else
    (void)routing_keys;
    (void)task;
    if (error) {
        *error = "rabbitmq_build_disabled";
    }
    return false;
#endif
}

void RabbitMqTaskBus::AckLastConsumed()
{
#if MEMOCHAT_ENABLE_RABBITMQ
    std::lock_guard<std::mutex> guard(_mutex);
    if (_connection && _last_envelope) {
        auto* envelope = static_cast<amqp_envelope_t*>(_last_envelope);
        amqp_basic_ack(_connection, 1, envelope->delivery_tag, false);
    }
    ClearLastConsumed();
#endif
}

void RabbitMqTaskBus::NackLastConsumed(const std::string& error)
{
#if MEMOCHAT_ENABLE_RABBITMQ
    std::lock_guard<std::mutex> guard(_mutex);
    if (!_connection || !_last_envelope) {
        return;
    }
    TaskEnvelope envelope = _last_consumed.envelope;
    envelope.retry_count += 1;
    std::string publish_error;
    if (envelope.retry_count > envelope.max_retries) {
        PublishSerialized(_config.exchange_dlx, DlqRoutingKeyFor(envelope.routing_key), SerializeTaskEnvelope(envelope), &publish_error);
    } else {
        PublishSerialized("", RetryQueueNameForRoutingKey(envelope.routing_key), SerializeTaskEnvelope(envelope), &publish_error);
    }
    auto* last_envelope = static_cast<amqp_envelope_t*>(_last_envelope);
    amqp_basic_ack(_connection, 1, last_envelope->delivery_tag, false);
    memolog::LogWarn("chat.taskbus.rabbitmq_nack", "rabbitmq task requeued or dead-lettered",
        {
            {"task_id", envelope.task_id},
            {"task_type", envelope.task_type},
            {"routing_key", envelope.routing_key},
            {"retry_count", std::to_string(envelope.retry_count)},
            {"error", error},
            {"publish_error", publish_error}
        });
    ClearLastConsumed();
#else
    (void)error;
#endif
}

bool RabbitMqTaskBus::Connect(std::string* error)
{
#if MEMOCHAT_ENABLE_RABBITMQ
    if (!_config.valid()) {
        if (error) {
            *error = "rabbitmq_invalid_config";
        }
        return false;
    }
    Close();
    _connection = amqp_new_connection();
    auto* socket = amqp_tcp_socket_new(_connection);
    if (!socket) {
        if (error) {
            *error = "rabbitmq_socket_create_failed";
        }
        Close();
        return false;
    }
    if (amqp_socket_open(socket, _config.host.c_str(), _config.port) != AMQP_STATUS_OK) {
        if (error) {
            *error = "rabbitmq_socket_open_failed";
        }
        Close();
        return false;
    }
    if (!RpcReplyOk(amqp_login(_connection, _config.vhost.c_str(), 0, 131072, 0, AMQP_SASL_METHOD_PLAIN,
        _config.username.c_str(), _config.password.c_str()), error)) {
        Close();
        return false;
    }
    amqp_channel_open(_connection, 1);
    if (!RpcReplyOk(amqp_get_rpc_reply(_connection), error)) {
        Close();
        return false;
    }
    amqp_basic_qos(_connection, 1, 0, static_cast<uint16_t>(_config.prefetch_count), false);
    if (!RpcReplyOk(amqp_get_rpc_reply(_connection), error)) {
        Close();
        return false;
    }
    return EnsureTopology(error);
#else
    (void)error;
    return false;
#endif
}

void RabbitMqTaskBus::Close()
{
#if MEMOCHAT_ENABLE_RABBITMQ
    ClearLastConsumed();
    if (_connection) {
        amqp_channel_close(_connection, 1, AMQP_REPLY_SUCCESS);
        amqp_connection_close(_connection, AMQP_REPLY_SUCCESS);
        amqp_destroy_connection(_connection);
        _connection = nullptr;
    }
#endif
}

bool RabbitMqTaskBus::EnsureTopology(std::string* error)
{
#if MEMOCHAT_ENABLE_RABBITMQ
    amqp_exchange_declare(_connection, 1, amqp_cstring_bytes(_config.exchange_direct.c_str()),
        amqp_cstring_bytes("direct"), 0, 1, 0, 0, amqp_empty_table);
    if (!RpcReplyOk(amqp_get_rpc_reply(_connection), error)) {
        return false;
    }
    amqp_exchange_declare(_connection, 1, amqp_cstring_bytes(_config.exchange_dlx.c_str()),
        amqp_cstring_bytes("direct"), 0, 1, 0, 0, amqp_empty_table);
    if (!RpcReplyOk(amqp_get_rpc_reply(_connection), error)) {
        return false;
    }

    const std::vector<std::string> routing_keys = {
        memochat::chatruntime::TaskRoutingDeliveryRetry(),
        memochat::chatruntime::TaskRoutingOfflineNotify(),
        memochat::chatruntime::TaskRoutingRelationNotify(),
        memochat::chatruntime::TaskRoutingOutboxRepair()
    };
    for (const auto& routing_key : routing_keys) {
        const auto queue_name = QueueNameForRoutingKey(routing_key);
        const auto retry_queue_name = RetryQueueNameForRoutingKey(routing_key);
        const auto dlq_routing = DlqRoutingKeyFor(routing_key);

        amqp_queue_declare(_connection, 1, amqp_cstring_bytes(queue_name.c_str()), 0, 1, 0, 0, amqp_empty_table);
        if (!RpcReplyOk(amqp_get_rpc_reply(_connection), error)) {
            return false;
        }
        amqp_queue_bind(_connection, 1, amqp_cstring_bytes(queue_name.c_str()),
            amqp_cstring_bytes(_config.exchange_direct.c_str()), amqp_cstring_bytes(routing_key.c_str()), amqp_empty_table);
        if (!RpcReplyOk(amqp_get_rpc_reply(_connection), error)) {
            return false;
        }

        amqp_table_entry_t retry_entries[3];
        retry_entries[0].key = amqp_cstring_bytes(const_cast<char*>("x-dead-letter-exchange"));
        retry_entries[0].value.kind = AMQP_FIELD_KIND_UTF8;
        retry_entries[0].value.value.bytes = amqp_cstring_bytes(_config.exchange_direct.c_str());
        retry_entries[1].key = amqp_cstring_bytes(const_cast<char*>("x-dead-letter-routing-key"));
        retry_entries[1].value.kind = AMQP_FIELD_KIND_UTF8;
        retry_entries[1].value.value.bytes = amqp_cstring_bytes(routing_key.c_str());
        retry_entries[2].key = amqp_cstring_bytes(const_cast<char*>("x-message-ttl"));
        retry_entries[2].value.kind = AMQP_FIELD_KIND_I32;
        retry_entries[2].value.value.i32 = _config.retry_delay_ms;
        amqp_table_t retry_args;
        retry_args.num_entries = 3;
        retry_args.entries = retry_entries;
        amqp_queue_declare(_connection, 1, amqp_cstring_bytes(retry_queue_name.c_str()), 0, 1, 0, 0, retry_args);
        if (!RpcReplyOk(amqp_get_rpc_reply(_connection), error)) {
            return false;
        }

        const auto dlq_queue_name = queue_name + ".dlq";
        amqp_queue_declare(_connection, 1, amqp_cstring_bytes(dlq_queue_name.c_str()), 0, 1, 0, 0, amqp_empty_table);
        if (!RpcReplyOk(amqp_get_rpc_reply(_connection), error)) {
            return false;
        }
        amqp_queue_bind(_connection, 1, amqp_cstring_bytes(dlq_queue_name.c_str()),
            amqp_cstring_bytes(_config.exchange_dlx.c_str()), amqp_cstring_bytes(dlq_routing.c_str()), amqp_empty_table);
        if (!RpcReplyOk(amqp_get_rpc_reply(_connection), error)) {
            return false;
        }
    }
    return true;
#else
    (void)error;
    return false;
#endif
}

bool RabbitMqTaskBus::PublishSerialized(const std::string& exchange, const std::string& routing_key, const std::string& payload, std::string* error)
{
#if MEMOCHAT_ENABLE_RABBITMQ
    amqp_basic_properties_t props;
    props._flags = AMQP_BASIC_CONTENT_TYPE_FLAG | AMQP_BASIC_DELIVERY_MODE_FLAG;
    props.content_type = amqp_cstring_bytes("application/json");
    props.delivery_mode = 2;
    const auto status = amqp_basic_publish(_connection, 1, amqp_cstring_bytes(exchange.c_str()),
        amqp_cstring_bytes(routing_key.c_str()), 0, 0, &props, amqp_cstring_bytes(payload.c_str()));
    if (status != AMQP_STATUS_OK) {
        if (error) {
            *error = "rabbitmq_publish_failed";
        }
        return false;
    }
    return true;
#else
    (void)exchange;
    (void)routing_key;
    (void)payload;
    if (error) {
        *error = "rabbitmq_build_disabled";
    }
    return false;
#endif
}

std::string RabbitMqTaskBus::QueueNameForRoutingKey(const std::string& routing_key) const
{
    return routing_key + ".q";
}

std::string RabbitMqTaskBus::RetryQueueNameForRoutingKey(const std::string& routing_key) const
{
    return QueueNameForRoutingKey(routing_key) + ".retry";
}

std::string RabbitMqTaskBus::DlqRoutingKeyFor(const std::string& routing_key) const
{
    return routing_key + ".dlq";
}

void RabbitMqTaskBus::ClearLastConsumed()
{
#if MEMOCHAT_ENABLE_RABBITMQ
    if (_last_envelope) {
        auto* envelope = static_cast<amqp_envelope_t*>(_last_envelope);
        amqp_destroy_envelope(envelope);
        delete envelope;
        _last_envelope = nullptr;
    }
    _last_consumed = ConsumedTask();
#endif
}
