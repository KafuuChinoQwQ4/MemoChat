#pragma once
#include "const.h"
#include "MongoDao.h"

class MongoMgr : public Singleton<MongoMgr> {
    friend class Singleton<MongoMgr>;
public:
    ~MongoMgr();

    bool InsertMomentContent(const MomentContentInfo& content);
    bool GetMomentContent(int64_t moment_id, MomentContentInfo& content);
    bool DeleteMomentContent(int64_t moment_id);

private:
    MongoMgr();
    MongoDao _dao;
};
