#pragma once
#include "const.h"
#include "MysqlDao.h"
class MysqlMgr: public Singleton<MysqlMgr>
{
	friend class Singleton<MysqlMgr>;
public:
	~MysqlMgr();

	int RegUser(const std::string& name, const std::string& email,  const std::string& pwd, const std::string& icon);
	bool CheckEmail(const std::string& name, const std::string & email);
	bool UpdatePwd(const std::string& name, const std::string& email);
	bool UpdateUserProfile(int uid, const std::string& nick, const std::string& desc, const std::string& icon);
	bool CheckPwd(const std::string& email, const std::string& pwd, UserInfo& userInfo);
	std::string GetUserPublicId(int uid);
	bool InsertMediaAsset(const MediaAssetInfo& asset);
	bool GetMediaAssetByKey(const std::string& media_key, MediaAssetInfo& asset);
	bool TestProcedure(const std::string &email, int& uid, string & name);
private:
	MysqlMgr();
	MysqlDao  _dao;
};
