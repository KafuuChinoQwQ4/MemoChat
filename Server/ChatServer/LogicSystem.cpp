#include "LogicSystem.h"

LogicSystem::LogicSystem() {
    RegisterCallBacks();
}

LogicSystem::~LogicSystem() {}

void LogicSystem::RegisterCallBacks() {
    _fun_callbacks[MSG_CHAT_LOGIN] = [this](std::shared_ptr<CSession> session, const short& msg_id, const std::string& msg_data) {
        std::cout << "Login request received: " << msg_data << endl;
        // 这里只是打印，后续会加上校验 Token 的逻辑
        session->Send("Login Success", MSG_CHAT_LOGIN_RSP);
    };
}

void LogicSystem::PostMsgToQue(std::shared_ptr<CSession> session, short msg_id, std::string msg_data) {
    // 实际项目中应该放到一个队列里，由工作线程去取，这里为了演示简化直接调用
    if (_fun_callbacks.find(msg_id) != _fun_callbacks.end()) {
        _fun_callbacks[msg_id](session, msg_id, msg_data);
    }
}