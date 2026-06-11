#include "AppCoordinators.h"

#include "DialogListService.h"

#include <QJsonDocument>
#include <QJsonObject>

SessionRelationBootstrap::SessionRelationBootstrap(RelationBootstrapPort port)
    : _port(std::move(port))
{
}

void SessionRelationBootstrap::requestRelationBootstrap()
{
    const RelationBootstrapSnapshot snapshot = _port.snapshot();
    if (snapshot.protocolVersion < 3 || !snapshot.chatTransportConnected)
    {
        return;
    }

    QJsonObject obj;
    obj["protocol_version"] = snapshot.protocolVersion;
    if (snapshot.pendingUid > 0)
    {
        obj["uid"] = snapshot.pendingUid;
    }
    if (!snapshot.traceId.isEmpty())
    {
        obj["trace_id"] = snapshot.traceId;
    }
    const QByteArray payload = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    _port.sendRelationBootstrap(payload);
}

void SessionRelationBootstrap::onRelationBootstrapUpdated()
{
    const RelationBootstrapSnapshot snapshot = _port.snapshot();
    if (!snapshot.isChatPage)
    {
        return;
    }

    _port.setChatListInitialized(false);
    _port.ensureChatListInitialized();

    const auto friendSnapshot = _port.friendListSnapshot();
    for (const auto& friendInfo : friendSnapshot)
    {
        if (friendInfo)
        {
            _port.upsertChatListFriend(friendInfo);
        }
    }

    const auto contactList = _port.nextContactPage();
    if (!snapshot.contactsReady)
    {
        _port.setContacts(contactList);
    }
    else
    {
        for (const auto& contact : contactList)
        {
            if (contact)
            {
                _port.upsertContact(contact);
            }
        }
    }
    _port.markContactPageLoaded();
    _port.refreshContactLoadMoreState();
    _port.setContactsReady(true);

    _port.refreshApplySnapshot();

    _port.refreshDialogModelIncremental();
    _port.flushIncomingMessageRouter();
    if (snapshot.currentChatPeerUid > 0 && snapshot.currentGroupId <= 0)
    {
        const auto friendInfo = _port.friendById(snapshot.currentChatPeerUid);
        if (friendInfo)
        {
            _port.setCurrentChatPeerName(DialogListService::privateDisplayName(friendInfo));
            _port.setCurrentChatPeerIcon(friendInfo->_icon.trimmed().isEmpty() ? QStringLiteral("qrc:/res/head_1.png")
                                                                               : friendInfo->_icon);
        }
    }
}
