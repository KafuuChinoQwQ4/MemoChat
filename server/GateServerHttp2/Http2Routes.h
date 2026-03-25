#pragma once

#include <string>
#include <functional>
#include <memory>
#include <vector>
#include <unordered_map>
#include <h2o.h>

struct Http2Request {
    std::string method;
    std::string path;
    std::string query;
    std::string body;
    std::string remote_addr;
    std::string trace_id;
    std::unordered_map<std::string, std::string> headers;
};

struct Http2Response {
    int status_code = 200;
    std::string status_message = "OK";
    std::string body;
    std::string content_type = "application/json";
    std::unordered_map<std::string, std::string> headers;

    void SetJsonBody(const std::string& json_body) {
        body = json_body;
        content_type = "application/json";
    }
    void SetStatus(int code, const std::string& msg) {
        status_code = code;
        status_message = msg;
    }
    void SetHeader(const std::string& key, const std::string& value) {
        headers[key] = value;
    }
};

using Http2Handler = std::function<void(const Http2Request&, Http2Response&)>;

class Http2Routes
{
public:
    static void RegisterRoutes();
    static void HandleRequest(const Http2Request& req, Http2Response& resp);

    static void RegisterHandler(const std::string& method, const std::string& path, Http2Handler handler);
};
