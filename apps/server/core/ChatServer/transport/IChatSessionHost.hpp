#pragma once
#include <string>

/// Transport-neutral host interface for TCP session lifecycle callbacks.
///
/// CSession holds an IChatSessionHost* instead of a concrete CServer* so that
/// the domain and session layers are not coupled to the TCP server implementation.
/// CServer is the sole implementation; other transports manage session cleanup
/// via injected close callbacks and do not implement this interface.
class IChatSessionHost
{
public:
    virtual ~IChatSessionHost() = default;

    /// Remove the session from the host's active session map.
    virtual void ClearSession(std::string session_id) = 0;

    /// Return true if the session_id is still registered in the active session map.
    virtual bool CheckValid(std::string session_id) = 0;
};
