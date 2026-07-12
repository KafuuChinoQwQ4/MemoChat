#pragma once

#include "MomentTypes.hpp"
#include "db/MongoCCompat.hpp"

#include <string>
#include <vector>

class MongoDao
{
public:
    MongoDao();
    ~MongoDao();

    bool Enabled() const;
    [[nodiscard]] bool Ready() const noexcept;
    [[nodiscard]] const std::string& StartupError() const noexcept;

    bool InsertMomentContent(const MomentContentInfo& content);
    bool GetMomentContent(int64_t moment_id, MomentContentInfo& content);
    bool DeleteMomentContent(int64_t moment_id);

private:
    bool Init();
    bool EnsureIndexes();

    bool enabled_ = false;
    bool required_ = false;
    bool init_ok_ = false;
    std::string startup_error_;
    std::string uri_;
    std::string database_name_;
    std::string moments_collection_name_;
    memo::db::MongoClientPool pool_;
};
