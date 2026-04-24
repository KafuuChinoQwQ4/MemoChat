#pragma once

#include <functional>
#include <memory>
#include <string>
#include "json/GlazeCompat.h"

class H1Connection;

class H1JsonSupport {
public:
    static bool ParseJsonBody(std::shared_ptr<H1Connection> connection,
                               memochat::json::JsonValue& root,
                               memochat::json::JsonValue& src_root);

    static bool HandleJsonPost(
        std::shared_ptr<H1Connection> connection,
        const std::function<bool(const memochat::json::JsonValue&, memochat::json::JsonValue&, const std::string&)>& fn);
};

