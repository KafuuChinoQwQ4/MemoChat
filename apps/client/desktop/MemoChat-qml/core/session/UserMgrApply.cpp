#include "usermgr.h"

#include <QJsonArray>

void UserMgr::AppendApplyList(QJsonArray array)
{
    _apply_list.clear();

    for (const QJsonValue& value : array)
    {
        auto name = value["name"].toString();
        auto desc = value["desc"].toString();
        auto icon = value["icon"].toString();
        auto nick = value["nick"].toString();
        auto sex = value["sex"].toInt();
        auto uid = value["uid"].toInt();
        auto userId = value["user_id"].toString();
        auto status = value["status"].toInt();
        auto info = std::make_shared<ApplyInfo>(uid, name, desc, icon, nick, sex, status, userId);
        _apply_list.push_back(info);
    }
}

std::vector<std::shared_ptr<ApplyInfo>> UserMgr::GetApplyList()
{
    return _apply_list;
}

std::vector<std::shared_ptr<ApplyInfo>> UserMgr::GetApplyListSnapshot() const
{
    return _apply_list;
}

void UserMgr::AddApplyList(std::shared_ptr<ApplyInfo> app)
{
    if (!app)
    {
        return;
    }

    for (const auto& item : _apply_list)
    {
        if (!item)
        {
            continue;
        }
        if (item->_uid == app->_uid)
        {
            item->_name = app->_name;
            item->_nick = app->_nick;
            item->_desc = app->_desc;
            item->_icon = app->_icon;
            item->_sex = app->_sex;
            item->_status = app->_status;
            return;
        }
    }

    _apply_list.push_back(app);
}

bool UserMgr::AlreadyApply(int uid)
{
    for (auto& apply : _apply_list)
    {
        if (apply->_uid == uid)
        {
            return true;
        }
    }

    return false;
}

void UserMgr::MarkApplyStatus(int uid, int status)
{
    for (const auto& apply : _apply_list)
    {
        if (!apply)
        {
            continue;
        }
        if (apply->_uid == uid)
        {
            apply->_status = status;
            return;
        }
    }
}
