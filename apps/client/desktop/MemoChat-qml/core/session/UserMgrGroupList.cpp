#include "usermgr.h"

#include <QJsonArray>
#include <algorithm>

void UserMgr::SetGroupList(const QJsonArray& array)
{
    const auto previousGroups = _group_map;
    _group_list.clear();
    _group_map.clear();
    _group_loaded = 0;
    for (const QJsonValue& value : array)
    {
        auto info = std::make_shared<GroupInfoData>();
        info->_group_id = value["groupid"].toVariant().toLongLong();
        info->_group_code = value["group_code"].toString();
        info->_name = value["name"].toString();
        info->_icon = value["icon"].toString();
        info->_announcement = value["announcement"].toString();
        info->_owner_uid = value["owner_uid"].toInt();
        info->_member_limit = value["member_limit"].toInt(200);
        info->_member_count = value["member_count"].toInt(0);
        info->_role = value["role"].toInt(1);
        info->_is_all_muted = value["is_all_muted"].toInt(0);
        info->_permission_bits = value["permission_bits"].toVariant().toLongLong();
        if (info->_group_id <= 0)
        {
            continue;
        }
        auto previous = previousGroups.find(info->_group_id);
        if (previous != previousGroups.end())
        {
            const auto& existing = previous.value();
            if (existing)
            {
                info->_chat_msgs = existing->_chat_msgs;
                if (info->_last_msg.isEmpty())
                {
                    info->_last_msg = existing->_last_msg;
                }
            }
        }
        _group_list.push_back(info);
        _group_map.insert(info->_group_id, info);
    }
}

std::vector<std::shared_ptr<GroupInfoData>> UserMgr::GetGroupListPerPage()
{
    std::vector<std::shared_ptr<GroupInfoData>> groups;
    int begin = _group_loaded;
    int end = begin + CHAT_COUNT_PER_PAGE;
    if (begin >= _group_list.size())
    {
        return groups;
    }
    if (end > _group_list.size())
    {
        groups = std::vector<std::shared_ptr<GroupInfoData>>(_group_list.begin() + begin, _group_list.end());
        return groups;
    }
    groups = std::vector<std::shared_ptr<GroupInfoData>>(_group_list.begin() + begin, _group_list.begin() + end);
    return groups;
}

bool UserMgr::IsLoadGroupFin()
{
    return _group_loaded >= _group_list.size();
}

void UserMgr::UpdateGroupLoadedCount()
{
    int begin = _group_loaded;
    int end = begin + CHAT_COUNT_PER_PAGE;
    if (begin >= _group_list.size())
    {
        return;
    }
    if (end > _group_list.size())
    {
        _group_loaded = static_cast<int>(_group_list.size());
        return;
    }
    _group_loaded = end;
}

std::shared_ptr<GroupInfoData> UserMgr::GetGroupById(qint64 groupId)
{
    auto iter = _group_map.find(groupId);
    if (iter == _group_map.end())
    {
        return nullptr;
    }
    return iter.value();
}

bool UserMgr::CheckGroupById(qint64 groupId)
{
    return _group_map.find(groupId) != _group_map.end();
}

void UserMgr::UpsertGroup(const std::shared_ptr<GroupInfoData>& groupInfo)
{
    if (!groupInfo || groupInfo->_group_id <= 0)
    {
        return;
    }
    auto iter = _group_map.find(groupInfo->_group_id);
    if (iter == _group_map.end())
    {
        _group_list.push_back(groupInfo);
        _group_map.insert(groupInfo->_group_id, groupInfo);
        return;
    }
    auto stored = iter.value();
    if (!stored)
    {
        iter.value() = groupInfo;
        return;
    }
    stored->_name = groupInfo->_name;
    stored->_icon = groupInfo->_icon;
    stored->_announcement = groupInfo->_announcement;
    stored->_owner_uid = groupInfo->_owner_uid;
    stored->_member_limit = groupInfo->_member_limit;
    stored->_member_count = groupInfo->_member_count;
    stored->_role = groupInfo->_role;
    stored->_is_all_muted = groupInfo->_is_all_muted;
    stored->_permission_bits = groupInfo->_permission_bits;
    if (!groupInfo->_group_code.isEmpty())
    {
        stored->_group_code = groupInfo->_group_code;
    }
    if (!groupInfo->_last_msg.isEmpty())
    {
        stored->_last_msg = groupInfo->_last_msg;
    }
}

void UserMgr::RemoveGroup(qint64 groupId)
{
    _group_map.remove(groupId);
    _group_list.erase(std::remove_if(_group_list.begin(),
                                     _group_list.end(),
                                     [groupId](const std::shared_ptr<GroupInfoData>& info)
                                     {
                                         return info && info->_group_id == groupId;
                                     }),
                      _group_list.end());
    if (_group_loaded > static_cast<int>(_group_list.size()))
    {
        _group_loaded = static_cast<int>(_group_list.size());
    }
}

std::vector<std::shared_ptr<GroupInfoData>> UserMgr::GetGroupListSnapshot() const
{
    return _group_list;
}
