#include "PostgresMgr.h"


PostgresMgr::~PostgresMgr() {

}

int PostgresMgr::RegUser(const std::string& name, const std::string& email, const std::string& pwd, const std::string& icon)
{
	return _dao.RegUserTransaction(name, email, pwd, icon);
}

bool PostgresMgr::CheckEmail(const std::string& name, const std::string& email) {
	return _dao.CheckEmail(name, email);
}

bool PostgresMgr::UpdatePwd(const std::string& name, const std::string& pwd) {
	return _dao.UpdatePwd(name, pwd);
}

bool PostgresMgr::UpdateUserProfile(int uid, const std::string& nick, const std::string& desc, const std::string& icon) {
	return _dao.UpdateUserProfile(uid, nick, desc, icon);
}
PostgresMgr::PostgresMgr() {
}

bool PostgresMgr::CheckPwd(const std::string& email, const std::string& pwd, UserInfo& userInfo) {
	return _dao.CheckPwd(email, pwd, userInfo);
}

std::string PostgresMgr::GetUserPublicId(int uid) {
	return _dao.GetUserPublicId(uid);
}

bool PostgresMgr::GetCallUserProfile(int uid, CallUserProfile& profile) {
	return _dao.GetCallUserProfile(uid, profile);
}

bool PostgresMgr::IsFriend(int uid, int peer_uid) {
	return _dao.IsFriend(uid, peer_uid);
}

bool PostgresMgr::UpsertCallSession(const CallSessionInfo& session) {
	return _dao.UpsertCallSession(session);
}

bool PostgresMgr::GetCallSession(const std::string& call_id, CallSessionInfo& session) {
	return _dao.GetCallSession(call_id, session);
}

bool PostgresMgr::InsertMediaAsset(const MediaAssetInfo& asset) {
	return _dao.InsertMediaAsset(asset);
}

bool PostgresMgr::GetMediaAssetByKey(const std::string& media_key, MediaAssetInfo& asset) {
	return _dao.GetMediaAssetByKey(media_key, asset);
}

bool PostgresMgr::TestProcedure(const std::string& email, int& uid, string& name) {
	return _dao.TestProcedure(email,uid, name);
}

