#pragma once
#include <string>
#include <vector>
#include <ctime>

struct ChatMessage {
    std::string msg_id;
    std::string role;      // "user" or "assistant"
    std::string content;
    int64_t created_at;
};

class ConversationContext {
public:
    ConversationContext() = default;
    ConversationContext(const std::string& session_id, int32_t uid,
                       const std::string& model_type, const std::string& model_name);

    std::string session_id;
    int32_t uid = 0;
    std::string model_type;
    std::string model_name;
    std::vector<ChatMessage> messages;  // sliding window (max 20)
    int64_t created_at = 0;
    int64_t last_active_at = 0;

    void AddUserMessage(const std::string& content);
    void AddAssistantMessage(const std::string& content);
    void Prune(size_t max_size = 20);

    std::string ToJson() const;
    static ConversationContext FromJson(const std::string& json);
};
