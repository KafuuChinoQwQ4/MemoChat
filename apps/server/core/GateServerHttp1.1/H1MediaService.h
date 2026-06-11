#pragma once

#include <memory>
#include <string>
#include <string_view>

class H1LogicSystem;
class H1Connection;

namespace memochat::json
{
class JsonValue;
}

class H1MediaService
{
public:
    static void RegisterRoutes(H1LogicSystem& logic);

private:
    static std::string RequestBody(const std::shared_ptr<H1Connection>& connection);
    static std::string HeaderValue(const std::shared_ptr<H1Connection>& connection, std::string_view name);
    static int HeaderIntValue(const std::shared_ptr<H1Connection>& connection, std::string_view name, int fallback = 0);
    static bool IsJsonRequest(const std::shared_ptr<H1Connection>& connection);
    static void WriteJsonResponse(const std::shared_ptr<H1Connection>& connection,
                                  const memochat::json::JsonValue& root);
    static std::string QueryValue(const std::shared_ptr<H1Connection>& connection, const std::string& key);
    static bool ParseJsonRequest(const std::shared_ptr<H1Connection>& connection, memochat::json::JsonValue& root);
};
