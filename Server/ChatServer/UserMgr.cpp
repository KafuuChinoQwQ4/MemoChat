#include "UserMgr.h"
#include "CSession.h"
#include "RedisMgr.h"
#include "ConfigMgr.h"

UserMgr::UserMgr() {}

UserMgr::~UserMgr() {
    _uid_to_session.clear();
}

std::shared_ptr<CSession> UserMgr::GetSession(int uid) {
    std::lock_guard<std::mutex> lock(_session_mtx);
    auto iter = _uid_to_session.find(uid);
    if (iter == _uid_to_session.end()) {
        return nullptr;
    }
    return iter->second;
}

void UserMgr::SetUserSession(int uid, std::shared_ptr<CSession> session) {
    std::lock_guard<std::mutex> lock(_session_mtx);
    _uid_to_session[uid] = session;
}

void UserMgr::RmvUserSession(int uid) {
    auto uid_str = std::to_string(uid);

    // 逻辑上：这里应该删除 Redis 中记录的用户所在服务器 IP
    // 但为了防止多端登录导致的误删，暂时注释掉，或者仅做内存清除
    // RedisMgr::GetInstance()->Del(USERIPPREFIX + uid_str);

    {
        std::lock_guard<std::mutex> lock(_session_mtx);
        _uid_to_session.erase(uid);
    }
}