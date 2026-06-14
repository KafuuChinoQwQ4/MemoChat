#pragma once

#include <string>
#include <unordered_map>

namespace memochat::gate::routing
{

enum class GateResponseBodyKind
{
    Inline,
    File
};

struct GateResponse
{
    int status = 200;
    std::string content_type;
    std::unordered_map<std::string, std::string> headers;
    GateResponseBodyKind body_kind = GateResponseBodyKind::Inline;
    std::string body;
    std::string file_path;
};

} // namespace memochat::gate::routing
