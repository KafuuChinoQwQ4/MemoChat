#include "RedisAsyncEventBus.h"

#include "ChatAsyncEvent.h"
#include "ChatRuntime.h"
#include "RedisMgr.h"

#include "json/GlazeCompat.h"
#include <memory>

bool RedisAsyncEventBus::Publish(const std::string& topic, const memochat::json::JsonValue& payload, std::string* error)
{
    auto envelope = BuildAsyncEventEnvelope(topic, payload);
    const auto serialized = SerializeAsyncEventEnvelope(envelope);
    if (serialized.empty()) {
        if (error) {
            *error = "serialize_failed";
        }
        return false;
    }
    if (!RedisMgr::GetInstance()->RPush(memochat::chatruntime::QueueKeyForTopic(topic), serialized)) {
        if (error) {
            *error = "queue_publish_failed";
        }
        return false;
    }
    return true;
}

bool RedisAsyncEventBus::ConsumeOnce(const std::vector<std::string>& topics, AsyncConsumedEvent& event, std::string* error)
{
    for (const auto& one_topic : topics) {
        std::string raw;
        if (!RedisMgr::GetInstance()->LPop(memochat::chatruntime::QueueKeyForTopic(one_topic), raw) || raw.empty()) {
            continue;
        }

        event = AsyncConsumedEvent();
        event.serialized = raw;
        event.parsed = ParseAsyncEventEnvelope(raw, event.envelope);
        if (!event.parsed) {
            event.envelope.topic = one_topic;
            if (error) {
                *error = "parse_failed";
            }
        }
        _last_topic = one_topic;
        _last_serialized = raw;
        return true;
    }

    return false;
}

void RedisAsyncEventBus::AckLastConsumed()
{
    _last_topic.clear();
    _last_serialized.clear();
}

void RedisAsyncEventBus::NackLastConsumed(const std::string&)
{
    if (_last_topic.empty() || _last_serialized.empty()) {
        return;
    }
    RedisMgr::GetInstance()->RPush(memochat::chatruntime::QueueKeyForTopic(_last_topic), _last_serialized);
    _last_topic.clear();
    _last_serialized.clear();
}
