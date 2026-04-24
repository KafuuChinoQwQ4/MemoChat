#pragma once

#include <functional>
#include <memory>
#include <string>
#include "json/GlazeCompat.h"

class GateHttp3Connection;

class GateHttp3JsonSupport {
public:
    static bool ParseJsonBody(std::shared_ptr<GateHttp3Connection> connection,
                              memochat::json::JsonValue& root, memochat::json::JsonValue& src_root);
    static bool HandleJsonPost(
        std::shared_ptr<GateHttp3Connection> connection,
        std::function<bool(const memochat::json::JsonValue&, memochat::json::JsonValue&, const std::string&)> fn);
};
