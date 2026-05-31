#include "AppController.h"
#include "IChatTransport.h"
#include "usermgr.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QVariantMap>

void AppController::selectContactIndex(int index)
{
    ensureContactsInitialized();
    const QVariantMap item = _contact_list_model.get(index);
    if (item.isEmpty())
    {
        return;
    }

    const QString nick = item.value("nick").toString();
    QString back = item.value("back").toString();
    if (back.isEmpty())
    {
        back = nick;
    }

    setCurrentContact(item.value("uid").toInt(),
                      item.value("name").toString(),
                      nick,
                      item.value("icon").toString(),
                      back,
                      item.value("sex").toInt(),
                      item.value("userId").toString());
    setContactPane(FriendInfoPane);
    setAuthStatus(QString(), false);
}

QVariantMap AppController::contactProfileByUid(int uid) const
{
    const auto friendInfo = _gateway.userMgr()->GetFriendById(uid);
    if (!friendInfo)
    {
        return {};
    }

    return {{"uid", friendInfo->_uid},
            {"userId", friendInfo->_user_id},
            {"name", friendInfo->_name},
            {"nick", friendInfo->_nick},
            {"icon", normalizeIconPath(friendInfo->_icon)},
            {"desc", friendInfo->_desc},
            {"back", friendInfo->_back},
            {"sex", friendInfo->_sex}};
}

void AppController::deleteFriend(int uid)
{
    auto selfInfo = _gateway.userMgr()->GetUserInfo();
    if (!selfInfo || selfInfo->_uid <= 0 || uid <= 0 || uid == selfInfo->_uid)
    {
        setAuthStatus("联系人状态异常，无法删除", true);
        return;
    }

    QJsonObject obj;
    obj["fromuid"] = selfInfo->_uid;
    obj["friend_uid"] = uid;
    const QByteArray payload = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    _gateway.chatTransport()->slot_send_data(ReqId::ID_DELETE_FRIEND_REQ, payload);
    setAuthStatus("正在删除联系人...", false);
}
