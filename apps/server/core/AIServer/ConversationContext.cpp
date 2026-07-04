#include "ConversationContext.hpp"

#include "AIServiceJsonDtos.hpp"

import memochat.ai.conversation_context_algorithms;

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/random_generator.hpp>
#include <chrono>
#include <stdexcept>
#include <utility>

ConversationContext::ConversationContext(const std::string& sid,
                                         int32_t u,
                                         const std::string& mt,
                                         const std::string& mn)
    : session_id(sid)
    , uid(u)
    , model_type(mt)
    , model_name(mn)
{
    auto now =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count();
    created_at = now;
    last_active_at = now;
}

void ConversationContext::AddUserMessage(const std::string& content)
{
    ChatMessage m;
    m.msg_id = boost::uuids::to_string(boost::uuids::random_generator()());
    m.role = memochat::ai::conversation_context::modules::UserRole();
    m.content = content;
    m.created_at =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count();
    messages.push_back(std::move(m));
    last_active_at = m.created_at;
}

void ConversationContext::AddAssistantMessage(const std::string& content)
{
    ChatMessage m;
    m.msg_id = boost::uuids::to_string(boost::uuids::random_generator()());
    m.role = memochat::ai::conversation_context::modules::AssistantRole();
    m.content = content;
    m.created_at =
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count();
    messages.push_back(std::move(m));
    last_active_at = m.created_at;
}

void ConversationContext::Prune(size_t max_size)
{
    if (memochat::ai::conversation_context::modules::ShouldPruneMessages(messages.size(), max_size))
    {
        messages.erase(messages.begin(), messages.end() - max_size);
    }
}

std::string ConversationContext::ToJson() const
{
    std::string body;
    std::string error;
    if (!ai_service_json_mapper::EncodeConversationContextJson(
            ai_service_json_mapper::ToConversationContextJsonDto(*this),
            &body,
            &error))
    {
        throw std::runtime_error("Failed to encode ConversationContext JSON: " + error);
    }
    return body;
}

ConversationContext ConversationContext::FromJson(const std::string& json)
{
    ai_service_json_mapper::ConversationContextJsonDto dto;
    if (!ai_service_json_mapper::DecodeConversationContextJson(json, &dto))
    {
        throw std::runtime_error("Failed to parse ConversationContext JSON");
    }
    return ai_service_json_mapper::FromConversationContextJsonDto(dto);
}
