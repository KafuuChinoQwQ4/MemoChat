#pragma once

#include "r18/R18BrowserImportService.hpp"

#include <string>

namespace memochat::r18
{

// E-Hentai/ExHentai session validation and normalization.
// This service validates browser-imported cookies against upstream.
class R18EhentaiSessionService
{
public:
    static R18EhentaiSessionService& Instance();

    // Validate and normalize E-Hentai session cookies.
    // Returns true if the session is valid for at least E-Hentai.
    struct ValidationResult
    {
        bool ok = false;
        bool ehentai_access = false;
        bool exhentai_access = false;
        std::string normalized_cookie_header; // For persistence
        std::string error;
    };
    ValidationResult ValidateSession(const EhentaiSessionCookies& cookies);

    // Build a Cookie header from structured cookies
    std::string BuildCookieHeader(const EhentaiSessionCookies& cookies);

private:
    R18EhentaiSessionService() = default;
};

} // namespace memochat::r18
