#pragma once
#include "const.hpp"
#include "MongoDao.hpp"

class MongoMgr : public Singleton<MongoMgr>
{
    friend class Singleton<MongoMgr>;

public:
    ~MongoMgr();
    [[nodiscard]] bool Ready() const noexcept;
    [[nodiscard]] const std::string& StartupError() const noexcept;

    bool InsertMomentContent(const MomentContentInfo& content);
    bool GetMomentContent(int64_t moment_id, MomentContentInfo& content);
    bool DeleteMomentContent(int64_t moment_id);

private:
    MongoMgr();
    MongoDao _dao;
};
