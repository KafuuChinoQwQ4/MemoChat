#pragma once
#include "common/proto/ai_message.pb.h"
#include <grpcpp/grpcpp.h>
#include <string>
#include <vector>
#include <ctime>
#include <memory>

class AISessionRepo
{
public:
    explicit AISessionRepo();
    ~AISessionRepo();

    std::string Create(int32_t uid, const std::string& model_type, const std::string& model_name);

    bool SoftDelete(int32_t uid, const std::string& session_id);

    bool UpdateTitle(int32_t uid, const std::string& session_id, const std::string& title);

    std::unique_ptr<ai::AISessionInfo> GetSession(int32_t uid, const std::string& session_id);

    std::vector<ai::AISessionInfo> ListByUid(int32_t uid);

    bool SaveMessage(const std::string& session_id,
                     int32_t uid,
                     const std::string& role,
                     const std::string& content,
                     const std::string& model_name,
                     int64_t tokens_used);

    std::vector<ai::AIMessage> GetMessages(int32_t uid, const std::string& session_id, int limit, int offset);

private:
    class Impl;
    std::unique_ptr<Impl> _impl;
};
