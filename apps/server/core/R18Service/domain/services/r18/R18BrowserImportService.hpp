#pragma once

#include <cstdint>
#include <string>

namespace memochat::r18
{

struct EhentaiSessionCookies
{
    std::string ipb_member_id;
    std::string ipb_pass_hash;
    std::string igneous;
    std::string sk;
};

enum class BrowserImportStatus
{
    Pending,
    Authenticated,
    Failed,
    Expired
};

class R18BrowserImportService
{
public:
    static R18BrowserImportService& Instance();

    struct StartResult
    {
        bool ok = false;
        std::string import_id;
        std::string ticket;
        int64_t expires_at_ms = 0;
        std::string error;
    };
    StartResult StartImport(int uid, const std::string& source_id, const std::string& client_kind);

    struct CompleteResult
    {
        bool ok = false;
        int uid = 0;
        std::string import_id;
        std::string source_id;
        std::string source_family;
        std::string error;
    };
    CompleteResult CompleteImport(const std::string& ticket, const EhentaiSessionCookies& cookies);

    struct StatusResult
    {
        bool ok = false;
        BrowserImportStatus status = BrowserImportStatus::Expired;
        std::string message;
        std::string error;
    };
    StatusResult GetStatus(int uid, const std::string& import_id);
    void SetStatus(int uid, const std::string& import_id, BrowserImportStatus status, const std::string& message);

    // No-op; Redis TTL handles expiry.
    void ExpireStaleTickets();

private:
    R18BrowserImportService();
    void
    UpdateStatusInRedis(int uid, const std::string& import_id, const std::string& status, const std::string& message);
};

} // namespace memochat::r18
