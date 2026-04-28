#include "VarifyServer.h"

#include "VarifyServiceImpl.h"
#include "VarifyRedisMgr.h"
#include "RateLimiter.h"
#include "EmailSender.h"
#include "EmailTaskBus.h"
#include "ConfigMgr.h"
#include "logging/Logger.h"
#include "logging/TraceContext.h"

#include <random>
#include <regex>
#include <chrono>
#include <grpcpp/grpcpp.h>

namespace {

constexpr const char* CODE_PREFIX = "code_";

} // namespace

namespace varifyservice {

VarifyMetrics g_metrics;

VarifyServiceImpl::VarifyServiceImpl() {
    auto& cfg = ConfigMgr::Inst();

    auto code_len_str = cfg["VerifyCode"]["CodeLength"];
    if (!code_len_str.empty()) {
        try { config_.code_length = std::stoi(code_len_str); } catch (...) {}
    }

    auto ttl_str = cfg["VerifyCode"]["TTLSeconds"];
    if (!ttl_str.empty()) {
        try { config_.code_ttl_sec = std::stoi(ttl_str); } catch (...) {}
    }

    auto email_count_str = cfg["RateLimit"]["EmailMaxRequests"];
    if (!email_count_str.empty()) {
        try { config_.email_rate_limit_count = std::stoi(email_count_str); } catch (...) {}
    }

    auto email_window_str = cfg["RateLimit"]["EmailWindowSec"];
    if (!email_window_str.empty()) {
        try { config_.email_rate_limit_window_sec = std::stoi(email_window_str); } catch (...) {}
    }

    auto ip_count_str = cfg["RateLimit"]["IpMaxRequests"];
    if (!ip_count_str.empty()) {
        try { config_.ip_rate_limit_count = std::stoi(ip_count_str); } catch (...) {}
    }

    auto ip_window_str = cfg["RateLimit"]["IpWindowSec"];
    if (!ip_window_str.empty()) {
        try { config_.ip_rate_limit_window_sec = std::stoi(ip_window_str); } catch (...) {}
    }

    memolog::LogInfo("varifyservice.config", "VarifyServiceImpl initialized",
                   {{"code_length", std::to_string(config_.code_length)},
                    {"code_ttl_sec", std::to_string(config_.code_ttl_sec)},
                    {"email_rl_count", std::to_string(config_.email_rate_limit_count)},
                    {"email_rl_window", std::to_string(config_.email_rate_limit_window_sec)},
                    {"ip_rl_count", std::to_string(config_.ip_rate_limit_count)},
                    {"ip_rl_window", std::to_string(config_.ip_rate_limit_window_sec)}});
}

VarifyServiceImpl::~VarifyServiceImpl() {
}

std::string VarifyServiceImpl::GenerateVerifyCode() {
    std::random_device rd;
    std::uniform_int_distribution<int> dist(100000, 999999);
    return std::to_string(dist(rd));
}

bool VarifyServiceImpl::IsValidEmail(const std::string& email) {
    static const std::regex kEmailRegex(
        R"([a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,})");
    return std::regex_match(email, kEmailRegex);
}

std::string VarifyServiceImpl::ExtractPeerIP(grpc::ServerContext* context) {
    if (!context) return "";

    auto peer = context->peer();
    if (peer.empty()) return "";

    if (peer.find("ipv4:") == 0) {
        return peer.substr(5);
    }
    if (peer.find("ipv6:") == 0) {
        return peer.substr(5);
    }

    return peer;
}

bool VarifyServiceImpl::ResolveVerifyCode(const std::string& email, std::string* out_code) {
    const std::string canonical_key = std::string(CODE_PREFIX) + email;
    if (VarifyRedisMgr::Instance().Get(canonical_key, *out_code)) {
        return true;
    }

    if (VarifyRedisMgr::Instance().Get(email, *out_code)) {
        int64_t ttl = VarifyRedisMgr::Instance().TTL(email);
        if (ttl <= 0) ttl = config_.code_ttl_sec;
        VarifyRedisMgr::Instance().SetEx(canonical_key, *out_code, static_cast<int>(ttl));
        VarifyRedisMgr::Instance().Del(email);
        memolog::LogWarn("varify.code.legacy_key_migrated", "migrated legacy verify-code key",
                        {{"email", email}});
        return true;
    }

    return false;
}

bool VarifyServiceImpl::StoreVerifyCode(const std::string& email, const std::string& code, int ttl_sec) {
    const std::string canonical_key = std::string(CODE_PREFIX) + email;
    return VarifyRedisMgr::Instance().SetEx(canonical_key, code, ttl_sec);
}

bool VarifyServiceImpl::SendEmail(const std::string& email, const std::string& code) {
    auto& task_bus = EmailTaskBus::Instance();
    if (task_bus.IsHealthy()) {
        bool published = task_bus.PublishVerifyDeliveryTask(email, code, "");
        if (published) {
            g_metrics.email_sent_async.fetch_add(1, std::memory_order_relaxed);
            return true;
        }
        memolog::LogWarn("varify.email.rabbitmq_fallback", "RabbitMQ publish failed, falling back to sync send",
                        {{"email", email}});
        g_metrics.email_rabbitmq_fallback.fetch_add(1, std::memory_order_relaxed);
    } else {
        memolog::LogWarn("varify.email.rabbitmq_unhealthy", "RabbitMQ unhealthy, falling back to sync send",
                        {{"email", email}});
        g_metrics.email_rabbitmq_fallback.fetch_add(1, std::memory_order_relaxed);
    }

    bool ok = EmailSender::Send(email, code);
    if (ok) {
        g_metrics.email_sent_sync.fetch_add(1, std::memory_order_relaxed);
    } else {
        g_metrics.email_failed.fetch_add(1, std::memory_order_relaxed);
    }
    return ok;
}

grpc::Status VarifyServiceImpl::GetVarifyCode(grpc::ServerContext* context,
                                               const message::GetVarifyReq* request,
                                               message::GetVarifyRsp* reply) {
    g_metrics.requests_total.fetch_add(1, std::memory_order_relaxed);

    const std::string email = request->email();

    std::string trace_id = memolog::TraceContext::GetTraceId();
    if (trace_id.empty()) {
        trace_id = std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    }

    memolog::LogInfo("varify.get_code.request", "get verify code request",
                    {{"email", email},
                     {"trace_id", trace_id},
                     {"module", "grpc"}});

    if (!IsValidEmail(email)) {
        g_metrics.invalid_email.fetch_add(1, std::memory_order_relaxed);
        memolog::LogWarn("varify.get_code.invalid_email", "invalid email format",
                        {{"email", email}, {"trace_id", trace_id}});
        reply->set_error(static_cast<int>(VarifyError::InvalidEmail));
        return grpc::Status::OK;
    }

    const std::string ip = ExtractPeerIP(context);

    auto email_result = RateLimiter::CheckEmail(email,
        config_.email_rate_limit_window_sec,
        config_.email_rate_limit_count);
    if (email_result == RateLimiter::Result::RateLimited) {
        g_metrics.rate_limited.fetch_add(1, std::memory_order_relaxed);
        memolog::LogWarn("varify.get_code.rate_limited_email", "rate limited by email",
                        {{"email", email}, {"trace_id", trace_id}});
        reply->set_error(static_cast<int>(VarifyError::RateLimited));
        return grpc::Status::OK;
    }

    if (!ip.empty()) {
        auto ip_result = RateLimiter::CheckIP(ip,
            config_.ip_rate_limit_window_sec,
            config_.ip_rate_limit_count);
        if (ip_result == RateLimiter::Result::RateLimited) {
            g_metrics.rate_limited.fetch_add(1, std::memory_order_relaxed);
            memolog::LogWarn("varify.get_code.rate_limited_ip", "rate limited by IP",
                            {{"ip", ip}, {"trace_id", trace_id}});
            reply->set_error(static_cast<int>(VarifyError::RateLimited));
            return grpc::Status::OK;
        }
    }

    std::string code;
    bool code_exists = ResolveVerifyCode(email, &code);

    if (!code_exists) {
        code = GenerateVerifyCode();
        bool stored = StoreVerifyCode(email, code, config_.code_ttl_sec);
        if (!stored) {
            g_metrics.requests_failed.fetch_add(1, std::memory_order_relaxed);
            memolog::LogError("varify.get_code.redis_failed", "failed to store verify code in Redis",
                            {{"email", email}, {"trace_id", trace_id}});
            reply->set_error(static_cast<int>(VarifyError::RedisErr));
            return grpc::Status::OK;
        }
        memolog::LogInfo("varify.get_code.new_code", "generated and stored new verify code",
                        {{"email", email}, {"trace_id", trace_id}});
    } else {
        memolog::LogInfo("varify.get_code.existing_code", "reusing existing verify code",
                        {{"email", email}, {"trace_id", trace_id}});
    }

    bool synthetic_email = IsSyntheticLoadtestEmail(email);

    if (!synthetic_email) {
        bool email_ok = SendEmail(email, code);
        if (!email_ok) {
            memolog::LogWarn("varify.get_code.email_failed", "failed to send email",
                            {{"email", email}, {"trace_id", trace_id}});
        }
    } else {
        memolog::LogInfo("varify.get_code.synthetic_skipped", "skipping email for synthetic loadtest account",
                        {{"email", email}, {"trace_id", trace_id}});
    }

    g_metrics.requests_success.fetch_add(1, std::memory_order_relaxed);

    reply->set_email(email);
    reply->set_error(static_cast<int>(VarifyError::Success));
    reply->set_code(code);

    return grpc::Status::OK;
}

} // namespace varifyservice
