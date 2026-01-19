#include "MysqlMgr.h"

MysqlMgr::~MysqlMgr() {}

MysqlMgr::MysqlMgr() {}

int MysqlMgr::RegUser(const std::string& name, const std::string& email, const std::string& pwd)
{
    return _dao.RegUser(name, email, pwd);
}