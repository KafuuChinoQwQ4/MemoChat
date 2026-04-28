#pragma once

#include <functional>
#include <memory>
#include <string>
#include "json/GlazeCompat.h"

class HttpConnection;

class GateHttpJsonSupport {
public:
    static bool ParseJsonBody(std::shared_ptr<HttpConnection> connection, memochat::json::JsonValue& root, memochat::json::JsonValue& src_root);
    static bool HandleJsonPost(
        std::shared_ptr<HttpConnection> connection,
        const std::function<bool(const memochat::json::JsonValue&, memochat::json::JsonValue&, const std::string&)>& fn);
};
