#include "KafkaAsyncEventBus.hpp"

#include "ChatAsyncEvent.hpp"
#include "ChatRuntime.hpp"
#include "PostgresMgr.hpp"
#include "logging/Logger.hpp"
#include "random/Uuid.hpp"

#include <chrono>
#include <memory>

#ifndef MEMOCHAT_ENABLE_KAFKA
#define MEMOCHAT_ENABLE_KAFKA 0
#endif

#include "json/GlazeCompat.hpp"

import memochat.chat.kafka_async_event_bus_algorithms;

namespace kafka_event_modules = memochat::chat::messaging::kafka_event_bus::modules;

namespace
{
int64_t NowMsKafkaBus()
{
    return static_cast<int64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count());
}

#if MEMOCHAT_ENABLE_KAFKA
bool SetKafkaConfig(rd_kafka_conf_t* config, const char* key, const std::string& value, std::string& error)
{
    char error_buffer[512]{};
    if (rd_kafka_conf_set(config, key, value.c_str(), error_buffer, sizeof(error_buffer)) != RD_KAFKA_CONF_OK)
    {
        error = error_buffer;
        return false;
    }
    return true;
}

rd_kafka_t* CreateKafkaHandle(rd_kafka_type_t type,
                              const std::vector<std::pair<const char*, std::string>>& values,
                              std::string& error)
{
    rd_kafka_conf_t* config = rd_kafka_conf_new();
    if (config == nullptr)
    {
        error = "rd_kafka_conf_new_failed";
        return nullptr;
    }
    for (const auto& [key, value] : values)
    {
        if (!SetKafkaConfig(config, key, value, error))
        {
            rd_kafka_conf_destroy(config);
            return nullptr;
        }
    }

    char error_buffer[512]{};
    rd_kafka_t* handle = rd_kafka_new(type, config, error_buffer, sizeof(error_buffer));
    if (handle == nullptr)
    {
        error = error_buffer;
        rd_kafka_conf_destroy(config);
        return nullptr;
    }
    return handle;
}
#endif
} // namespace

bool KafkaAsyncEventBus::BuildAvailable()
{
#if MEMOCHAT_ENABLE_KAFKA
    return true;
#else
    return false;
#endif
}

KafkaAsyncEventBus::KafkaAsyncEventBus(PublishOutboxRepairTaskFn publish_outbox_repair_task)
    : _config(LoadKafkaConfig())
    , _publish_outbox_repair_task(std::move(publish_outbox_repair_task))
{
#if MEMOCHAT_ENABLE_KAFKA
    if (kafka_event_modules::ShouldStartFromValidConfig(_config.valid()))
    {
        std::string startup_error;
        _producer = CreateKafkaHandle(RD_KAFKA_PRODUCER,
                                      {{"bootstrap.servers", _config.brokers},
                                       {"client.id", _config.client_id},
                                       {"enable.idempotence", _config.enable_idempotence ? "true" : "false"},
                                       {"batch.num.messages", std::to_string(_config.batch_num_messages)},
                                       {"linger.ms", std::to_string(_config.queue_buffering_max_ms)}},
                                      startup_error);
        _consumer = CreateKafkaHandle(RD_KAFKA_CONSUMER,
                                      {{"bootstrap.servers", _config.brokers},
                                       {"group.id", _config.consumer_group},
                                       {"client.id", _config.client_id + ".consumer"},
                                       {"enable.auto.commit", "false"},
                                       {"auto.offset.reset", "earliest"},
                                       {"session.timeout.ms", std::to_string(_config.session_timeout_ms)}},
                                      startup_error);
        if (_producer == nullptr || _consumer == nullptr)
        {
            if (_producer != nullptr)
            {
                rd_kafka_destroy(_producer);
                _producer = nullptr;
            }
            if (_consumer != nullptr)
            {
                rd_kafka_destroy(_consumer);
                _consumer = nullptr;
            }
            memolog::LogError("chat.kafka.start_failed",
                              "chat kafka event bus failed to initialize",
                              {{"error", startup_error}});
            return;
        }

        const rd_kafka_resp_err_t poll_error = rd_kafka_poll_set_consumer(_consumer);
        if (poll_error != RD_KAFKA_RESP_ERR_NO_ERROR)
        {
            memolog::LogError("chat.kafka.start_failed",
                              "chat kafka consumer poll setup failed",
                              {{"error", rd_kafka_err2str(poll_error)}});
            rd_kafka_destroy(_producer);
            rd_kafka_destroy(_consumer);
            _producer = nullptr;
            _consumer = nullptr;
            return;
        }
        rd_kafka_topic_partition_list_t* topics = rd_kafka_topic_partition_list_new(2);
        if (topics == nullptr)
        {
            memolog::LogError("chat.kafka.start_failed", "chat kafka topic list allocation failed");
            rd_kafka_destroy(_producer);
            rd_kafka_destroy(_consumer);
            _producer = nullptr;
            _consumer = nullptr;
            return;
        }
        rd_kafka_topic_partition_list_add(topics, _config.topic_private.c_str(), RD_KAFKA_PARTITION_UA);
        rd_kafka_topic_partition_list_add(topics, _config.topic_group.c_str(), RD_KAFKA_PARTITION_UA);
        const rd_kafka_resp_err_t subscribe_error = rd_kafka_subscribe(_consumer, topics);
        rd_kafka_topic_partition_list_destroy(topics);
        if (subscribe_error != RD_KAFKA_RESP_ERR_NO_ERROR)
        {
            memolog::LogError("chat.kafka.start_failed",
                              "chat kafka consumer subscribe failed",
                              {{"error", rd_kafka_err2str(subscribe_error)}});
            rd_kafka_destroy(_producer);
            rd_kafka_destroy(_consumer);
            _producer = nullptr;
            _consumer = nullptr;
            return;
        }
        _outbox_service = std::make_unique<ChatOutboxService>(
            _config,
            [this](const std::string& topic,
                   const std::string& partition_key,
                   const std::string& payload_json,
                   std::string* error)
            {
                return ProduceSerialized(topic, partition_key, payload_json, error);
            },
            [this](int64_t outbox_id, int delay_ms, int max_retries, const std::string&)
            {
                if (kafka_event_modules::ShouldSkipOutboxRepairPublish(!_publish_outbox_repair_task))
                {
                    return;
                }
                std::string error;
                _publish_outbox_repair_task(outbox_id, delay_ms, max_retries, &error);
            });
        std::string outbox_error;
        if (!_outbox_service->Start(&outbox_error))
        {
            memolog::LogError("chat.kafka.outbox_start_failed",
                              "chat kafka outbox thread failed to start",
                              {{"error", outbox_error}});
            _outbox_service.reset();
            rd_kafka_destroy(_producer);
            rd_kafka_destroy(_consumer);
            _producer = nullptr;
            _consumer = nullptr;
            return;
        }
        memolog::LogInfo("chat.kafka.started",
                         "chat kafka event bus started",
                         {{"brokers", _config.brokers},
                          {"client_id", _config.client_id},
                          {"consumer_group", _config.consumer_group},
                          {"topic_private", _config.topic_private},
                          {"topic_group", _config.topic_group}});
    }
    else
    {
        memolog::LogWarn("chat.kafka.invalid_config",
                         "chat kafka config invalid, event bus disabled",
                         {{"brokers", _config.brokers},
                          {"client_id", _config.client_id},
                          {"consumer_group", _config.consumer_group}});
    }
#endif
}

KafkaAsyncEventBus::~KafkaAsyncEventBus()
{
    if (_outbox_service)
    {
        _outbox_service->Stop();
    }
#if MEMOCHAT_ENABLE_KAFKA
    ClearLastConsumed();
    if (_consumer != nullptr)
    {
        rd_kafka_consumer_close(_consumer);
        rd_kafka_destroy(_consumer);
        _consumer = nullptr;
    }
    if (_producer != nullptr)
    {
        rd_kafka_flush(_producer, 5000);
        rd_kafka_destroy(_producer);
        _producer = nullptr;
    }
#endif
}

bool KafkaAsyncEventBus::Publish(const std::string& topic, const memochat::json::JsonValue& payload, std::string* error)
{
#if MEMOCHAT_ENABLE_KAFKA
    if (kafka_event_modules::ShouldRejectPublish(!_producer, !_config.valid()))
    {
        if (error)
        {
            *error = kafka_event_modules::KafkaNotConfiguredError();
        }
        return false;
    }

    auto envelope = BuildAsyncEventEnvelope(topic, payload);
    if (kafka_event_modules::ShouldAssignGeneratedEventId(envelope.event_id.empty()))
    {
        if (!memochat::random::GenerateUuid(envelope.event_id, error))
        {
            return false;
        }
        auto payload_copy = envelope.payload;
        payload_copy["event_id"] = envelope.event_id;
        envelope.payload = payload_copy;
    }
    const auto serialized = SerializeAsyncEventEnvelope(envelope);
    if (kafka_event_modules::ShouldRejectSerializedPayload(serialized.empty()))
    {
        if (error)
        {
            *error = kafka_event_modules::SerializeFailedError();
        }
        return false;
    }

    ChatOutboxEventInfo record;
    record.event_id = envelope.event_id;
    record.topic = envelope.topic;
    record.partition_key = envelope.partition_key;
    record.payload_json = serialized;
    record.status = kafka_event_modules::InitialOutboxStatus();
    record.retry_count = kafka_event_modules::InitialRetryCount();
    record.next_retry_at = NowMsKafkaBus();
    record.created_at = NowMsKafkaBus();
    if (!PostgresMgr::GetInstance()->EnqueueChatOutboxEvent(record))
    {
        if (error)
        {
            *error = kafka_event_modules::OutboxEnqueueFailedError();
        }
        return false;
    }
    return true;
#else
    (void) topic;
    (void) payload;
    if (error)
    {
        *error = kafka_event_modules::KafkaBuildDisabledError();
    }
    return false;
#endif
}

bool KafkaAsyncEventBus::ConsumeOnce(const std::vector<std::string>&, AsyncConsumedEvent& event, std::string* error)
{
#if MEMOCHAT_ENABLE_KAFKA
    if (!_consumer)
    {
        return false;
    }

    std::lock_guard<std::mutex> guard(_consumer_mutex);
    rd_kafka_message_t* message = rd_kafka_consumer_poll(_consumer, 50);
    if (message == nullptr)
    {
        return false;
    }
    if (message->err != RD_KAFKA_RESP_ERR_NO_ERROR)
    {
        if (message->err != RD_KAFKA_RESP_ERR__PARTITION_EOF && error != nullptr)
        {
            *error = rd_kafka_message_errstr(message);
        }
        rd_kafka_message_destroy(message);
        return false;
    }

    event = AsyncConsumedEvent();
    if (message->payload != nullptr && message->len > 0)
    {
        event.serialized.assign(static_cast<const char*>(message->payload), message->len);
    }
    event.parsed = ParseAsyncEventEnvelope(event.serialized, event.envelope);
    if (kafka_event_modules::ShouldSetParseError(event.parsed, error != nullptr))
    {
        *error = kafka_event_modules::ParseFailedError();
    }
    ClearLastConsumed();
    _last_message = message;
    _last_consumed = event;
    return true;
#else
    (void) event;
    if (error)
    {
        *error = kafka_event_modules::KafkaBuildDisabledError();
    }
    return false;
#endif
}

void KafkaAsyncEventBus::AckLastConsumed()
{
#if MEMOCHAT_ENABLE_KAFKA
    std::lock_guard<std::mutex> guard(_consumer_mutex);
    if (kafka_event_modules::ShouldCommitLastConsumed(_consumer != nullptr, _last_message != nullptr))
    {
        const rd_kafka_resp_err_t commit_error = rd_kafka_commit_message(_consumer, _last_message, 0);
        if (commit_error != RD_KAFKA_RESP_ERR_NO_ERROR)
        {
            memolog::LogWarn("chat.kafka.commit_failed",
                             "chat kafka offset commit failed",
                             {{"error", rd_kafka_err2str(commit_error)}});
        }
    }
    ClearLastConsumed();
#endif
}

void KafkaAsyncEventBus::NackLastConsumed(const std::string& error)
{
#if MEMOCHAT_ENABLE_KAFKA
    std::lock_guard<std::mutex> guard(_consumer_mutex);
    if (!kafka_event_modules::ShouldNackLastConsumed(_consumer != nullptr, _last_message != nullptr))
    {
        return;
    }

    AsyncEventEnvelope envelope = _last_consumed.envelope;
    envelope.retry_count += 1;
    const auto serialized = SerializeAsyncEventEnvelope(envelope);
    std::string publish_error;
    if (kafka_event_modules::ShouldRouteToDlq(envelope.retry_count, _config.consume_retry_max))
    {
        ProduceSerialized(DlqTopicForLastConsumed(), envelope.partition_key, serialized, &publish_error);
    }
    else
    {
        ProduceSerialized(envelope.topic, envelope.partition_key, serialized, &publish_error);
    }

    const rd_kafka_resp_err_t commit_error = rd_kafka_commit_message(_consumer, _last_message, 0);
    memolog::LogWarn("chat.kafka.consume_failed",
                     "chat kafka consume failed",
                     {{"event_id", envelope.event_id},
                      {"topic", envelope.topic},
                      {"partition_key", envelope.partition_key},
                      {"retry_count", std::to_string(envelope.retry_count)},
                      {"error", error},
                      {"publish_error", publish_error},
                      {"commit_error",
                       commit_error == RD_KAFKA_RESP_ERR_NO_ERROR ? std::string() : rd_kafka_err2str(commit_error)}});
    ClearLastConsumed();
#else
    (void) error;
#endif
}

bool KafkaAsyncEventBus::ProduceSerialized(const std::string& topic,
                                           const std::string& partition_key,
                                           const std::string& payload_json,
                                           std::string* error)
{
#if MEMOCHAT_ENABLE_KAFKA
    if (!_producer)
    {
        if (error)
        {
            *error = kafka_event_modules::ProducerUnavailableError();
        }
        return false;
    }
    std::lock_guard<std::mutex> guard(_producer_mutex);
    const rd_kafka_resp_err_t produce_error =
        rd_kafka_producev(_producer,
                          RD_KAFKA_V_TOPIC(topic.c_str()),
                          RD_KAFKA_V_KEY(const_cast<char*>(partition_key.data()), partition_key.size()),
                          RD_KAFKA_V_VALUE(const_cast<char*>(payload_json.data()), payload_json.size()),
                          RD_KAFKA_V_MSGFLAGS(RD_KAFKA_MSG_F_COPY),
                          RD_KAFKA_V_END);
    if (produce_error != RD_KAFKA_RESP_ERR_NO_ERROR)
    {
        if (error != nullptr)
        {
            *error = rd_kafka_err2str(produce_error);
        }
        return false;
    }
    const rd_kafka_resp_err_t flush_error = rd_kafka_flush(_producer, 5000);
    if (flush_error != RD_KAFKA_RESP_ERR_NO_ERROR)
    {
        if (error != nullptr)
        {
            *error = rd_kafka_err2str(flush_error);
        }
        return false;
    }
    return true;
#else
    (void) topic;
    (void) partition_key;
    (void) payload_json;
    if (error)
    {
        *error = kafka_event_modules::KafkaBuildDisabledError();
    }
    return false;
#endif
}

std::string KafkaAsyncEventBus::DlqTopicForLastConsumed() const
{
    return DlqTopicFor(_config, _last_consumed.envelope.topic);
}

void KafkaAsyncEventBus::ClearLastConsumed()
{
    if (_last_message != nullptr)
    {
        rd_kafka_message_destroy(_last_message);
        _last_message = nullptr;
    }
    _last_consumed = AsyncConsumedEvent();
}
