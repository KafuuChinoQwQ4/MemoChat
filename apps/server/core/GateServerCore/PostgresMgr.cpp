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

bool PostgresMgr::GetUserInfo(int uid, UserInfo& user_info) {
	return _dao.GetUserInfo(uid, user_info);
}

bool PostgresMgr::TestProcedure(const std::string& email, int& uid, string& name) {
	return _dao.TestProcedure(email,uid, name);
}

bool PostgresMgr::AddMoment(const MomentInfo& moment, int64_t* moment_id) {
	return _dao.AddMoment(moment, moment_id);
}

bool PostgresMgr::GetMomentsFeed(int viewer_uid, int64_t last_moment_id, int limit, int author_uid,
	std::vector<MomentInfo>& moments, bool& has_more) {
	return _dao.GetMomentsFeed(viewer_uid, last_moment_id, limit, author_uid, moments, has_more);
}

bool PostgresMgr::CanViewMoment(int viewer_uid, const MomentInfo& moment) {
	return _dao.CanViewMoment(viewer_uid, moment);
}

bool PostgresMgr::GetMomentById(int64_t moment_id, MomentInfo& moment) {
	return _dao.GetMomentById(moment_id, moment);
}

bool PostgresMgr::DeleteMoment(int64_t moment_id, int uid) {
	return _dao.DeleteMoment(moment_id, uid);
}

bool PostgresMgr::AddMomentLike(int64_t moment_id, int uid) {
	return _dao.AddMomentLike(moment_id, uid);
}

bool PostgresMgr::RemoveMomentLike(int64_t moment_id, int uid) {
	return _dao.RemoveMomentLike(moment_id, uid);
}

bool PostgresMgr::HasLikedMoment(int64_t moment_id, int uid) {
	return _dao.HasLikedMoment(moment_id, uid);
}

bool PostgresMgr::GetMomentLikes(int64_t moment_id, int limit,
	std::vector<MomentLikeInfo>& likes, bool& has_more) {
	return _dao.GetMomentLikes(moment_id, limit, likes, has_more);
}

bool PostgresMgr::AddMomentComment(const MomentCommentInfo& comment) {
	return _dao.AddMomentComment(comment);
}

bool PostgresMgr::DeleteMomentComment(int64_t comment_id, int uid) {
	return _dao.DeleteMomentComment(comment_id, uid);
}

bool PostgresMgr::GetMomentComments(int64_t moment_id, int64_t last_comment_id, int limit,
	std::vector<MomentCommentInfo>& comments, bool& has_more) {
	return _dao.GetMomentComments(moment_id, last_comment_id, limit, comments, has_more);
}

bool PostgresMgr::AddMomentCommentLike(int64_t comment_id, int uid) {
	return _dao.AddMomentCommentLike(comment_id, uid);
}

bool PostgresMgr::RemoveMomentCommentLike(int64_t comment_id, int uid) {
	return _dao.RemoveMomentCommentLike(comment_id, uid);
}

bool PostgresMgr::HasLikedMomentComment(int64_t comment_id, int uid) {
	return _dao.HasLikedMomentComment(comment_id, uid);
}

bool PostgresMgr::GetMomentCommentLikes(int64_t comment_id, int limit,
	std::vector<MomentLikeInfo>& likes, bool& has_more) {
	return _dao.GetMomentCommentLikes(comment_id, limit, likes, has_more);
}
