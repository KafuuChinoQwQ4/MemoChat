#include "ConversationContext.h"
#include "logging/Logger.h"
#include "json/GlazeCompat.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/random_generator.hpp>
#include <chrono>
#include <stdexcept>

using namespace std::chrono;

ConversationContext::ConversationContext(const std::string& sid, int32_t u, const std::string& mt, const std::string& mn)
    : session_id(sid), uid(u), model_type(mt), model_name(mn) {
    auto now = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    created_at = now;
    last_active_at = now;
}

void ConversationContext::AddUserMessage(const std::string& content) {
    ChatMessage m;
    m.msg_id = boost::uuids::to_string(boost::uuids::random_generator()());
    m.role = "user";
    m.content = content;
    m.created_at = duration_cast<milliseconds>(
        system_clock::now().time_since_epoch()).count();
    messages.push_back(std::move(m));
    last_active_at = m.created_at;
}

void ConversationContext::AddAssistantMessage(const std::string& content) {
    ChatMessage m;
    m.msg_id = boost::uuids::to_string(boost::uuids::random_generator()());
    m.role = "assistant";
    m.content = content;
    m.created_at = duration_cast<milliseconds>(
        system_clock::now().time_since_epoch()).count();
    messages.push_back(std::move(m));
    last_active_at = m.created_at;
}

void ConversationContext::Prune(size_t max_size) {
    if (messages.size() > max_size) {
        messages.erase(messages.begin(), messages.end() - max_size);
    }
}

std::string ConversationContext::ToJson() const {
    auto msgs = memochat::json::glaze_make_array();
    for (const auto& m : messages) {
        auto jm = memochat::json::make_document(
            memochat::json::kvp<std::string>("msg_id", m.msg_id),
            memochat::json::kvp<std::string>("role", m.role),
            memochat::json::kvp<std::string>("content", m.content),
            memochat::json::kvp<int64_t>("created_at", m.created_at)
        );
        memochat::json::glaze_append(msgs, jm);
    }

    auto root = memochat::json::make_document(
        memochat::json::kvp<std::string>("session_id", session_id),
        memochat::json::kvp<int64_t>("uid", uid),
        memochat::json::kvp<std::string>("model_type", model_type),
        memochat::json::kvp<std::string>("model_name", model_name),
        memochat::json::kvp<int64_t>("created_at", created_at),
        memochat::json::kvp<int64_t>("last_active_at", last_active_at),
        memochat::json::kvp<memochat::json::JsonValue>("messages", std::move(msgs))
    );
    return memochat::json::glaze_stringify(root);
}

ConversationContext ConversationContext::FromJson(const std::string& json) {
    memochat::json::JsonValue root;
    if (!memochat::json::glaze_parse(root, json)) {
        throw std::runtime_error("Failed to parse ConversationContext JSON");
    }

    ConversationContext ctx;
    ctx.session_id = memochat::json::glaze_safe_get<std::string>(root, "session_id", "");
    ctx.uid = static_cast<int32_t>(memochat::json::glaze_safe_get<int64_t>(root, "uid", 0));
    ctx.model_type = memochat::json::glaze_safe_get<std::string>(root, "model_type", "");
    ctx.model_name = memochat::json::glaze_safe_get<std::string>(root, "model_name", "");
    ctx.created_at = memochat::json::glaze_safe_get<int64_t>(root, "created_at", 0);
    ctx.last_active_at = memochat::json::glaze_safe_get<int64_t>(root, "last_active_at", 0);

    if (root.holds_object()) {
        const auto& obj = root.impl().get<memochat::json::object_t>();
        auto it = obj.find("messages");
        if (it != obj.end() && it->second.holds<memochat::json::array_t>()) {
            const auto& msgs = it->second.get<memochat::json::array_t>();
            for (const auto& jm : msgs) {
                ChatMessage m;
                m.msg_id = memochat::json::glaze_safe_get<std::string>(jm, "msg_id", "");
                m.role = memochat::json::glaze_safe_get<std::string>(jm, "role", "");
                m.content = memochat::json::glaze_safe_get<std::string>(jm, "content", "");
                m.created_at = memochat::json::glaze_safe_get<int64_t>(jm, "created_at", 0);
                ctx.messages.push_back(m);
            }
        }
    }
    return ctx;
}

