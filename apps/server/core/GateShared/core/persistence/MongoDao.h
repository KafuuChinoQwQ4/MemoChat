#pragma once

#include "MomentTypes.h"

#include <memory>
#include <string>
#include <vector>

#include <mongocxx/pool.hpp>

class MongoDao
{
public:
    MongoDao();
    ~MongoDao();

    bool Enabled() const;

    bool InsertMomentContent(const MomentContentInfo& content);
    bool GetMomentContent(int64_t moment_id, MomentContentInfo& content);
    bool DeleteMomentContent(int64_t moment_id);

private:
    bool Init();
    bool EnsureIndexes();

    bool enabled_ = false;
    bool init_ok_ = false;
    std::string uri_;
    std::string database_name_;
    std::string moments_collection_name_;
    std::unique_ptr<mongocxx::pool> pool_;
};
