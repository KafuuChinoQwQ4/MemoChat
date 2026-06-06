#pragma once

#include <string>

struct RelationCommandRequest
{
    short request_msg_id = 0;
    std::string payload_json;
    int session_uid = 0;
    std::string session_id;
    std::string server_name;
    std::string trace_id;
};

struct RelationCommandResult
{
    short response_msg_id = 0;
    std::string payload_json;
};

class IRelationCommandService
{
public:
    virtual ~IRelationCommandService() = default;

    virtual RelationCommandResult SearchUser(const RelationCommandRequest& request) = 0;
    virtual RelationCommandResult AddFriendApply(const RelationCommandRequest& request) = 0;
    virtual RelationCommandResult AuthFriendApply(const RelationCommandRequest& request) = 0;
    virtual RelationCommandResult DeleteFriend(const RelationCommandRequest& request) = 0;
    virtual RelationCommandResult GetDialogList(const RelationCommandRequest& request) = 0;
    virtual RelationCommandResult SyncDraft(const RelationCommandRequest& request) = 0;
    virtual RelationCommandResult PinDialog(const RelationCommandRequest& request) = 0;
};
