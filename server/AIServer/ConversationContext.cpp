#include "ConversationContext.h"
#include "logging/Logger.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/random_generator.hpp>
#include <chrono>
#include <json/json.h>

using namespace std::chrono;

ConversationContext::ConversationContext(const std::string& sid, int32_t u,
                                         const std::string& mt, const std::string& mn)
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
        messages.erase(messages.begin(),
                      messages.end() - max_size);
    }
}

std::string ConversationContext::ToJson() const {
    Json::Value root;
    root["session_id"] = session_id;
    root["uid"] = uid;
    root["model_type"] = model_type;
    root["model_name"] = model_name;
    root["created_at"] = created_at;
    root["last_active_at"] = last_active_at;

    Json::Value msgs(Json::arrayValue);
    for (const auto& m : messages) {
        Json::Value jm;
        jm["msg_id"] = m.msg_id;
        jm["role"] = m.role;
        jm["content"] = m.content;
        jm["created_at"] = m.created_at;
        msgs.append(jm);
    }
    root["messages"] = msgs;

    Json::StreamWriterBuilder builder;
    return Json::writeString(builder, root);
}

ConversationContext ConversationContext::FromJson(const std::string& json) {
    Json::CharReaderBuilder builder;
    Json::Value root;
    std::string err;
    std::istringstream(json) >> root;

    ConversationContext ctx;
    ctx.session_id = root["session_id"].asString();
    ctx.uid = root["uid"].asInt();
    ctx.model_type = root["model_type"].asString();
    ctx.model_name = root["model_name"].asString();
    ctx.created_at = root["created_at"].asInt64();
    ctx.last_active_at = root["last_active_at"].asInt64();

    for (const auto& jm : root["messages"]) {
        ChatMessage m;
        m.msg_id = jm["msg_id"].asString();
        m.role = jm["role"].asString();
        m.content = jm["content"].asString();
        m.created_at = jm["created_at"].asInt64();
        ctx.messages.push_back(m);
    }
    return ctx;
}
