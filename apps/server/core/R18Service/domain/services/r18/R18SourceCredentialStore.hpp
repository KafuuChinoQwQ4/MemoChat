#pragma once

#include "json/GlazeCompat.hpp"

#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace memochat::r18
{

struct R18SourceCredential
{
    std::string source_id;
    std::string username;
    std::string password;
    std::string session_token;
    std::string session_cookie;
    std::string status = "not_configured"; // not_configured | configured | authenticated | error
    std::string message;
    int64_t updated_at_ms = 0;
};

// Per-MemoChat-user credential vault for external comic sources.
// Stored under data/r18/credentials/<uid>.json (local runtime data, not shared).
class R18SourceCredentialStore
{
public:
    static R18SourceCredentialStore& Instance();

    // Public projection never returns password/session secrets.
    memochat::json::JsonValue ListPublicAccounts(int uid);
    std::optional<R18SourceCredential> Get(int uid, const std::string& source_id);
    bool UpsertLogin(int uid,
                     const std::string& source_id,
                     const std::string& username,
                     const std::string& password,
                     std::string* error);
    bool Clear(int uid, const std::string& source_id, std::string* error);
    bool UpdateSession(int uid,
                       const std::string& source_id,
                       const std::string& session_token,
                       const std::string& session_cookie,
                       const std::string& status,
                       const std::string& message,
                       std::string* error);
    bool MarkError(int uid, const std::string& source_id, const std::string& message, std::string* error);

private:
    R18SourceCredentialStore();

    std::filesystem::path ResolveRoot() const;
    std::filesystem::path PathForUid(int uid) const;
    void EnsureRootLocked();
    void LoadUidLocked(int uid);
    void SaveUidLocked(int uid);
    memochat::json::JsonValue ToPublicJson(const R18SourceCredential& cred) const;

    mutable std::mutex mu_;
    std::filesystem::path root_;
    std::unordered_map<int, std::unordered_map<std::string, R18SourceCredential>> by_uid_;
};

} // namespace memochat::r18
