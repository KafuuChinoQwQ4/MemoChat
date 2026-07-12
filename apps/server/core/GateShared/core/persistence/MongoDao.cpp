#include "MongoDao.hpp"

#include "ConfigMgr.hpp"

#include <bson/bson.h>

#include <cstdint>
#include <iostream>
#include <memory>
#include <utility>

import memochat.gate.mongo_dao_algorithms;

namespace mongo_dao_modules = memochat::gate::mongo_dao::modules;

namespace
{

struct CollectionDeleter
{
    void operator()(mongoc_collection_t* collection) const noexcept
    {
        if (collection != nullptr)
        {
            mongoc_collection_destroy(collection);
        }
    }
};

struct CursorDeleter
{
    void operator()(mongoc_cursor_t* cursor) const noexcept
    {
        if (cursor != nullptr)
        {
            mongoc_cursor_destroy(cursor);
        }
    }
};

struct IndexModelDeleter
{
    void operator()(mongoc_index_model_t* model) const noexcept
    {
        if (model != nullptr)
        {
            mongoc_index_model_destroy(model);
        }
    }
};

using CollectionPtr = std::unique_ptr<mongoc_collection_t, CollectionDeleter>;
using CursorPtr = std::unique_ptr<mongoc_cursor_t, CursorDeleter>;
using IndexModelPtr = std::unique_ptr<mongoc_index_model_t, IndexModelDeleter>;

bool ParseBool(const std::string& raw)
{
    return mongo_dao_modules::ParseBoolText(raw.c_str());
}

CollectionPtr
OpenCollection(mongoc_client_t* client, const std::string& database_name, const std::string& collection_name)
{
    if (client == nullptr)
    {
        return {};
    }
    return CollectionPtr(mongoc_client_get_collection(client, database_name.c_str(), collection_name.c_str()));
}

void LogMongoError(const char* operation, const bson_error_t& error)
{
    std::cerr << "[MongoDao] " << operation << " failed: " << memo::db::MongoErrorMessage(error) << std::endl;
}

bool AppendUtf8(bson_t* document, const char* key, const std::string& value)
{
    return bson_append_utf8(document,
                            key,
                            static_cast<int>(std::char_traits<char>::length(key)),
                            value.data(),
                            static_cast<int>(value.size()));
}

std::string GetString(const bson_t* document, const char* key, const std::string& default_value = "")
{
    bson_iter_t iter;
    if (!bson_iter_init_find(&iter, document, key) || !BSON_ITER_HOLDS_UTF8(&iter))
    {
        return default_value;
    }
    uint32_t length = 0;
    const char* value = bson_iter_utf8(&iter, &length);
    return value == nullptr ? default_value : std::string(value, length);
}

int64_t GetInt64(const bson_t* document, const char* key, int64_t default_value = 0)
{
    bson_iter_t iter;
    if (!bson_iter_init_find(&iter, document, key))
    {
        return default_value;
    }
    if (BSON_ITER_HOLDS_INT64(&iter))
    {
        return bson_iter_int64(&iter);
    }
    if (BSON_ITER_HOLDS_INT32(&iter))
    {
        return bson_iter_int32(&iter);
    }
    return default_value;
}

int GetInt(const bson_t* document, const char* key, int default_value = 0)
{
    return static_cast<int>(GetInt64(document, key, default_value));
}

bool AppendMomentItem(bson_t* items, uint32_t index, const MomentItemInfo& item)
{
    bson_t item_document;
    bson_init(&item_document);
    const bool built = BSON_APPEND_INT32(&item_document, "seq", item.seq) &&
                       AppendUtf8(&item_document, "media_type", item.media_type) &&
                       AppendUtf8(&item_document, "media_key", item.media_key) &&
                       AppendUtf8(&item_document, "thumb_key", item.thumb_key) &&
                       AppendUtf8(&item_document, "content", item.content) &&
                       BSON_APPEND_INT32(&item_document, "width", item.width) &&
                       BSON_APPEND_INT32(&item_document, "height", item.height) &&
                       BSON_APPEND_INT32(&item_document, "duration_ms", item.duration_ms);

    const char* key = nullptr;
    char key_buffer[16]{};
    bson_uint32_to_string(index, &key, key_buffer, sizeof(key_buffer));
    const bool appended = built && bson_append_document(items, key, -1, &item_document);
    bson_destroy(&item_document);
    return appended;
}

bool BuildMomentDocument(const MomentContentInfo& content, bson_t* document)
{
    if (!BSON_APPEND_INT64(document, "moment_id", content.moment_id))
    {
        return false;
    }

    bson_t items;
    if (!bson_append_array_begin(document, "items", -1, &items))
    {
        return false;
    }
    for (uint32_t index = 0; index < content.items.size(); ++index)
    {
        if (!AppendMomentItem(&items, index, content.items[index]))
        {
            bson_append_array_end(document, &items);
            return false;
        }
    }
    if (!bson_append_array_end(document, &items))
    {
        return false;
    }
    return BSON_APPEND_INT64(document, "server_time", content.server_time);
}

bool DecodeMomentItems(const bson_t* document, std::vector<MomentItemInfo>* items)
{
    bson_iter_t iter;
    if (!bson_iter_init_find(&iter, document, "items"))
    {
        return true;
    }
    if (!BSON_ITER_HOLDS_ARRAY(&iter))
    {
        return false;
    }

    const uint8_t* array_data = nullptr;
    uint32_t array_length = 0;
    bson_iter_array(&iter, &array_length, &array_data);
    bson_t array_document;
    if (!bson_init_static(&array_document, array_data, array_length))
    {
        return false;
    }

    bson_iter_t item_iter;
    if (!bson_iter_init(&item_iter, &array_document))
    {
        return false;
    }
    while (bson_iter_next(&item_iter))
    {
        if (!BSON_ITER_HOLDS_DOCUMENT(&item_iter))
        {
            continue;
        }
        const uint8_t* item_data = nullptr;
        uint32_t item_length = 0;
        bson_iter_document(&item_iter, &item_length, &item_data);
        bson_t item_document;
        if (!bson_init_static(&item_document, item_data, item_length))
        {
            return false;
        }

        MomentItemInfo item;
        item.seq = GetInt(&item_document, "seq", 0);
        item.media_type = GetString(&item_document, "media_type", "text");
        item.media_key = GetString(&item_document, "media_key", "");
        item.thumb_key = GetString(&item_document, "thumb_key", "");
        item.content = GetString(&item_document, "content", "");
        item.width = GetInt(&item_document, "width", 0);
        item.height = GetInt(&item_document, "height", 0);
        item.duration_ms = GetInt(&item_document, "duration_ms", 0);
        items->push_back(std::move(item));
    }
    return true;
}

} // namespace

MongoDao::MongoDao()
{
    init_ok_ = Init();
}

MongoDao::~MongoDao() = default;

bool MongoDao::Enabled() const
{
    return mongo_dao_modules::IsEnabled(enabled_, init_ok_, pool_.IsOpen());
}

bool MongoDao::Ready() const noexcept
{
    return !required_ || init_ok_;
}

const std::string& MongoDao::StartupError() const noexcept
{
    return startup_error_;
}

bool MongoDao::Init()
{
    auto& cfg = ConfigMgr::Inst();
    enabled_ = ParseBool(cfg.GetValue("Mongo", "Enabled"));
    required_ = enabled_;
    if (!mongo_dao_modules::ShouldInitializeMongo(enabled_))
    {
        std::cerr << "[MongoDao] MongoDB not enabled, Moments content will not be stored" << std::endl;
        return false;
    }

    uri_ = cfg.GetValue("Mongo", "Uri");
    database_name_ = cfg.GetValue("Mongo", "Database");
    moments_collection_name_ = cfg.GetValue("Mongo", "MomentsCollection");
    if (!mongo_dao_modules::HasRequiredConfig(uri_.empty(), database_name_.empty()))
    {
        std::cerr << "[MongoDao] Mongo config missing Uri or Database" << std::endl;
        startup_error_ = "Mongo config missing Uri or Database";
        enabled_ = false;
        return false;
    }
    if (mongo_dao_modules::ShouldUseDefaultMomentsCollection(moments_collection_name_.empty()))
    {
        moments_collection_name_ = mongo_dao_modules::DefaultMomentsCollection();
    }

    std::string error;
    if (!pool_.Open(uri_, error))
    {
        std::cerr << "[MongoDao] init failed: " << error << std::endl;
        startup_error_ = error;
        enabled_ = false;
        return false;
    }
    if (!EnsureIndexes())
    {
        startup_error_ = "Mongo index initialization failed";
        return false;
    }
    return true;
}

bool MongoDao::EnsureIndexes()
{
    if (!mongo_dao_modules::CanEnsureIndexes(pool_.IsOpen()))
    {
        return false;
    }

    auto client = pool_.Acquire();
    auto collection = OpenCollection(client.get(), database_name_, moments_collection_name_);
    if (!client || !collection)
    {
        std::cerr << "[MongoDao] ensure indexes failed: client or collection unavailable" << std::endl;
        return false;
    }

    bson_t keys;
    bson_init(&keys);
    bson_t index_options;
    bson_init(&index_options);
    const bool options_ok = BSON_APPEND_INT32(&keys, "moment_id", 1) &&
                            BSON_APPEND_UTF8(&index_options, "name", "idx_moment_id") &&
                            BSON_APPEND_BOOL(&index_options, "unique", true);
    IndexModelPtr model(options_ok ? mongoc_index_model_new(&keys, &index_options) : nullptr);
    bson_error_t error{};
    mongoc_index_model_t* model_pointer = model.get();
    const bool created =
        model_pointer != nullptr &&
        mongoc_collection_create_indexes_with_opts(collection.get(), &model_pointer, 1, nullptr, nullptr, &error);
    bson_destroy(&index_options);
    bson_destroy(&keys);
    if (!created)
    {
        LogMongoError("ensure indexes", error);
    }
    return created;
}

bool MongoDao::InsertMomentContent(const MomentContentInfo& content)
{
    if (!mongo_dao_modules::CanAccessMomentContent(Enabled(), content.moment_id))
    {
        return false;
    }

    auto client = pool_.Acquire();
    auto collection = OpenCollection(client.get(), database_name_, moments_collection_name_);
    if (!client || !collection)
    {
        return false;
    }

    bson_t filter;
    bson_init(&filter);
    bson_t document;
    bson_init(&document);
    const bool document_ok =
        BSON_APPEND_INT64(&filter, "moment_id", content.moment_id) && BuildMomentDocument(content, &document);
    if (!document_ok)
    {
        bson_destroy(&filter);
        bson_destroy(&document);
        return false;
    }

    bson_t options;
    bson_init(&options);
    BSON_APPEND_BOOL(&options, "upsert", true);
    bson_error_t error{};
    const bool stored = mongoc_collection_replace_one(collection.get(), &filter, &document, &options, nullptr, &error);
    bson_destroy(&options);
    bson_destroy(&document);
    if (!stored)
    {
        LogMongoError("InsertMomentContent", error);
        bson_destroy(&filter);
        return false;
    }

    CursorPtr cursor(mongoc_collection_find_with_opts(collection.get(), &filter, nullptr, nullptr));
    const bson_t* found = nullptr;
    const bool verified = cursor != nullptr && mongoc_cursor_next(cursor.get(), &found);
    if (!verified && cursor != nullptr && mongoc_cursor_error(cursor.get(), &error))
    {
        LogMongoError("InsertMomentContent verify", error);
    }
    bson_destroy(&filter);
    return verified;
}

bool MongoDao::GetMomentContent(int64_t moment_id, MomentContentInfo& content)
{
    if (!mongo_dao_modules::CanAccessMomentContent(Enabled(), moment_id))
    {
        return false;
    }

    auto client = pool_.Acquire();
    auto collection = OpenCollection(client.get(), database_name_, moments_collection_name_);
    if (!client || !collection)
    {
        return false;
    }

    bson_t filter;
    bson_init(&filter);
    BSON_APPEND_INT64(&filter, "moment_id", moment_id);
    CursorPtr cursor(mongoc_collection_find_with_opts(collection.get(), &filter, nullptr, nullptr));
    bson_destroy(&filter);
    if (!cursor)
    {
        return false;
    }

    const bson_t* document = nullptr;
    bson_error_t error{};
    if (!mongoc_cursor_next(cursor.get(), &document))
    {
        if (mongoc_cursor_error(cursor.get(), &error))
        {
            LogMongoError("GetMomentContent", error);
        }
        return false;
    }

    MomentContentInfo decoded;
    decoded.moment_id = GetInt64(document, "moment_id", 0);
    decoded.server_time = GetInt64(document, "server_time", 0);
    if (!DecodeMomentItems(document, &decoded.items))
    {
        return false;
    }
    content = std::move(decoded);
    return true;
}

bool MongoDao::DeleteMomentContent(int64_t moment_id)
{
    if (!mongo_dao_modules::CanAccessMomentContent(Enabled(), moment_id))
    {
        return false;
    }

    auto client = pool_.Acquire();
    auto collection = OpenCollection(client.get(), database_name_, moments_collection_name_);
    if (!client || !collection)
    {
        return false;
    }

    bson_t filter;
    bson_init(&filter);
    BSON_APPEND_INT64(&filter, "moment_id", moment_id);
    bson_error_t error{};
    const bool deleted = mongoc_collection_delete_one(collection.get(), &filter, nullptr, nullptr, &error);
    bson_destroy(&filter);
    if (!deleted)
    {
        LogMongoError("DeleteMomentContent", error);
    }
    return deleted;
}
