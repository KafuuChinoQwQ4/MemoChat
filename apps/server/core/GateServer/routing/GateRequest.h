#pragma once

#include <string>
#include <unordered_map>

namespace memochat::gate::routing
{

struct GateRequest
{
    std::string method;
    std::string path;
    std::string target;
    std::unordered_map<std::string, std::string> query;
    std::unordered_map<std::string, std::string> headers;
    std::string body;
    std::string trace_id;
    std::string request_id;
    std::string remote_address;
};

} // namespace memochat::gate::routing
