#include "MongoDao.h"

#include "ConfigMgr.h"

#include <algorithm>
#include <iostream>

#include <bsoncxx/builder/basic/array.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <bsoncxx/json.hpp>
#include <bsoncxx/types.hpp>
#include <mongocxx/exception/exception.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/options/replace.hpp>

namespace {
using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_document;
using bsoncxx::builder::basic::sub_array;
using bsoncxx::builder::basic::sub_document;

mongocxx::instance& MongoInstance() {
    static mongocxx::instance instance{};
    return instance;
}

bool ParseBool(const std::string& raw) {
    std::string normalized = raw;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return normalized == "1" || normalized == "true" || normalized == "yes" || normalized == "on";
}

std::string GetString(const bsoncxx::document::view& view, const char* key, const std::string& default_value = "") {
    auto element = view[key];
    if (!element || element.type() != bsoncxx::type::k_string) {
        return default_value;
    }
    return std::string(element.get_string().value);
}

int GetInt(const bsoncxx::document::view& view, const char* key, int default_value = 0) {
    auto element = view[key];
    if (!element) {
        return default_value;
    }
    if (element.type() == bsoncxx::type::k_int32) {
        return element.get_int32().value;
    }
    if (element.type() == bsoncxx::type::k_int64) {
        return static_cast<int>(element.get_int64().value);
    }
    return default_value;
}

int64_t GetInt64(const bsoncxx::document::view& view, const char* key, int64_t default_value = 0) {
    auto element = view[key];
    if (!element) {
        return default_value;
    }
    if (element.type() == bsoncxx::type::k_int64) {
        return element.get_int64().value;
    }
    if (element.type() == bsoncxx::type::k_int32) {
        return element.get_int32().value;
    }
    return default_value;
}
}  // namespace

MongoDao::MongoDao() {
    init_ok_ = Init();
}

MongoDao::~MongoDao() {
}

bool MongoDao::Enabled() const {
    return enabled_ && init_ok_ && pool_ != nullptr;
}

bool MongoDao::Init() {
    auto& cfg = ConfigMgr::Inst();
    enabled_ = ParseBool(cfg.GetValue("Mongo", "Enabled"));
    if (!enabled_) {
        std::cerr << "[MongoDao] MongoDB not enabled, Moments content will not be stored" << std::endl;
        return false;
    }

    uri_ = cfg.GetValue("Mongo", "Uri");
    database_name_ = cfg.GetValue("Mongo", "Database");
    moments_collection_name_ = cfg.GetValue("Mongo", "MomentsCollection");
    if (uri_.empty() || database_name_.empty()) {
        std::cerr << "[MongoDao] Mongo config missing Uri or Database" << std::endl;
        enabled_ = false;
        return false;
    }
    if (moments_collection_name_.empty()) {
        moments_collection_name_ = "moments_content";
    }

    try {
        (void)MongoInstance();
        pool_.reset(new mongocxx::pool(mongocxx::uri{ uri_ }));
        return EnsureIndexes();
    }
    catch (const std::exception& e) {
        std::cerr << "[MongoDao] init failed: " << e.what() << std::endl;
        pool_.reset();
        enabled_ = false;
        return false;
    }
}

bool MongoDao::EnsureIndexes() {
    if (!pool_) {
        return false;
    }

    try {
        auto client = pool_->acquire();
        auto db = (*client)[database_name_];
        auto coll = db[moments_collection_name_];

        mongocxx::options::index opts;
        opts.name("idx_moment_id");
        opts.unique(true);
        coll.create_index(bsoncxx::builder::basic::make_document(kvp("moment_id", 1)), opts);

        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "[MongoDao] ensure indexes failed: " << e.what() << std::endl;
        return false;
    }
}

bool MongoDao::InsertMomentContent(const MomentContentInfo& content) {
    if (!Enabled() || content.moment_id <= 0) {
        return false;
    }

    try {
        auto client = pool_->acquire();
        auto collection = (*client)[database_name_][moments_collection_name_];

        bsoncxx::builder::basic::document doc;
        doc.append(kvp("moment_id", content.moment_id));

        bsoncxx::builder::basic::array items_arr;
        for (const auto& item : content.items) {
            bsoncxx::builder::basic::document item_doc;
            item_doc.append(kvp("seq", item.seq));
            item_doc.append(kvp("media_type", item.media_type));
            item_doc.append(kvp("media_key", item.media_key));
            item_doc.append(kvp("thumb_key", item.thumb_key));
            item_doc.append(kvp("content", item.content));
            item_doc.append(kvp("width", item.width));
            item_doc.append(kvp("height", item.height));
            item_doc.append(kvp("duration_ms", item.duration_ms));
            items_arr.append(item_doc.extract());
        }
        doc.append(kvp("items", items_arr.extract()));
        doc.append(kvp("server_time", content.server_time));

        const auto filter = bsoncxx::builder::basic::make_document(kvp("moment_id", content.moment_id));
        const auto doc_value = doc.extract();
        collection.delete_one(filter.view());
        const auto inserted = collection.insert_one(doc_value.view());
        if (!inserted) {
            std::cerr << "[MongoDao] InsertMomentContent insert_one returned null, moment_id="
                      << content.moment_id << std::endl;
            return false;
        }

        const auto verify = collection.find_one(filter.view());
        if (!verify) {
            std::cerr << "[MongoDao] InsertMomentContent verify failed, moment_id="
                      << content.moment_id
                      << " db=" << database_name_
                      << " collection=" << moments_collection_name_
                      << " items=" << content.items.size() << std::endl;
            return false;
        }
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "[MongoDao] InsertMomentContent failed: " << e.what() << std::endl;
        return false;
    }
}

bool MongoDao::GetMomentContent(int64_t moment_id, MomentContentInfo& content) {
    if (!Enabled() || moment_id <= 0) {
        return false;
    }

    try {
        auto client = pool_->acquire();
        auto collection = (*client)[database_name_][moments_collection_name_];
        auto result = collection.find_one(bsoncxx::builder::basic::make_document(kvp("moment_id", moment_id)));
        if (!result) {
            return false;
        }

        const auto doc = result->view();
        content.moment_id = GetInt64(doc, "moment_id", 0);
        content.server_time = GetInt64(doc, "server_time", 0);
        content.items.clear();

        auto items_view = doc["items"];
        if (items_view && items_view.type() == bsoncxx::type::k_array) {
            for (auto&& elem : items_view.get_array().value) {
                if (elem.type() != bsoncxx::type::k_document) {
                    continue;
                }
                auto item_view = elem.get_document().view();
                MomentItemInfo item;
                item.seq = GetInt(item_view, "seq", 0);
                item.media_type = GetString(item_view, "media_type", "text");
                item.media_key = GetString(item_view, "media_key", "");
                item.thumb_key = GetString(item_view, "thumb_key", "");
                item.content = GetString(item_view, "content", "");
                item.width = GetInt(item_view, "width", 0);
                item.height = GetInt(item_view, "height", 0);
                item.duration_ms = GetInt(item_view, "duration_ms", 0);
                content.items.push_back(std::move(item));
            }
        }

        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "[MongoDao] GetMomentContent failed: " << e.what() << std::endl;
        return false;
    }
}

bool MongoDao::DeleteMomentContent(int64_t moment_id) {
    if (!Enabled() || moment_id <= 0) {
        return false;
    }

    try {
        auto client = pool_->acquire();
        auto collection = (*client)[database_name_][moments_collection_name_];
        collection.delete_one(bsoncxx::builder::basic::make_document(kvp("moment_id", moment_id)));
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "[MongoDao] DeleteMomentContent failed: " << e.what() << std::endl;
        return false;
    }
}
