#pragma once

#include <memory>
#include <string>

class IChatSession;

class ISessionRegistry
{
public:
    virtual ~ISessionRegistry() = default;

    virtual std::shared_ptr<IChatSession> FindSession(int uid) = 0;
    virtual void BindSession(int uid, std::shared_ptr<IChatSession> session) = 0;
    virtual void UnbindSession(int uid, const std::string& session_id) = 0;
};
