#pragma once
#include <grpcpp/grpcpp.h>
#include <json/json.h>
#include <string>
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

    // Chat
    grpc::Status Chat(int32_t uid, const std::string& session_id,
                      const std::string& content,
                      const std::string& model_type,
                      const std::string& model_name,
                      Json::Value* out_result);

    // 流式 Chat
    grpc::Status ChatStream(int32_t uid, const std::string& session_id,
                           const std::string& content,
                           const std::string& model_type,
                           const std::string& model_name,
                           ChunkCallback on_chunk,
                           Json::Value* out_result);

    // Smart 功能
    grpc::Status Smart(const std::string& feature_type,
                       const std::string& content,
                       const std::string& target_lang,
                       const std::string& context_json,
                       Json::Value* out_result);

    // 知识库
    grpc::Status KbUpload(int32_t uid, const std::string& file_name,
                          const std::string& file_type,
                          const std::string& base64_content,
                          Json::Value* out_result);

    grpc::Status KbSearch(int32_t uid, const std::string& query,
                          int top_k,
                          Json::Value* out_result);

private:
    class Impl;
    std::unique_ptr<Impl> _impl;
};
