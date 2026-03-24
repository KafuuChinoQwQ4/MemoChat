#pragma once
#include "const.h"
#include "PostgresDao.h"
class PostgresMgr: public Singleton<PostgresMgr>
{
	friend class Singleton<PostgresMgr>;
public:
	~PostgresMgr();

	int RegUser(const std::string& name, const std::string& email,  const std::string& pwd, const std::string& icon);
	bool CheckEmail(const std::string& name, const std::string & email);
	bool UpdatePwd(const std::string& name, const std::string& email);
	bool UpdateUserProfile(int uid, const std::string& nick, const std::string& desc, const std::string& icon);
	bool CheckPwd(const std::string& email, const std::string& pwd, UserInfo& userInfo);
	std::string GetUserPublicId(int uid);
	bool GetCallUserProfile(int uid, CallUserProfile& profile);
	bool IsFriend(int uid, int peer_uid);
	bool UpsertCallSession(const CallSessionInfo& session);
	bool GetCallSession(const std::string& call_id, CallSessionInfo& session);
	bool InsertMediaAsset(const MediaAssetInfo& asset);
	bool GetMediaAssetByKey(const std::string& media_key, MediaAssetInfo& asset);
	bool GetUserInfo(int uid, UserInfo& user_info);
	bool TestProcedure(const std::string &email, int& uid, string & name);

	// Moments operations
	bool AddMoment(const MomentInfo& moment);
	bool GetMomentsFeed(int viewer_uid, int64_t last_moment_id, int limit,
		std::vector<MomentInfo>& moments, bool& has_more);
	bool GetMomentById(int64_t moment_id, MomentInfo& moment);
	bool DeleteMoment(int64_t moment_id, int uid);
	bool AddMomentLike(int64_t moment_id, int uid);
	bool RemoveMomentLike(int64_t moment_id, int uid);
	bool HasLikedMoment(int64_t moment_id, int uid);
	bool GetMomentLikes(int64_t moment_id, int limit,
		std::vector<MomentLikeInfo>& likes, bool& has_more);
	bool AddMomentComment(const MomentCommentInfo& comment);
	bool DeleteMomentComment(int64_t comment_id, int uid);
	bool GetMomentComments(int64_t moment_id, int64_t last_comment_id, int limit,
		std::vector<MomentCommentInfo>& comments, bool& has_more);

private:
	PostgresMgr();
	PostgresDao  _dao;
};
