#pragma once
#include "common/proto/ai_message.grpc.pb.h"
#include <grpcpp/grpcpp.h>
#include "json/GlazeCompat.h"
#include <string>
#include <memory>
#include <functional>

class AIServiceClient {
public:
    explicit AIServiceClient();
    ~AIServiceClient();

    using ChunkCallback = std::function<void(
        const std::string& chunk,
        bool is_final,
        const std::string& msg_id,
        int64_t total_tokens,
        const std::string& trace_id,
        const std::string& skill,
        const std::string& feedback_summary,
        const std::string& observations_json,
        const std::string& events_json)>;

    memochat::json::JsonValue Chat(int32_t uid, const std::string& session_id,
                     const std::string& content,
                     const std::string& model_type,
                     const std::string& model_name,
                     const std::string& metadata_json = "{}");

    void ChatStream(int32_t uid, const std::string& session_id,
                    const std::string& content,
                    const std::string& model_type,
                    const std::string& model_name,
                    const std::string& metadata_json,
                    ChunkCallback on_chunk,
                    memochat::json::JsonValue* out_result);

    memochat::json::JsonValue Smart(const std::string& feature_type,
                       const std::string& content,
                       const std::string& target_lang,
                       const std::string& context_json);

    memochat::json::JsonValue GetHistory(int32_t uid, const std::string& session_id,
                           int limit, int offset);

    memochat::json::JsonValue CreateSession(int32_t uid, const std::string& model_type,
                              const std::string& model_name);

    memochat::json::JsonValue ListSessions(int32_t uid, const std::string& model_type,
                              const std::string& model_name);

    memochat::json::JsonValue DeleteSession(int32_t uid, const std::string& session_id);

    memochat::json::JsonValue ListModels();

    memochat::json::JsonValue RegisterApiProvider(const std::string& provider_name,
                                                  const std::string& base_url,
                                                  const std::string& api_key,
                                                  const std::string& adapter);

    memochat::json::JsonValue KbUpload(int32_t uid, const std::string& file_name,
                          const std::string& file_type,
                          const std::string& base64_content);

    memochat::json::JsonValue KbSearch(int32_t uid, const std::string& query, int top_k);

    memochat::json::JsonValue ListKb(int32_t uid);

    memochat::json::JsonValue DeleteKb(int32_t uid, const std::string& kb_id);

private:
    class Impl;
    std::unique_ptr<Impl> _impl;
};
