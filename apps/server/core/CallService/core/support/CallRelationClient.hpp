#pragma once

#include "runtime/GateReadinessProbe.hpp"

#include <chrono>
#include <string>

namespace memochat::gate::services::call
{

class CallRelationClient
{
public:
    explicit CallRelationClient(std::string endpoint,
                                std::string auth_token,
                                std::chrono::milliseconds timeout = std::chrono::seconds(2));

    // The relation service owns friend tables. Any RPC or payload failure is
    // fail-closed so an unverifiable relationship cannot start a call.
    bool AreUsersFriends(int uid, int peer_uid) const;

private:
    std::string endpoint_;
    std::string auth_token_;
    std::chrono::milliseconds timeout_;
};

GateReadinessProbe CallRelationReadinessProbe(std::string endpoint,
                                              std::string auth_token,
                                              std::chrono::milliseconds timeout = std::chrono::milliseconds(500));

} // namespace memochat::gate::services::call
