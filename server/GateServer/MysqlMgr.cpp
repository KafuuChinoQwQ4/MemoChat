#include "MysqlMgr.h"


MysqlMgr::~MysqlMgr() {

}

int MysqlMgr::RegUser(const std::string& name, const std::string& email, const std::string& pwd, const std::string& icon)
{
	return _dao.RegUserTransaction(name, email, pwd, icon);
}

bool MysqlMgr::CheckEmail(const std::string& name, const std::string& email) {
	return _dao.CheckEmail(name, email);
}

bool MysqlMgr::UpdatePwd(const std::string& name, const std::string& pwd) {
	return _dao.UpdatePwd(name, pwd);
}

bool MysqlMgr::UpdateUserProfile(int uid, const std::string& nick, const std::string& desc, const std::string& icon) {
	return _dao.UpdateUserProfile(uid, nick, desc, icon);
}
MysqlMgr::MysqlMgr() {
}

bool MysqlMgr::CheckPwd(const std::string& email, const std::string& pwd, UserInfo& userInfo) {
	return _dao.CheckPwd(email, pwd, userInfo);
}

std::string MysqlMgr::GetUserPublicId(int uid) {
	return _dao.GetUserPublicId(uid);
}

bool MysqlMgr::GetCallUserProfile(int uid, CallUserProfile& profile) {
	return _dao.GetCallUserProfile(uid, profile);
}

bool MysqlMgr::IsFriend(int uid, int peer_uid) {
	return _dao.IsFriend(uid, peer_uid);
}

bool MysqlMgr::UpsertCallSession(const CallSessionInfo& session) {
	return _dao.UpsertCallSession(session);
}

bool MysqlMgr::GetCallSession(const std::string& call_id, CallSessionInfo& session) {
	return _dao.GetCallSession(call_id, session);
}

bool MysqlMgr::InsertMediaAsset(const MediaAssetInfo& asset) {
	return _dao.InsertMediaAsset(asset);
}

bool MysqlMgr::GetMediaAssetByKey(const std::string& media_key, MediaAssetInfo& asset) {
	return _dao.GetMediaAssetByKey(media_key, asset);
}

bool MysqlMgr::TestProcedure(const std::string& email, int& uid, string& name) {
	return _dao.TestProcedure(email,uid, name);
}

