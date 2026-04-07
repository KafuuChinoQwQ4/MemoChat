#pragma once

#include <grpcpp/grpcpp.h>
#include "common/proto/message.grpc.pb.h"
#include <atomic>
#include <mutex>
#include <memory>
#include <string>
#include <unordered_map>

namespace varifyservice {

inline bool IsSyntheticLoadtestEmail(const std::string& email) {
    if (email.size() < 14) return false;
    return email.compare(email.size() - 14, 14, "@loadtest.local") == 0;
}

enum class VarifyError {
    Success = 0,
    RedisErr = 1,
    Exception = 2,
    RateLimited = 3,
    InvalidEmail = 4,
};

struct VarifyMetrics {
    std::atomic<uint64_t> requests_total{0};
    std::atomic<uint64_t> requests_success{0};
    std::atomic<uint64_t> requests_failed{0};
    std::atomic<uint64_t> rate_limited{0};
    std::atomic<uint64_t> invalid_email{0};
    std::atomic<uint64_t> email_sent_sync{0};
    std::atomic<uint64_t> email_sent_async{0};
    std::atomic<uint64_t> email_failed{0};
    std::atomic<uint64_t> email_rabbitmq_fallback{0};
};

extern VarifyMetrics g_metrics;

class VarifyServiceImpl final : public message::VarifyService::Service {
public:
    VarifyServiceImpl();
    ~VarifyServiceImpl() override;

    grpc::Status GetVarifyCode(grpc::ServerContext* context,
                                const message::GetVarifyReq* request,
                                message::GetVarifyRsp* reply) override;

private:
    std::string GenerateVerifyCode();
    bool IsValidEmail(const std::string& email);

    bool CheckRateLimitByEmail(const std::string& email);
    bool CheckRateLimitByIP(const std::string& ip);

    bool ResolveVerifyCode(const std::string& email, std::string* out_code);
    bool StoreVerifyCode(const std::string& email, const std::string& code, int ttl_sec);

    bool SendEmail(const std::string& email, const std::string& code);

    std::string ExtractPeerIP(grpc::ServerContext* context);

    struct {
        int code_length = 6;
        int code_ttl_sec = 600;
        int email_rate_limit_count = 3;
        int email_rate_limit_window_sec = 60;
        int ip_rate_limit_count = 10;
        int ip_rate_limit_window_sec = 60;
    } config_;
};

} // namespace varifyservice
