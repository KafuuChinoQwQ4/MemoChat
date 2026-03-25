#pragma once

#include <functional>
#include <memory>
#include <string>

class H1Connection;
namespace Json {
class Value;
}

class H1JsonSupport {
public:
    static bool ParseJsonBody(std::shared_ptr<H1Connection> connection, Json::Value& root, Json::Value& src_root);
    static bool HandleJsonPost(
        std::shared_ptr<H1Connection> connection,
        const std::function<bool(const Json::Value&, Json::Value&, const std::string&)>& fn);
};
