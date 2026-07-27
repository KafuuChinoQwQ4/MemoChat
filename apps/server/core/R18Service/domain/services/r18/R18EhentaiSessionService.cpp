#include "r18/R18EhentaiSessionService.hpp"
#include "r18/R18EhentaiAdapter.hpp"

#include <sstream>

namespace memochat::r18
{

R18EhentaiSessionService& R18EhentaiSessionService::Instance()
{
    static R18EhentaiSessionService service;
    return service;
}

std::string R18EhentaiSessionService::BuildCookieHeader(const EhentaiSessionCookies& cookies)
{
    std::ostringstream oss;
    bool first = true;

    auto append = [&](const std::string& name, const std::string& value)
    {
        if (!value.empty())
        {
            if (!first)
                oss << "; ";
            oss << name << "=" << value;
            first = false;
        }
    };

    append("ipb_member_id", cookies.ipb_member_id);
    append("ipb_pass_hash", cookies.ipb_pass_hash);
    append("igneous", cookies.igneous);
    append("sk", cookies.sk);

    return oss.str();
}

R18EhentaiSessionService::ValidationResult
R18EhentaiSessionService::ValidateSession(const EhentaiSessionCookies& cookies)
{
    ValidationResult result;

    // Basic structural validation
    if (cookies.ipb_member_id.empty() || cookies.ipb_pass_hash.empty())
    {
        result.error = "missing_required_cookies";
        return result;
    }

    result.normalized_cookie_header = BuildCookieHeader(cookies);
    result.ehentai_access = true;
    if (!cookies.igneous.empty())
    {
        std::string validation_error;
        if (!ExhentaiValidateSession(result.normalized_cookie_header, &validation_error))
        {
            result.error = validation_error.empty() ? "exhentai_session_invalid" : validation_error;
            return result;
        }
        result.exhentai_access = true;
    }
    result.ok = true;

    return result;
}

} // namespace memochat::r18
