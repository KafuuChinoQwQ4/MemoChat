#pragma once

#include <memory>
#include <string>

class CSession;
class LogicSystem;
namespace Json {
class Value;
}

class ChatRelationService {
public:
    explicit ChatRelationService(LogicSystem& logic);

    void AppendRelationBootstrapJson(int uid, Json::Value& out);
    void BuildDialogListJson(int uid, Json::Value& out);
    void HandleSearchUser(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data);
    void HandleAddFriendApply(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data);
    void HandleAuthFriendApply(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data);
    void HandleGetDialogList(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data);
    void HandleSyncDraft(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data);
    void HandlePinDialog(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data);

private:
    LogicSystem& _logic;
};
