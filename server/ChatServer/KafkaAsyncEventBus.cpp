#ifndef _HAS_STD_BYTE
#define _HAS_STD_BYTE 0
#endif

#include "KafkaAsyncEventBus.h"

#include "ChatAsyncEvent.h"
#include "ChatRuntime.h"
#include "PostgresMgr.h"
#include "logging/Logger.h"

#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <chrono>
#include <json/json.h>
#include <memory>

#ifndef MEMOCHAT_ENABLE_KAFKA
#define MEMOCHAT_ENABLE_KAFKA 0
#endif

#if MEMOCHAT_ENABLE_KAFKA
#include <cppkafka/cppkafka.h>
#endif

namespace {
int64_t NowMsKafkaBus()
{
    return static_cast<int64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());
}
}

bool KafkaAsyncEventBus::BuildAvailable()
{
#if MEMOCHAT_ENABLE_KAFKA
    return true;
#else
    return false;
#endif
}

KafkaAsyncEventBus::KafkaAsyncEventBus(PublishOutboxRepairTaskFn publish_outbox_repair_task)
    : _config(LoadKafkaConfig()),
      _publish_outbox_repair_task(std::move(publish_outbox_repair_task)) {
#if MEMOCHAT_ENABLE_KAFKA
    if (_config.valid()) {
        cppkafka::Configuration producer_config = {
            { "metadata.broker.list", _config.brokers },
            { "client.id", _config.client_id },
            { "enable.idempotence", _config.enable_idempotence ? "true" : "false" },
            { "batch.num.messages", std::to_string(_config.batch_num_messages) },
            { "queue.buffering.max.ms", std::to_string(_config.queue_buffering_max_ms) }
        };
        cppkafka::Configuration consumer_config = {
            { "metadata.broker.list", _config.brokers },
            { "group.id", _config.consumer_group },
            { "client.id", _config.client_id + ".consumer" },
            { "enable.auto.commit", false },
            { "auto.offset.reset", "earliest" },
            { "session.timeout.ms", std::to_string(_config.session_timeout_ms) }
        };
        _producer = std::make_unique<cppkafka::Producer>(producer_config);
        _consumer = std::make_unique<cppkafka::Consumer>(consumer_config);
        _consumer->subscribe({ _config.topic_private, _config.topic_group });
        _outbox_service = std::make_unique<ChatOutboxService>(_config,
            [this](const std::string& topic, const std::string& partition_key, const std::string& payload_json, std::string* error) {
                return ProduceSerialized(topic, partition_key, payload_json, error);
            },
            [this](int64_t outbox_id, int delay_ms, int max_retries, const std::string&) {
                if (!_publish_outbox_repair_task) {
                    return;
                }
                std::string error;
                _publish_outbox_repair_task(outbox_id, delay_ms, max_retries, &error);
            });
        _outbox_service->Start();
        memolog::LogInfo("chat.kafka.started", "chat kafka event bus started",
            {
                {"brokers", _config.brokers},
                {"client_id", _config.client_id},
                {"consumer_group", _config.consumer_group},
                {"topic_private", _config.topic_private},
                {"topic_group", _config.topic_group}
            });
    } else {
        memolog::LogWarn("chat.kafka.invalid_config", "chat kafka config invalid, event bus disabled",
            {
                {"brokers", _config.brokers},
                {"client_id", _config.client_id},
                {"consumer_group", _config.consumer_group}
            });
    }
#endif
}

KafkaAsyncEventBus::~KafkaAsyncEventBus()
{
    if (_outbox_service) {
        _outbox_service->Stop();
    }
}

bool KafkaAsyncEventBus::Publish(const std::string& topic, const Json::Value& payload, std::string* error)
{
#if MEMOCHAT_ENABLE_KAFKA
    if (!_producer || !_config.valid()) {
        if (error) {
            *error = "kafka_not_configured";
        }
        return false;
    }

    auto envelope = BuildAsyncEventEnvelope(topic, payload);
    if (envelope.event_id.empty()) {
        envelope.event_id = boost::uuids::to_string(boost::uuids::random_generator()());
        auto payload_copy = envelope.payload();
        payload_copy["event_id"] = envelope.event_id;
        envelope.setPayload(payload_copy);
    }
    const auto serialized = SerializeAsyncEventEnvelope(envelope);
    if (serialized.empty()) {
        if (error) {
            *error = "serialize_failed";
        }
        return false;
    }

    ChatOutboxEventInfo record;
    record.event_id = envelope.event_id;
    record.topic = envelope.topic;
    record.partition_key = envelope.partition_key;
    record.payload_json = serialized;
    record.status = 0;
    record.retry_count = 0;
    record.next_retry_at = NowMsKafkaBus();
    record.created_at = NowMsKafkaBus();
    if (!PostgresMgr::GetInstance()->EnqueueChatOutboxEvent(record)) {
        if (error) {
            *error = "outbox_enqueue_failed";
        }
        return false;
    }
    return true;
#else
    (void)topic;
    (void)payload;
    if (error) {
        *error = "kafka_build_disabled";
    }
    return false;
#endif
}

bool KafkaAsyncEventBus::ConsumeOnce(const std::vector<std::string>&, AsyncConsumedEvent& event, std::string* error)
{
#if MEMOCHAT_ENABLE_KAFKA
    if (!_consumer) {
        return false;
    }

    std::lock_guard<std::mutex> guard(_consumer_mutex);
    auto message = _consumer->poll(std::chrono::milliseconds(50));
    if (!message) {
        return false;
    }
    if (message.get_error()) {
        if (!message.is_eof() && error) {
            *error = message.get_error().to_string();
        }
        return false;
    }

    event = AsyncConsumedEvent();
    event.serialized = message.get_payload();
    event.parsed = ParseAsyncEventEnvelope(event.serialized, event.envelope);
    if (!event.parsed && error) {
        *error = "parse_failed";
    }
    _last_message = std::make_shared<cppkafka::Message>(std::move(message));
    _last_consumed = event;
    return true;
#else
    (void)event;
    if (error) {
        *error = "kafka_build_disabled";
    }
    return false;
#endif
}

void KafkaAsyncEventBus::AckLastConsumed()
{
#if MEMOCHAT_ENABLE_KAFKA
    std::lock_guard<std::mutex> guard(_consumer_mutex);
    if (_consumer && _last_message) {
        _consumer->commit(*_last_message);
    }
    ClearLastConsumed();
#endif
}

void KafkaAsyncEventBus::NackLastConsumed(const std::string& error)
{
#if MEMOCHAT_ENABLE_KAFKA
    std::lock_guard<std::mutex> guard(_consumer_mutex);
    if (!_consumer || !_last_message) {
        return;
    }

    AsyncEventEnvelope envelope = _last_consumed.envelope;
    envelope.retry_count += 1;
    const auto serialized = SerializeAsyncEventEnvelope(envelope);
    std::string publish_error;
    if (envelope.retry_count >= _config.consume_retry_max) {
        ProduceSerialized(DlqTopicForLastConsumed(), envelope.partition_key, serialized, &publish_error);
    } else {
        ProduceSerialized(envelope.topic, envelope.partition_key, serialized, &publish_error);
    }

    _consumer->commit(*_last_message);
    memolog::LogWarn("chat.kafka.consume_failed", "chat kafka consume failed",
        {
            {"event_id", envelope.event_id},
            {"topic", envelope.topic},
            {"partition_key", envelope.partition_key},
            {"retry_count", std::to_string(envelope.retry_count)},
            {"error", error},
            {"publish_error", publish_error}
        });
    ClearLastConsumed();
#else
    (void)error;
#endif
}

bool KafkaAsyncEventBus::ProduceSerialized(const std::string& topic, const std::string& partition_key, const std::string& payload_json, std::string* error)
{
#if MEMOCHAT_ENABLE_KAFKA
    if (!_producer) {
        if (error) {
            *error = "producer_unavailable";
        }
        return false;
    }
    try {
        std::lock_guard<std::mutex> guard(_producer_mutex);
        cppkafka::MessageBuilder builder(topic);
        builder.key(partition_key);
        builder.payload(payload_json);
        _producer->produce(builder);
        _producer->flush();
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
    (void)payload_json;
    if (error) {
        *error = "kafka_build_disabled";
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
    _last_message.reset();
    _last_consumed = AsyncConsumedEvent();
}
