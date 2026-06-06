#pragma once

#include "RelationGrpcClient.h"
#include "ports/IRelationService.h"

#include <chrono>
#include <memory>
#include <string>

#include <grpcpp/grpcpp.h>

class RelationGrpcServiceAdapter final : public IRelationService
{
public:
    explicit RelationGrpcServiceAdapter(const std::string& endpoint,
                                        std::chrono::milliseconds timeout = std::chrono::seconds(2));
    explicit RelationGrpcServiceAdapter(std::shared_ptr<grpc::Channel> channel,
                                        std::chrono::milliseconds timeout = std::chrono::seconds(2));

    void AppendRelationBootstrapJson(int uid, memochat::json::JsonValue& out) override;
    void BuildDialogListJson(int uid, memochat::json::JsonValue& out) override;

    RelationCommandResult SearchUser(const RelationCommandRequest& request) override;
    RelationCommandResult AddFriendApply(const RelationCommandRequest& request) override;
    RelationCommandResult AuthFriendApply(const RelationCommandRequest& request) override;
    RelationCommandResult DeleteFriend(const RelationCommandRequest& request) override;
    RelationCommandResult GetDialogList(const RelationCommandRequest& request) override;
    RelationCommandResult SyncDraft(const RelationCommandRequest& request) override;
    RelationCommandResult PinDialog(const RelationCommandRequest& request) override;

    void HandleSearchUser(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data) override;
    void
    HandleAddFriendApply(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data) override;
    void
    HandleAuthFriendApply(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data) override;
    void
    HandleDeleteFriend(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data) override;
    void
    HandleGetDialogList(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data) override;
    void HandleSyncDraft(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data) override;
    void HandlePinDialog(const std::shared_ptr<CSession>& session, short msg_id, const std::string& msg_data) override;

private:
    RelationCommandRequest BuildSessionCommandRequest(const std::shared_ptr<CSession>& session,
                                                      short msg_id,
                                                      const std::string& msg_data) const;
    void SendSessionCommandResult(const std::shared_ptr<CSession>& session, const RelationCommandResult& result) const;

    RelationGrpcClient _client;
};
