#pragma once
#include <grpcpp/grpcpp.h>
#include "json/GlazeCompat.h"
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
        int64_t total_tokens,
        const std::string& trace_id,
        const std::string& skill,
        const std::string& feedback_summary,
        const std::string& observations_json,
        const std::string& events_json)>;

    grpc::Status Chat(int32_t uid, const std::string& session_id,
                      const std::string& content,
                      const std::string& model_type,
                      const std::string& model_name,
                      const std::string& metadata_json,
                      memochat::json::JsonValue* out_result);

    grpc::Status ChatStream(int32_t uid, const std::string& session_id,
                           const std::string& content,
                           const std::string& model_type,
                           const std::string& model_name,
                           const std::string& metadata_json,
                           ChunkCallback on_chunk,
                           memochat::json::JsonValue* out_result);

    grpc::Status Smart(const std::string& feature_type,
                       const std::string& content,
                       const std::string& target_lang,
                       const std::string& context_json,
                       memochat::json::JsonValue* out_result);

    grpc::Status KbUpload(int32_t uid, const std::string& file_name,
                          const std::string& file_type,
                          const std::string& base64_content,
                          memochat::json::JsonValue* out_result);

    grpc::Status KbSearch(int32_t uid, const std::string& query,
                          int top_k,
                          memochat::json::JsonValue* out_result);

    grpc::Status ListModels(memochat::json::JsonValue* out_result);

    grpc::Status RegisterApiProvider(const std::string& provider_name,
                                     const std::string& base_url,
                                     const std::string& api_key,
                                     const std::string& adapter,
                                     memochat::json::JsonValue* out_result);

    grpc::Status KbList(int32_t uid, memochat::json::JsonValue* out_result);

    grpc::Status KbDelete(int32_t uid, const std::string& kb_id,
                          memochat::json::JsonValue* out_result);

private:
    class Impl;
    std::unique_ptr<Impl> _impl;
};
