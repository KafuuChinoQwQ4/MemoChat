#include "AppCoordinators.h"
#include "AppPortRegistry.h"
#include "IChatTransport.h"
#include "usermgr.h"

RelationBootstrapPort AppPortRegistry::makeRelationBootstrapPort()
{
    return RelationBootstrapPort{
        [this]()
        {
            RelationBootstrapSnapshot snapshot;
            const AppPendingLoginState& pending = _session_coordinator->pendingLoginState();
            const AppChatEndpointState& endpointState = _session_coordinator->chatEndpointState();
            snapshot.isChatPage = page() == _constants.chatPage;
            snapshot.chatTransportConnected = _gateway.chatTransport()->isConnected();
            snapshot.contactsReady = contactsReady();
            snapshot.protocolVersion = endpointState.protocolVersion;
            snapshot.pendingUid = pending.uid;
            snapshot.traceId = pending.traceId;
            snapshot.currentChatPeerUid = _chat_state.uid;
            snapshot.currentGroupId = currentGroupId();
            return snapshot;
        },
        [this](const QByteArray& payload)
        {
            _gateway.chatTransport()->slot_send_data(ReqId::ID_GET_RELATION_BOOTSTRAP_REQ, payload);
        },
        [this](bool initialized)
        {
            _shell_state.bootstrapState().chatListInitialized = initialized;
        },
        [this]()
        {
            ensureChatListInitialized();
        },
        [this]()
        {
            return _gateway.userMgr()->GetFriendListSnapshot();
        },
        [this](const std::shared_ptr<FriendInfo>& friendInfo)
        {
            _features.chatFeatureController.upsertContactFriendInfo(friendInfo);
        },
        [this](const std::vector<std::shared_ptr<FriendInfo>>& contacts)
        {
            _features.contactController.setContacts(contacts);
        },
        [this]()
        {
            _features.contactController.refreshContactProfiles();
        },
        [this]()
        {
            refreshContactLoadMoreState();
        },
        [this](bool ready)
        {
            setContactsReady(ready);
        },
        [this]()
        {
            _features.contactController.refreshApplySnapshot();
        },
        [this]()
        {
            refreshDialogModelIncremental();
        },
        [this]()
        {
            flushIncomingMessageRouter();
        },
        [this](int uid)
        {
            return _gateway.userMgr()->GetFriendById(uid);
        },
        [this](const QString& name)
        {
            setCurrentChatPeerName(name);
        },
        [this](const QString& icon)
        {
            setCurrentChatPeerIcon(icon);
        }};
}
