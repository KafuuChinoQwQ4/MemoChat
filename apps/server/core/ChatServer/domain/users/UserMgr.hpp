#pragma once
#include "Singleton.hpp"
#include "ports/ISessionRegistry.hpp"
#include <unordered_map>
#include <memory>
#include <mutex>
#include <string>

class CSession;
class UserMgr
    : public Singleton<UserMgr>
    , public ISessionRegistry
{
    friend class Singleton<UserMgr>;

public:
    ~UserMgr();
    std::shared_ptr<CSession> GetSession(int uid);
    void SetUserSession(int uid, std::shared_ptr<CSession> session);
    void RmvUserSession(int uid, std::string session_id);
    std::shared_ptr<CSession> FindSession(int uid) override;
    void BindSession(int uid, std::shared_ptr<CSession> session) override;
    void UnbindSession(int uid, const std::string& session_id) override;

private:
    UserMgr();
    std::mutex _session_mtx;
    std::unordered_map<int, std::shared_ptr<CSession>> _uid_to_session;
};
