#pragma once

#include <functional>
#include <memory>
#include <string>

class GateHttp3Connection;

namespace Json {
class Value;
}

class GateHttp3JsonSupport {
public:
    static bool ParseJsonBody(std::shared_ptr<GateHttp3Connection> connection,
                              Json::Value& root, Json::Value& src_root);
    static bool HandleJsonPost(
        std::shared_ptr<GateHttp3Connection> connection,
        std::function<bool(const Json::Value&, Json::Value&, const std::string&)> fn);
};
