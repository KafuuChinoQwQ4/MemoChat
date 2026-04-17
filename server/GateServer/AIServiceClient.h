#pragma once
#include "common/proto/ai_message.grpc.pb.h"
#include <grpcpp/grpcpp.h>
#include <json/json.h>
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
        int64_t total_tokens)>;

    Json::Value Chat(int32_t uid, const std::string& session_id,
                     const std::string& content,
                     const std::string& model_type,
                     const std::string& model_name);

    void ChatStream(int32_t uid, const std::string& session_id,
                    const std::string& content,
                    const std::string& model_type,
                    const std::string& model_name,
                    ChunkCallback on_chunk,
                    Json::Value* out_result);

    Json::Value Smart(const std::string& feature_type,
                       const std::string& content,
                       const std::string& target_lang,
                       const std::string& context_json);

    Json::Value GetHistory(int32_t uid, const std::string& session_id,
                           int limit, int offset);

    Json::Value CreateSession(int32_t uid, const std::string& model_type,
                              const std::string& model_name);

    Json::Value ListSessions(int32_t uid, const std::string& model_type,
                              const std::string& model_name);

    Json::Value DeleteSession(int32_t uid, const std::string& session_id);

    Json::Value ListModels();

    Json::Value KbUpload(int32_t uid, const std::string& file_name,
                          const std::string& file_type,
                          const std::string& base64_content);

    Json::Value KbSearch(int32_t uid, const std::string& query, int top_k);

    Json::Value ListKb(int32_t uid);

    Json::Value DeleteKb(int32_t uid, const std::string& kb_id);

private:
    class Impl;
    std::unique_ptr<Impl> _impl;
};
