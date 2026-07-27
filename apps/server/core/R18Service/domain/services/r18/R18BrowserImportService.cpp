#include "r18/R18BrowserImportService.hpp"

#include "RedisMgr.hpp"
#include "json/GlazeCompat.hpp"

#include <openssl/evp.h>
#include <openssl/rand.h>

#include <array>
#include <cerrno>
#include <chrono>
#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace memochat::r18
{
namespace
{

int64_t NowMs()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
        .count();
}

constexpr int kTicketTtlSec = 120;
constexpr int64_t kTicketTtlMs = 120'000;
constexpr int kRateLimitTtlSec = 12;
constexpr int kStatusTtlSec = 420;

// --- Crypto ------------------------------------------------------------------

// 256-bit CSPRNG, returned as lowercase hex.  Returns "" on RAND_bytes failure.
std::string SecureRandomHex(std::size_t bytes)
{
    std::vector<unsigned char> buf(bytes);
    if (RAND_bytes(buf.data(), static_cast<int>(bytes)) != 1)
        return {};
    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (unsigned char b : buf)
        oss << std::setw(2) << static_cast<unsigned>(b);
    return oss.str();
}

// OpenSSL EVP SHA-256 hex digest.  Returns "" on failure.
std::string SHA256Hex(const std::string& input)
{
    unsigned char digest[EVP_MAX_MD_SIZE] = {};
    unsigned int digest_len = 0;

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    if (!ctx)
        return {};
    const bool ok = EVP_DigestInit_ex(ctx, EVP_sha256(), nullptr) == 1 &&
                    EVP_DigestUpdate(ctx, input.data(), input.size()) == 1 &&
                    EVP_DigestFinal_ex(ctx, digest, &digest_len) == 1;
    EVP_MD_CTX_free(ctx);
    if (!ok)
        return {};

    std::ostringstream oss;
    oss << std::hex << std::setfill('0');
    for (unsigned int i = 0; i < digest_len; ++i)
        oss << std::setw(2) << static_cast<unsigned>(digest[i]);
    return oss.str();
}

// --- Redis key helpers -------------------------------------------------------

std::string TicketKey(const std::string& import_id)
{
    return "r18:ticket:digest:" + import_id;
}
std::string TicketMetaKey(const std::string& digest)
{
    return "r18:ticket:meta:" + digest;
}
std::string StatusKey(const std::string& import_id)
{
    return "r18:ticket:status:" + import_id;
}
std::string RateLimitKey(int uid, const std::string& sf)
{
    return "r18:ticket:ratelimit:" + std::to_string(uid) + ":" + sf;
}

std::string MapSourceToFamily(const std::string& source_id)
{
    if (source_id == "ehentai.official" || source_id == "exhentai.official")
        return "ehentai";
    return source_id;
}

// --- Minimal JSON serialisation for ticket metadata -------------------------

std::string SerializeMeta(int uid,
                          const std::string& source_id,
                          const std::string& source_family,
                          const std::string& client_kind,
                          const std::string& import_id,
                          int64_t expires_at_ms)
{
    return "{\"uid\":" + std::to_string(uid) + ",\"source_id\":\"" + source_id + "\"" + ",\"source_family\":\"" +
           source_family + "\"" + ",\"client_kind\":\"" + client_kind + "\"" + ",\"import_id\":\"" + import_id + "\"" +
           ",\"expires_at_ms\":" + std::to_string(expires_at_ms) + "}";
}

std::string SerializeStatus(int uid, const std::string& status, const std::string& message)
{
    memochat::json::JsonValue value;
    value["uid"] = uid;
    value["status"] = status;
    value["message"] = message;
    return memochat::json::glaze_stringify(value);
}

// Extract a JSON string or number field value from the hand-rolled meta above.
std::string ExtractField(const std::string& json, const std::string& key)
{
    const std::string pattern = "\"" + key + "\":";
    const auto pos0 = json.find(pattern);
    if (pos0 == std::string::npos)
        return {};
    const std::size_t vstart = pos0 + pattern.size();
    if (vstart >= json.size())
        return {};
    if (json[vstart] == '"')
    {
        const auto end = json.find('"', vstart + 1);
        return (end == std::string::npos) ? "" : json.substr(vstart + 1, end - vstart - 1);
    }
    // numeric
    const auto end = json.find_first_of(",}", vstart);
    return (end == std::string::npos) ? json.substr(vstart) : json.substr(vstart, end - vstart);
}

int64_t ExtractInt64(const std::string& json, const std::string& key)
{
    const std::string v = ExtractField(json, key);
    if (v.empty())
        return 0;
    char* end = nullptr;
    errno = 0;
    const long long r = std::strtoll(v.c_str(), &end, 10);
    if (errno != 0 || end == v.c_str())
        return 0;
    return static_cast<int64_t>(r);
}

int ExtractInt(const std::string& json, const std::string& key)
{
    return static_cast<int>(ExtractInt64(json, key));
}

} // namespace

// =============================================================================

R18BrowserImportService& R18BrowserImportService::Instance()
{
    static R18BrowserImportService svc;
    return svc;
}

R18BrowserImportService::R18BrowserImportService() = default;

// StartImport — generate entropy-backed ticket, store in Redis with TTL.
R18BrowserImportService::StartResult
R18BrowserImportService::StartImport(int uid, const std::string& source_id, const std::string& client_kind)
{
    StartResult result;

    if (uid <= 0)
    {
        result.error = "invalid_uid";
        return result;
    }
    const std::string source_family = MapSourceToFamily(source_id);
    if (source_family != "ehentai")
    {
        result.error = "unsupported_source";
        return result;
    }
    if (client_kind != "web_extension" && client_kind != "qt_webengine")
    {
        result.error = "invalid_client_kind";
        return result;
    }

    auto redis = RedisMgr::GetInstance();
    if (!redis || !redis->Ready())
    {
        result.error = "cache_unavailable";
        return result;
    }

    // Rate-limit: one ticket per 12 s per uid:source_family.
    std::string rate_val;
    if (redis->Get(RateLimitKey(uid, source_family), rate_val) && !rate_val.empty())
    {
        result.error = "rate_limited";
        return result;
    }

    // Generate import_id (16 bytes / 128 bits) and raw ticket (32 bytes / 256 bits).
    const std::string import_id = "imp_" + SecureRandomHex(16);
    const std::string ticket = "tkt_" + SecureRandomHex(32);
    if (import_id == "imp_" || ticket == "tkt_")
    {
        result.error = "entropy_failure";
        return result;
    }

    const std::string digest = SHA256Hex(ticket);
    if (digest.empty())
    {
        result.error = "crypto_failure";
        return result;
    }

    const int64_t now = NowMs();
    const int64_t expires = now + kTicketTtlMs;

    const std::string meta = SerializeMeta(uid, source_id, source_family, client_kind, import_id, expires);

    // Three Redis writes, then rate-limit marker.
    const bool ok1 = redis->SetEx(TicketKey(import_id), digest, kTicketTtlSec);
    const bool ok2 = redis->SetEx(TicketMetaKey(digest), meta, kTicketTtlSec);
    const bool ok3 = redis->SetEx(StatusKey(import_id), SerializeStatus(uid, "pending", ""), kStatusTtlSec);
    redis->SetEx(RateLimitKey(uid, source_family), "1", kRateLimitTtlSec);

    if (!ok1 || !ok2 || !ok3)
    {
        result.error = "cache_write_failure";
        return result;
    }

    result.ok = true;
    result.import_id = import_id;
    result.ticket = ticket; // raw ticket returned exactly once
    result.expires_at_ms = expires;
    return result;
}

// CompleteImport — atomically consume ticket and validate cookies.
R18BrowserImportService::CompleteResult R18BrowserImportService::CompleteImport(const std::string& ticket,
                                                                                const EhentaiSessionCookies& cookies)
{
    CompleteResult result;

    if (cookies.ipb_member_id.empty() || cookies.ipb_pass_hash.empty())
    {
        result.error = "missing_required_cookies";
        return result;
    }

    constexpr std::size_t kMaxCookieLen = 512;
    if (cookies.ipb_member_id.size() > kMaxCookieLen || cookies.ipb_pass_hash.size() > kMaxCookieLen ||
        cookies.igneous.size() > kMaxCookieLen || cookies.sk.size() > kMaxCookieLen)
    {
        result.error = "cookie_value_too_large";
        return result;
    }

    auto has_control = [](const std::string& s)
    {
        for (unsigned char c : s)
            if (c < 0x20u && c != '\t')
                return true;
        return false;
    };
    if (has_control(cookies.ipb_member_id) || has_control(cookies.ipb_pass_hash) || has_control(cookies.igneous) ||
        has_control(cookies.sk))
    {
        result.error = "invalid_cookie_format";
        return result;
    }

    const std::string digest = SHA256Hex(ticket);
    if (digest.empty())
    {
        result.error = "crypto_failure";
        return result;
    }

    auto redis = RedisMgr::GetInstance();
    if (!redis || !redis->Ready())
    {
        result.error = "cache_unavailable";
        return result;
    }

    // Atomic GET + DEL via Lua prevents concurrent completion of one ticket.
    const std::string meta_key = TicketMetaKey(digest);
    static const std::string kConsumeScript = "local v = redis.call('GET', KEYS[1]) "
                                              "if v == false then return '' end "
                                              "redis.call('DEL', KEYS[1]) "
                                              "return v";
    std::string meta;
    if (!redis->EvalString(kConsumeScript, {meta_key}, {}, meta) || meta.empty())
    {
        result.error = "invalid_or_consumed_ticket";
        return result;
    }

    const int uid = ExtractInt(meta, "uid");
    if (uid <= 0)
    {
        result.error = "ticket_uid_invalid";
        return result;
    }

    const int64_t expires_at_ms = ExtractInt64(meta, "expires_at_ms");
    if (expires_at_ms > 0 && NowMs() > expires_at_ms)
    {
        UpdateStatusInRedis(uid, ExtractField(meta, "import_id"), "expired", "Ticket expired");
        result.error = "ticket_expired";
        return result;
    }

    const std::string import_id = ExtractField(meta, "import_id");
    redis->Del(TicketKey(import_id));

    result.ok = true;
    result.uid = uid;
    result.import_id = import_id;
    result.source_id = ExtractField(meta, "source_id");
    result.source_family = ExtractField(meta, "source_family");
    return result;
}

void R18BrowserImportService::SetStatus(int uid,
                                        const std::string& import_id,
                                        BrowserImportStatus status,
                                        const std::string& message)
{
    const char* value = "pending";
    switch (status)
    {
        case BrowserImportStatus::Authenticated:
            value = "authenticated";
            break;
        case BrowserImportStatus::Failed:
            value = "failed";
            break;
        case BrowserImportStatus::Expired:
            value = "expired";
            break;
        case BrowserImportStatus::Pending:
            break;
    }
    UpdateStatusInRedis(uid, import_id, value, message);
}

// GetStatus — uid-bound import status lookup.
R18BrowserImportService::StatusResult R18BrowserImportService::GetStatus(int uid, const std::string& import_id)
{
    StatusResult result;

    auto redis = RedisMgr::GetInstance();
    if (!redis || !redis->Ready())
    {
        result.error = "cache_unavailable";
        return result;
    }

    std::string status_json;
    if (!redis->Get(StatusKey(import_id), status_json) || status_json.empty())
    {
        result.error = "import_not_found";
        return result;
    }

    const int status_uid = ExtractInt(status_json, "uid");
    if (uid <= 0 || status_uid != uid)
    {
        result.error = "import_not_found";
        return result;
    }

    const std::string st = ExtractField(status_json, "status");
    const std::string msg = ExtractField(status_json, "message");

    BrowserImportStatus parsed = BrowserImportStatus::Pending;
    if (st == "authenticated")
        parsed = BrowserImportStatus::Authenticated;
    else if (st == "failed")
        parsed = BrowserImportStatus::Failed;
    else if (st == "expired")
        parsed = BrowserImportStatus::Expired;

    result.ok = true;
    result.status = parsed;
    result.message = msg;
    return result;
}

void R18BrowserImportService::UpdateStatusInRedis(int uid,
                                                  const std::string& import_id,
                                                  const std::string& status,
                                                  const std::string& message)
{
    if (uid <= 0 || import_id.empty())
        return;
    auto redis = RedisMgr::GetInstance();
    if (!redis || !redis->Ready())
        return;

    redis->SetEx(StatusKey(import_id), SerializeStatus(uid, status, message), kStatusTtlSec);
}

void R18BrowserImportService::ExpireStaleTickets()
{
    // Redis TTL handles all expiry automatically; nothing to do in-process.
}

} // namespace memochat::r18
