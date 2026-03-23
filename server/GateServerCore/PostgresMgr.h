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
private:
	PostgresMgr();
	PostgresDao  _dao;
};
