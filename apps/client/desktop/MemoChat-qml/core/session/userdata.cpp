#include "userdata.h"
#include <algorithm>
#include <unordered_set>
SearchInfo::SearchInfo(int uid, QString name,
    QString nick, QString desc, int sex, QString icon, QString userId):_uid(uid)
  ,_user_id(userId),_name(name), _nick(nick),_desc(desc),_sex(sex),_icon(icon){
}

AddFriendApply::AddFriendApply(int from_uid, QString name, QString desc,
                               QString icon, QString nick, int sex, QString userId)
    :_from_uid(from_uid),_user_id(userId),_name(name),
      _desc(desc),_icon(icon),_nick(nick),_sex(sex)
{

}

void FriendInfo::AppendChatMsgs(const std::vector<std::shared_ptr<TextChatData> > text_vec)
{
    if (text_vec.empty()) {
        return;
    }

    std::unordered_set<std::string> existed;
    existed.reserve(_chat_msgs.size());
    for (const auto &one : _chat_msgs) {
        if (!one) {
            continue;
        }
        existed.insert(one->_msg_id.toStdString());
    }

    for (const auto &text: text_vec) {
        if (!text || text->_msg_id.isEmpty()) {
            continue;
        }
        const std::string key = text->_msg_id.toStdString();
        if (existed.find(key) != existed.end()) {
            continue;
        }
        existed.insert(key);
        _chat_msgs.push_back(text);
    }

    std::sort(_chat_msgs.begin(), _chat_msgs.end(),
              [](const std::shared_ptr<TextChatData> &lhs, const std::shared_ptr<TextChatData> &rhs) {
                  if (!lhs || !rhs) {
                      return static_cast<bool>(lhs);
                  }
                  if (lhs->_created_at != rhs->_created_at) {
                      return lhs->_created_at < rhs->_created_at;
                  }
                  return lhs->_msg_id < rhs->_msg_id;
              });

    for (auto it = _chat_msgs.begin(); it != _chat_msgs.end();) {
        if (!*it) {
            it = _chat_msgs.erase(it);
        } else {
            ++it;
        }
    }
}
