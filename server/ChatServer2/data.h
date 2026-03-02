#pragma once
#include <string>
#include <vector>
struct UserInfo {
	UserInfo():name(""), pwd(""),uid(0),email(""),nick(""),desc(""),sex(0), icon(""), back("") {}
	std::string name;
	std::string pwd;
	int uid;
	std::string email;
	std::string nick;
	std::string desc;
	int sex;
	std::string icon;
	std::string back;
	std::vector<std::string> labels;
};

struct ApplyInfo {
	ApplyInfo(int uid, std::string name, std::string desc,
		std::string icon, std::string nick, int sex, int status)
		:_uid(uid),_name(name),_desc(desc),
		_icon(icon),_nick(nick),_sex(sex),_status(status){}

	int _uid;
	std::string _name;
	std::string _desc;
	std::string _icon;
	std::string _nick;
	int _sex;
	int _status;
	std::vector<std::string> _labels;
};

struct GroupInfo {
	GroupInfo()
		: group_id(0), owner_uid(0), member_limit(200), is_all_muted(0),
		  role(0), member_count(0), status(1) {}

	int64_t group_id;
	std::string name;
	int owner_uid;
	std::string announcement;
	int member_limit;
	int is_all_muted;
	int role; // 3-owner, 2-admin, 1-member
	int member_count;
	int status;
};

struct GroupMemberInfo {
	GroupMemberInfo()
		: group_id(0), uid(0), role(1), mute_until(0), status(1) {}

	int64_t group_id;
	int uid;
	int role; // 3-owner, 2-admin, 1-member
	int64_t mute_until;
	int status; // 1-active, 2-exited, 3-kicked
	std::string name;
	std::string nick;
	std::string icon;
};

struct GroupApplyInfo {
	GroupApplyInfo()
		: apply_id(0), group_id(0), applicant_uid(0), inviter_uid(0),
		  reviewer_uid(0), status(0) {}

	int64_t apply_id;
	int64_t group_id;
	int applicant_uid;
	int inviter_uid;
	std::string type; // invite/apply
	int status; // 0-pending 1-accepted 2-rejected
	std::string reason;
	int reviewer_uid;
};

struct GroupMessageInfo {
	GroupMessageInfo()
		: group_id(0), from_uid(0), size(0), created_at(0) {}

	std::string msg_id;
	int64_t group_id;
	int from_uid;
	std::string msg_type;
	std::string content;
	std::string mentions_json;
	std::string file_name;
	std::string mime;
	int size;
	int64_t created_at;
};
