#include "AppCoordinators.h"

#include "AppController.h"
#include "DialogListService.h"
#include "IChatTransport.h"
#include "usermgr.h"

#include <QJsonDocument>
#include <QJsonObject>

SessionRelationBootstrap::SessionRelationBootstrap(AppController& controller)
    : _app(controller)
{
}

void SessionRelationBootstrap::requestRelationBootstrap()
{
    if (_app._chat_endpoint_state.protocolVersion < 3 || !_app._gateway.chatTransport()->isConnected())
    {
        return;
    }

    QJsonObject obj;
    obj["protocol_version"] = _app._chat_endpoint_state.protocolVersion;
    if (_app._pending_login_state.uid > 0)
    {
        obj["uid"] = _app._pending_login_state.uid;
    }
    if (!_app._pending_login_state.traceId.isEmpty())
    {
        obj["trace_id"] = _app._pending_login_state.traceId;
    }
    const QByteArray payload = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    _app._gateway.chatTransport()->slot_send_data(ReqId::ID_GET_RELATION_BOOTSTRAP_REQ, payload);
}

void SessionRelationBootstrap::onRelationBootstrapUpdated()
{
    if (_app._page != AppController::ChatPage)
    {
        return;
    }

    _app._bootstrap_state.chatListInitialized = false;
    _app.ensureChatListInitialized();

    const auto friendSnapshot = _app._gateway.userMgr()->GetFriendListSnapshot();
    for (const auto& friendInfo : friendSnapshot)
    {
        if (friendInfo)
        {
            _app._chat_list_model.upsertFriend(friendInfo);
        }
    }

    // 增量更新联系人列表：避免全量 clear + set 操作
    // 只有在未初始化时才全量加载，已初始化时使用增量更新
    const auto contactList = _app._gateway.userMgr()->GetConListPerPage();
    if (!_app._bootstrap_state.contactsReady)
    {
        _app._contact_list_model.clear();
        _app._contact_list_model.setFriends(contactList);
    }
    else
    {
        // 已初始化时使用 upsert 增量更新
        for (const auto& contact : contactList)
        {
            if (contact)
            {
                _app._contact_list_model.upsertFriend(contact);
            }
        }
    }
    _app._gateway.userMgr()->UpdateContactLoadedCount();
    _app.refreshContactLoadMoreState();
    _app.setContactsReady(true);

    _app.refreshApplyModel();
    _app.setApplyReady(true);

    // 增量更新会话列表
    _app.refreshDialogModelIncremental();
    _app.flushPendingIncomingMessages();
    if (_app._chat_state.uid > 0 && _app._group_state.currentId <= 0)
    {
        const auto friendInfo = _app._gateway.userMgr()->GetFriendById(_app._chat_state.uid);
        if (friendInfo)
        {
            _app.setCurrentChatPeerName(DialogListService::privateDisplayName(friendInfo));
            _app.setCurrentChatPeerIcon(friendInfo->_icon.trimmed().isEmpty() ? QStringLiteral("qrc:/res/head_1.png")
                                                                              : friendInfo->_icon);
        }
    }
    emit _app.pendingApplyChanged();
}
