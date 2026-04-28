#pragma once

#include <memory>
#include <string>
#include <vector>

#include <mongocxx/pool.hpp>

struct MomentItemInfo {
    int seq = 0;
    std::string media_type;   // text, image, video
    std::string media_key;
    std::string thumb_key;
    std::string content;
    int width = 0;
    int height = 0;
    int duration_ms = 0;
};

struct MomentContentInfo {
    int64_t moment_id = 0;
    std::vector<MomentItemInfo> items;
    int64_t server_time = 0;
};

class MongoDao {
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
