#pragma once

#include "json/GlazeCompat.hpp"

#include <chrono>
#include <cstdint>
#include <string>

// Shared helpers for the ChatServer message-service translation units
// (GroupMessageService / PrivateMessageService).
//
// Defined `inline` (not in an anonymous namespace) so the single definition is
// shared across TUs without ODR violations. These replace the per-file copies
// NowMsGroupLocal / NowSecGroupLocal / NowMsLocal and the duplicated
// JsonToWireString helper.
namespace memochat::chat::message
{

// Wall-clock milliseconds since epoch.
inline int64_t NowMs()
{
    return static_cast<int64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count());
}

// Wall-clock seconds since epoch.
inline int64_t NowSec()
{
    return static_cast<int64_t>(
        std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch()).count());
}

// Compact wire JSON for TCP/QUIC transport (Qt QJsonDocument is strict).
inline std::string JsonToWireString(const memochat::json::JsonValue& v)
{
    memochat::json::JsonStreamWriterBuilder builder;
    builder["indentation"] = "";
    return memochat::json::writeString(builder, v);
}

} // namespace memochat::chat::message
