#pragma once
#include "const.h"
#include "PostgresDao.h"
class PostgresMgr: public Singleton<PostgresMgr>
{
	friend class Singleton<PostgresMgr>;
public:
	~PostgresMgr();
	int RegUser(const std::string& name, const std::string& email,  const std::string& pwd);
	bool CheckEmail(const std::string& name, const std::string & email);
	bool UpdatePwd(const std::string& name, const std::string& email);
	bool CheckPwd(const std::string& name, const std::string& pwd, UserInfo& userInfo);
private:
	PostgresMgr();
	PostgresDao  _dao;
};

