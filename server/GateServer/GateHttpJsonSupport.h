#pragma once

#include <functional>
#include <memory>
#include <string>

class HttpConnection;
namespace Json {
class Value;
}

class GateHttpJsonSupport {
public:
    static bool ParseJsonBody(std::shared_ptr<HttpConnection> connection, Json::Value& root, Json::Value& src_root);
    static bool HandleJsonPost(
        std::shared_ptr<HttpConnection> connection,
        const std::function<bool(const Json::Value&, Json::Value&, const std::string&)>& fn);
};
