#include "MongoDao.hpp"

#include "ConfigMgr.hpp"

#include <bson/bson.h>
#include <mongoc/mongoc.h>

#include <chrono>
#include <cstdint>
#include <iostream>
#include <limits>
#include <memory>
#include <string>

import memochat.chat.mongo_dao_algorithms;

namespace mongo_dao_modules = memochat::chat::persistence::mongo_dao::modules;

namespace
{
constexpr int64_t kMessageRevokeWindowMsMongoDao = 5 * 60 * 1000;

class BsonDocument
{
public:
    BsonDocument()
    {
        bson_init(&value_);
    }

    BsonDocument(const BsonDocument&) = delete;
    BsonDocument& operator=(const BsonDocument&) = delete;

    ~BsonDocument()
    {
        bson_destroy(&value_);
    }

    bson_t* get() noexcept
    {
        return &value_;
    }

    const bson_t* get() const noexcept
    {
        return &value_;
    }

private:
    bson_t value_{};
};

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

using CollectionPtr = std::unique_ptr<mongoc_collection_t, CollectionDeleter>;
using CursorPtr = std::unique_ptr<mongoc_cursor_t, CursorDeleter>;

bool ParseBool(const std::string& raw)
{
    return mongo_dao_modules::ParseBoolText(raw.c_str());
}

void LogMongoError(const char* operation, const std::string& error)
{
    std::cerr << "[MongoDao] " << operation << " failed: " << error << std::endl;
}

void LogMongoError(const char* operation, const bson_error_t& error)
{
    LogMongoError(operation, memo::db::MongoErrorMessage(error));
}

bool AppendString(bson_t* document, const char* key, const std::string& value)
{
    if (document == nullptr || value.size() > static_cast<std::size_t>(std::numeric_limits<int>::max()))
    {
        return false;
    }
    return bson_append_utf8(document, key, -1, value.data(), static_cast<int>(value.size()));
}

bool AppendInt(bson_t* document, const char* key, int value)
{
    return document != nullptr && bson_append_int32(document, key, -1, value);
}

bool AppendInt64(bson_t* document, const char* key, int64_t value)
{
    return document != nullptr && bson_append_int64(document, key, -1, value);
}

bool AppendBool(bson_t* document, const char* key, bool value)
{
    return document != nullptr && bson_append_bool(document, key, -1, value);
}

bool AppendDocument(bson_t* parent, const char* key, const BsonDocument& child)
{
    return parent != nullptr && bson_append_document(parent, key, -1, child.get());
}

bool AppendArray(bson_t* parent, const char* key, const BsonDocument& child)
{
    return parent != nullptr && bson_append_array(parent, key, -1, child.get());
}

std::string GetString(const bson_t* document, const char* key, const std::string& default_value = "")
{
    bson_iter_t value{};
    if (document == nullptr || !bson_iter_init_find(&value, document, key) || !BSON_ITER_HOLDS_UTF8(&value))
    {
        return default_value;
    }
    uint32_t length = 0;
    const char* text = bson_iter_utf8(&value, &length);
    return text == nullptr ? default_value : std::string(text, length);
}

int64_t GetInt64(const bson_t* document, const char* key, int64_t default_value = 0)
{
    bson_iter_t value{};
    if (document == nullptr || !bson_iter_init_find(&value, document, key))
    {
        return default_value;
    }
    if (BSON_ITER_HOLDS_INT64(&value))
    {
        return bson_iter_int64(&value);
    }
    if (BSON_ITER_HOLDS_INT32(&value))
    {
        return bson_iter_int32(&value);
    }
    return default_value;
}

int GetInt(const bson_t* document, const char* key, int default_value = 0)
{
    const int64_t value = GetInt64(document, key, default_value);
    if (value < std::numeric_limits<int>::min() || value > std::numeric_limits<int>::max())
    {
        return default_value;
    }
    return static_cast<int>(value);
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

bool CreateIndex(mongoc_collection_t* collection,
                 const BsonDocument& keys,
                 const char* name,
                 bool unique,
                 std::string& error)
{
    BsonDocument options;
    if (!bson_append_utf8(options.get(), "name", -1, name, -1) ||
        (unique && !AppendBool(options.get(), "unique", true)))
    {
        error = "failed to construct index options";
        return false;
    }

    std::unique_ptr<mongoc_index_model_t, decltype(&mongoc_index_model_destroy)> model(
        mongoc_index_model_new(keys.get(), options.get()),
        &mongoc_index_model_destroy);
    if (!model)
    {
        error = "mongoc_index_model_new returned null";
        return false;
    }

    mongoc_index_model_t* models[] = {model.get()};
    BsonDocument reply;
    bson_error_t driver_error{};
    if (!mongoc_collection_create_indexes_with_opts(collection, models, 1, nullptr, reply.get(), &driver_error))
    {
        error = memo::db::MongoErrorMessage(driver_error);
        return false;
    }
    return true;
}

bool ReplaceOne(mongoc_collection_t* collection,
                const BsonDocument& filter,
                const BsonDocument& replacement,
                const char* operation)
{
    BsonDocument options;
    BsonDocument reply;
    if (!AppendBool(options.get(), "upsert", true))
    {
        LogMongoError(operation, "failed to construct replace options");
        return false;
    }
    bson_error_t error{};
    if (!mongoc_collection_replace_one(collection, filter.get(), replacement.get(), options.get(), reply.get(), &error))
    {
        LogMongoError(operation, error);
        return false;
    }
    return true;
}

bool UpdateOneMatched(mongoc_collection_t* collection,
                      const BsonDocument& filter,
                      const BsonDocument& update,
                      const char* operation)
{
    BsonDocument reply;
    bson_error_t error{};
    if (!mongoc_collection_update_one(collection, filter.get(), update.get(), nullptr, reply.get(), &error))
    {
        LogMongoError(operation, error);
        return false;
    }
    return GetInt64(reply.get(), "matchedCount") > 0;
}

bool CursorHasError(mongoc_cursor_t* cursor, const char* operation)
{
    bson_error_t error{};
    if (mongoc_cursor_error(cursor, &error))
    {
        LogMongoError(operation, error);
        return true;
    }
    return false;
}

bool AppendPrivateMessage(BsonDocument& document, const PrivateMessageInfo& msg, const std::string& conv_key)
{
    return AppendString(document.get(), "_id", msg.msg_id) && AppendString(document.get(), "msg_id", msg.msg_id) &&
           AppendString(document.get(), "conv_key", conv_key) &&
           AppendInt(document.get(), "conv_uid_min", msg.conv_uid_min) &&
           AppendInt(document.get(), "conv_uid_max", msg.conv_uid_max) &&
           AppendInt(document.get(), "from_uid", msg.from_uid) && AppendInt(document.get(), "to_uid", msg.to_uid) &&
           AppendString(document.get(), "content", msg.content) &&
           AppendInt64(document.get(), "reply_to_server_msg_id", msg.reply_to_server_msg_id) &&
           AppendString(document.get(), "forward_meta_json", msg.forward_meta_json) &&
           AppendInt64(document.get(), "edited_at_ms", msg.edited_at_ms) &&
           AppendInt64(document.get(), "deleted_at_ms", msg.deleted_at_ms) &&
           AppendInt64(document.get(), "created_at", msg.created_at);
}

bool AppendGroupMessage(BsonDocument& document, const GroupMessageInfo& msg)
{
    return AppendString(document.get(), "_id", msg.msg_id) && AppendString(document.get(), "msg_id", msg.msg_id) &&
           AppendInt64(document.get(), "group_id", msg.group_id) &&
           AppendInt64(document.get(), "server_msg_id", msg.server_msg_id) &&
           AppendInt64(document.get(), "group_seq", msg.group_seq) &&
           AppendInt(document.get(), "from_uid", msg.from_uid) &&
           AppendString(document.get(), "msg_type", msg.msg_type) &&
           AppendString(document.get(), "content", msg.content) &&
           AppendString(document.get(), "mentions_json", msg.mentions_json) &&
           AppendString(document.get(), "file_name", msg.file_name) && AppendString(document.get(), "mime", msg.mime) &&
           AppendInt(document.get(), "size", msg.size) &&
           AppendInt64(document.get(), "reply_to_server_msg_id", msg.reply_to_server_msg_id) &&
           AppendString(document.get(), "forward_meta_json", msg.forward_meta_json) &&
           AppendInt64(document.get(), "edited_at_ms", msg.edited_at_ms) &&
           AppendInt64(document.get(), "deleted_at_ms", msg.deleted_at_ms) &&
           AppendInt64(document.get(), "created_at", msg.created_at) &&
           AppendString(document.get(), "from_name", msg.from_name) &&
           AppendString(document.get(), "from_nick", msg.from_nick) &&
           AppendString(document.get(), "from_icon", msg.from_icon);
}

void ReadPrivateMessage(const bson_t* document, PrivateMessageInfo& info)
{
    info.msg_id = GetString(document, "_id");
    info.conv_uid_min = GetInt(document, "conv_uid_min");
    info.conv_uid_max = GetInt(document, "conv_uid_max");
    info.from_uid = GetInt(document, "from_uid");
    info.to_uid = GetInt(document, "to_uid");
    info.content = GetString(document, "content");
    info.reply_to_server_msg_id = GetInt64(document, "reply_to_server_msg_id");
    info.forward_meta_json = GetString(document, "forward_meta_json");
    info.edited_at_ms = GetInt64(document, "edited_at_ms");
    info.deleted_at_ms = GetInt64(document, "deleted_at_ms");
    info.created_at = GetInt64(document, "created_at");
}

void ReadGroupMessage(const bson_t* document, GroupMessageInfo& info)
{
    info.msg_id = GetString(document, "_id");
    info.group_id = GetInt64(document, "group_id");
    info.server_msg_id = GetInt64(document, "server_msg_id");
    info.group_seq = GetInt64(document, "group_seq");
    info.from_uid = GetInt(document, "from_uid");
    info.msg_type = GetString(document, "msg_type", "text");
    info.content = GetString(document, "content");
    info.mentions_json = GetString(document, "mentions_json", "[]");
    info.file_name = GetString(document, "file_name");
    info.mime = GetString(document, "mime");
    info.size = GetInt(document, "size");
    info.reply_to_server_msg_id = GetInt64(document, "reply_to_server_msg_id");
    info.forward_meta_json = GetString(document, "forward_meta_json");
    info.edited_at_ms = GetInt64(document, "edited_at_ms");
    info.deleted_at_ms = GetInt64(document, "deleted_at_ms");
    info.created_at = GetInt64(document, "created_at");
    info.from_name = GetString(document, "from_name");
    info.from_nick = GetString(document, "from_nick");
    info.from_icon = GetString(document, "from_icon");
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

bool MongoDao::Init()
{
    auto& cfg = ConfigMgr::Inst();
    enabled_ = ParseBool(cfg.GetValue("Mongo", "Enabled"));
    if (!mongo_dao_modules::ShouldInitializeMongo(enabled_))
    {
        return false;
    }

    uri_ = cfg.GetValue("Mongo", "Uri");
    database_name_ = cfg.GetValue("Mongo", "Database");
    private_collection_name_ = cfg.GetValue("Mongo", "PrivateCollection");
    group_collection_name_ = cfg.GetValue("Mongo", "GroupCollection");
    if (!mongo_dao_modules::HasRequiredConfig(uri_.empty(), database_name_.empty()))
    {
        std::cerr << "[MongoDao] Mongo config missing Uri or Database" << std::endl;
        return false;
    }
    if (mongo_dao_modules::ShouldUseDefaultCollectionName(private_collection_name_.empty()))
    {
        private_collection_name_ = "private_messages";
    }
    if (mongo_dao_modules::ShouldUseDefaultCollectionName(group_collection_name_.empty()))
    {
        group_collection_name_ = "group_messages";
    }

    std::string error;
    if (!pool_.Open(uri_, error))
    {
        LogMongoError("init", error);
        return false;
    }
    return EnsureIndexes();
}

bool MongoDao::EnsureIndexes()
{
    if (!mongo_dao_modules::CanEnsureIndexes(pool_.IsOpen()))
    {
        return false;
    }

    auto client = pool_.Acquire();
    if (!client)
    {
        LogMongoError("ensure indexes", "Mongo client pool returned no client");
        return false;
    }
    auto private_collection = OpenCollection(client.get(), database_name_, private_collection_name_);
    auto group_collection = OpenCollection(client.get(), database_name_, group_collection_name_);
    if (!private_collection || !group_collection)
    {
        LogMongoError("ensure indexes", "failed to open Mongo collection");
        return false;
    }

    BsonDocument private_conv_keys;
    BsonDocument group_seq_keys;
    BsonDocument group_time_keys;
    if (!AppendInt(private_conv_keys.get(), "conv_key", 1) || !AppendInt(private_conv_keys.get(), "created_at", -1) ||
        !AppendInt(private_conv_keys.get(), "_id", -1) || !AppendInt(group_seq_keys.get(), "group_id", 1) ||
        !AppendInt(group_seq_keys.get(), "group_seq", -1) || !AppendInt(group_seq_keys.get(), "server_msg_id", -1) ||
        !AppendInt(group_time_keys.get(), "group_id", 1) || !AppendInt(group_time_keys.get(), "created_at", -1) ||
        !AppendInt(group_time_keys.get(), "_id", -1))
    {
        LogMongoError("ensure indexes", "failed to construct index keys");
        return false;
    }

    std::string error;
    if (!CreateIndex(private_collection.get(), private_conv_keys, "idx_conv_time", false, error) ||
        !CreateIndex(group_collection.get(), group_seq_keys, "idx_group_seq", true, error) ||
        !CreateIndex(group_collection.get(), group_time_keys, "idx_group_time", false, error))
    {
        LogMongoError("ensure indexes", error);
        return false;
    }
    return true;
}

std::string MongoDao::BuildPrivateConvKey(int uid_a, int uid_b) const
{
    const int conv_min = mongo_dao_modules::MinInt(uid_a, uid_b);
    const int conv_max = mongo_dao_modules::MaxInt(uid_a, uid_b);
    return std::string("p_") + std::to_string(conv_min) + "_" + std::to_string(conv_max);
}

bool MongoDao::SavePrivateMessage(const PrivateMessageInfo& msg)
{
    if (!mongo_dao_modules::CanSavePrivateMessage(Enabled(), msg.msg_id.empty()))
    {
        return false;
    }

    auto client = pool_.Acquire();
    auto collection = OpenCollection(client.get(), database_name_, private_collection_name_);
    if (!client || !collection)
    {
        LogMongoError("SavePrivateMessage", "failed to acquire Mongo collection");
        return false;
    }

    BsonDocument filter;
    BsonDocument document;
    if (!AppendString(filter.get(), "_id", msg.msg_id) ||
        !AppendPrivateMessage(document, msg, BuildPrivateConvKey(msg.conv_uid_min, msg.conv_uid_max)))
    {
        LogMongoError("SavePrivateMessage", "failed to construct BSON document");
        return false;
    }
    return ReplaceOne(collection.get(), filter, document, "SavePrivateMessage");
}

bool MongoDao::GetPrivateHistory(const int& uid,
                                 const int& peer_uid,
                                 const int64_t& before_ts,
                                 const std::string& before_msg_id,
                                 const int& limit,
                                 std::vector<std::shared_ptr<PrivateMessageInfo>>& messages,
                                 bool& has_more)
{
    messages.clear();
    has_more = false;
    if (!mongo_dao_modules::CanReadPrivateHistory(Enabled(), uid, peer_uid, limit))
    {
        return false;
    }

    auto client = pool_.Acquire();
    auto collection = OpenCollection(client.get(), database_name_, private_collection_name_);
    if (!client || !collection)
    {
        LogMongoError("GetPrivateHistory", "failed to acquire Mongo collection");
        return false;
    }

    const int final_limit = mongo_dao_modules::ClampHistoryLimit(limit);
    BsonDocument filter;
    if (!AppendString(filter.get(), "conv_key", BuildPrivateConvKey(uid, peer_uid)))
    {
        return false;
    }
    if (mongo_dao_modules::ShouldApplyPrivateTieBreaker(before_ts, before_msg_id.empty()))
    {
        BsonDocument created_before;
        BsonDocument first_clause;
        BsonDocument id_before;
        BsonDocument second_clause;
        BsonDocument clauses;
        if (!AppendInt64(created_before.get(), "$lt", before_ts) ||
            !AppendDocument(first_clause.get(), "created_at", created_before) ||
            !AppendInt64(second_clause.get(), "created_at", before_ts) ||
            !AppendString(id_before.get(), "$lt", before_msg_id) ||
            !AppendDocument(second_clause.get(), "_id", id_before) ||
            !AppendDocument(clauses.get(), "0", first_clause) || !AppendDocument(clauses.get(), "1", second_clause) ||
            !AppendArray(filter.get(), "$or", clauses))
        {
            return false;
        }
    }
    else if (mongo_dao_modules::ShouldApplyTimestampFilter(before_ts))
    {
        BsonDocument created_before;
        if (!AppendInt64(created_before.get(), "$lt", before_ts) ||
            !AppendDocument(filter.get(), "created_at", created_before))
        {
            return false;
        }
    }

    BsonDocument sort;
    BsonDocument options;
    if (!AppendInt(sort.get(), "created_at", -1) || !AppendInt(sort.get(), "_id", -1) ||
        !AppendDocument(options.get(), "sort", sort) || !AppendInt64(options.get(), "limit", final_limit + 1))
    {
        return false;
    }

    CursorPtr cursor(mongoc_collection_find_with_opts(collection.get(), filter.get(), options.get(), nullptr));
    if (!cursor)
    {
        LogMongoError("GetPrivateHistory", "mongoc_collection_find_with_opts returned null");
        return false;
    }
    const bson_t* document = nullptr;
    while (mongoc_cursor_next(cursor.get(), &document))
    {
        auto info = std::make_shared<PrivateMessageInfo>();
        ReadPrivateMessage(document, *info);
        messages.push_back(std::move(info));
    }
    if (CursorHasError(cursor.get(), "GetPrivateHistory"))
    {
        messages.clear();
        return false;
    }
    if (mongo_dao_modules::HasMorePage(static_cast<int>(messages.size()), final_limit))
    {
        has_more = true;
        messages.resize(final_limit);
    }
    return true;
}

bool MongoDao::GetPrivateMessageByMsgId(const std::string& msg_id, std::shared_ptr<PrivateMessageInfo>& message)
{
    message = nullptr;
    if (!mongo_dao_modules::CanFindPrivateMessage(Enabled(), msg_id.empty()))
    {
        return false;
    }

    auto client = pool_.Acquire();
    auto collection = OpenCollection(client.get(), database_name_, private_collection_name_);
    if (!client || !collection)
    {
        LogMongoError("GetPrivateMessageByMsgId", "failed to acquire Mongo collection");
        return false;
    }
    BsonDocument filter;
    BsonDocument options;
    if (!AppendString(filter.get(), "_id", msg_id) || !AppendInt64(options.get(), "limit", 1))
    {
        return false;
    }

    CursorPtr cursor(mongoc_collection_find_with_opts(collection.get(), filter.get(), options.get(), nullptr));
    const bson_t* document = nullptr;
    if (!cursor)
    {
        return false;
    }
    if (!mongoc_cursor_next(cursor.get(), &document))
    {
        CursorHasError(cursor.get(), "GetPrivateMessageByMsgId");
        return false;
    }

    auto info = std::make_shared<PrivateMessageInfo>();
    ReadPrivateMessage(document, *info);
    message = std::move(info);
    return true;
}

bool MongoDao::UpdatePrivateMessageContent(const int& uid,
                                           const int& peer_uid,
                                           const std::string& msg_id,
                                           const std::string& content,
                                           int64_t edited_at_ms)
{
    if (!mongo_dao_modules::CanUpdatePrivateMessage(Enabled(), uid, peer_uid, msg_id.empty(), content.empty()))
    {
        return false;
    }
    const auto now_ms = static_cast<int64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count());
    edited_at_ms = mongo_dao_modules::SelectOperationTimestamp(edited_at_ms, now_ms);

    std::shared_ptr<PrivateMessageInfo> message;
    if (!GetPrivateMessageByMsgId(msg_id, message) || !message ||
        !mongo_dao_modules::IsPrivateMessageOwner(message->conv_uid_min,
                                                  message->conv_uid_max,
                                                  message->from_uid,
                                                  uid,
                                                  peer_uid))
    {
        return false;
    }

    auto client = pool_.Acquire();
    auto collection = OpenCollection(client.get(), database_name_, private_collection_name_);
    if (!client || !collection)
    {
        return false;
    }
    BsonDocument filter;
    BsonDocument fields;
    BsonDocument update;
    if (!AppendString(filter.get(), "_id", msg_id) || !AppendString(fields.get(), "content", content) ||
        !AppendInt64(fields.get(), "edited_at_ms", edited_at_ms) || !AppendInt64(fields.get(), "deleted_at_ms", 0) ||
        !AppendDocument(update.get(), "$set", fields))
    {
        return false;
    }
    return UpdateOneMatched(collection.get(), filter, update, "UpdatePrivateMessageContent");
}

bool MongoDao::RevokePrivateMessage(const int& uid,
                                    const int& peer_uid,
                                    const std::string& msg_id,
                                    int64_t deleted_at_ms)
{
    if (!mongo_dao_modules::CanRequestPrivateMessageRevoke(Enabled(), uid, peer_uid, msg_id.empty()))
    {
        return false;
    }
    const auto now_ms = static_cast<int64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count());
    deleted_at_ms = mongo_dao_modules::SelectOperationTimestamp(deleted_at_ms, now_ms);

    std::shared_ptr<PrivateMessageInfo> message;
    if (!GetPrivateMessageByMsgId(msg_id, message) || !message)
    {
        return false;
    }
    const bool owner_matches = mongo_dao_modules::IsPrivateMessageOwner(message->conv_uid_min,
                                                                        message->conv_uid_max,
                                                                        message->from_uid,
                                                                        uid,
                                                                        peer_uid);
    if (!mongo_dao_modules::CanRevokePrivateMessage(owner_matches,
                                                    message->deleted_at_ms,
                                                    message->created_at,
                                                    deleted_at_ms,
                                                    kMessageRevokeWindowMsMongoDao))
    {
        return false;
    }

    auto client = pool_.Acquire();
    auto collection = OpenCollection(client.get(), database_name_, private_collection_name_);
    if (!client || !collection)
    {
        return false;
    }
    BsonDocument created_after;
    BsonDocument filter;
    BsonDocument fields;
    BsonDocument update;
    const int conv_min = mongo_dao_modules::MinInt(uid, peer_uid);
    const int conv_max = mongo_dao_modules::MaxInt(uid, peer_uid);
    if (!AppendString(filter.get(), "_id", msg_id) || !AppendInt(filter.get(), "conv_uid_min", conv_min) ||
        !AppendInt(filter.get(), "conv_uid_max", conv_max) || !AppendInt(filter.get(), "from_uid", uid) ||
        !AppendInt64(filter.get(), "deleted_at_ms", 0) ||
        !AppendInt64(created_after.get(), "$gte", deleted_at_ms - kMessageRevokeWindowMsMongoDao) ||
        !AppendDocument(filter.get(), "created_at", created_after) ||
        !AppendString(fields.get(), "content", "[消息已撤回]") ||
        !AppendInt64(fields.get(), "deleted_at_ms", deleted_at_ms) || !AppendInt64(fields.get(), "edited_at_ms", 0) ||
        !AppendDocument(update.get(), "$set", fields))
    {
        return false;
    }
    return UpdateOneMatched(collection.get(), filter, update, "RevokePrivateMessage");
}

bool MongoDao::SaveGroupMessage(const GroupMessageInfo& msg)
{
    if (!mongo_dao_modules::CanSaveGroupMessage(Enabled(), msg.msg_id.empty(), msg.group_id))
    {
        return false;
    }

    auto client = pool_.Acquire();
    auto collection = OpenCollection(client.get(), database_name_, group_collection_name_);
    if (!client || !collection)
    {
        return false;
    }
    BsonDocument filter;
    BsonDocument document;
    if (!AppendString(filter.get(), "_id", msg.msg_id) || !AppendGroupMessage(document, msg))
    {
        return false;
    }
    return ReplaceOne(collection.get(), filter, document, "SaveGroupMessage");
}

bool MongoDao::GetGroupHistory(const int64_t& group_id,
                               const int64_t& before_ts,
                               const int64_t& before_seq,
                               const int& limit,
                               std::vector<std::shared_ptr<GroupMessageInfo>>& messages,
                               bool& has_more)
{
    messages.clear();
    has_more = false;
    if (!mongo_dao_modules::CanReadGroupHistory(Enabled(), group_id, limit))
    {
        return false;
    }

    auto client = pool_.Acquire();
    auto collection = OpenCollection(client.get(), database_name_, group_collection_name_);
    if (!client || !collection)
    {
        return false;
    }
    const int final_limit = mongo_dao_modules::ClampHistoryLimit(limit);
    BsonDocument filter;
    if (!AppendInt64(filter.get(), "group_id", group_id))
    {
        return false;
    }
    if (mongo_dao_modules::ShouldApplyGroupSeqFilter(before_seq))
    {
        BsonDocument before;
        if (!AppendInt64(before.get(), "$lt", before_seq) || !AppendDocument(filter.get(), "group_seq", before))
        {
            return false;
        }
    }
    else if (mongo_dao_modules::ShouldApplyGroupTimestampFilter(before_seq, before_ts))
    {
        BsonDocument before;
        if (!AppendInt64(before.get(), "$lt", before_ts) || !AppendDocument(filter.get(), "created_at", before))
        {
            return false;
        }
    }

    BsonDocument sort;
    BsonDocument options;
    if (!AppendInt(sort.get(), "group_seq", -1) || !AppendInt(sort.get(), "server_msg_id", -1) ||
        !AppendDocument(options.get(), "sort", sort) || !AppendInt64(options.get(), "limit", final_limit + 1))
    {
        return false;
    }
    CursorPtr cursor(mongoc_collection_find_with_opts(collection.get(), filter.get(), options.get(), nullptr));
    if (!cursor)
    {
        return false;
    }
    const bson_t* document = nullptr;
    while (mongoc_cursor_next(cursor.get(), &document))
    {
        auto info = std::make_shared<GroupMessageInfo>();
        ReadGroupMessage(document, *info);
        messages.push_back(std::move(info));
    }
    if (CursorHasError(cursor.get(), "GetGroupHistory"))
    {
        messages.clear();
        return false;
    }
    if (mongo_dao_modules::HasMorePage(static_cast<int>(messages.size()), final_limit))
    {
        has_more = true;
        messages.resize(final_limit);
    }
    return true;
}

bool MongoDao::GetGroupMessageByMsgId(const int64_t& group_id,
                                      const std::string& msg_id,
                                      std::shared_ptr<GroupMessageInfo>& message)
{
    message = nullptr;
    if (!mongo_dao_modules::CanFindGroupMessage(Enabled(), group_id, msg_id.empty()))
    {
        return false;
    }

    auto client = pool_.Acquire();
    auto collection = OpenCollection(client.get(), database_name_, group_collection_name_);
    if (!client || !collection)
    {
        return false;
    }
    BsonDocument filter;
    BsonDocument options;
    if (!AppendInt64(filter.get(), "group_id", group_id) || !AppendString(filter.get(), "_id", msg_id) ||
        !AppendInt64(options.get(), "limit", 1))
    {
        return false;
    }

    CursorPtr cursor(mongoc_collection_find_with_opts(collection.get(), filter.get(), options.get(), nullptr));
    const bson_t* document = nullptr;
    if (!cursor)
    {
        return false;
    }
    if (!mongoc_cursor_next(cursor.get(), &document))
    {
        CursorHasError(cursor.get(), "GetGroupMessageByMsgId");
        return false;
    }
    auto info = std::make_shared<GroupMessageInfo>();
    ReadGroupMessage(document, *info);
    message = std::move(info);
    return true;
}

bool MongoDao::UpdateGroupMessageContent(const int64_t& group_id,
                                         const int& operator_uid,
                                         const std::string& msg_id,
                                         const std::string& content,
                                         int64_t edited_at_ms)
{
    (void) operator_uid;
    if (!mongo_dao_modules::CanUpdateGroupMessage(Enabled(), group_id, msg_id.empty(), content.empty()))
    {
        return false;
    }
    const auto now_ms = static_cast<int64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count());
    edited_at_ms = mongo_dao_modules::SelectOperationTimestamp(edited_at_ms, now_ms);

    auto client = pool_.Acquire();
    auto collection = OpenCollection(client.get(), database_name_, group_collection_name_);
    if (!client || !collection)
    {
        return false;
    }
    BsonDocument filter;
    BsonDocument fields;
    BsonDocument update;
    if (!AppendInt64(filter.get(), "group_id", group_id) || !AppendString(filter.get(), "_id", msg_id) ||
        !AppendString(fields.get(), "content", content) ||
        !AppendString(fields.get(), "msg_type", std::string("text")) ||
        !AppendString(fields.get(), "mentions_json", std::string("[]")) ||
        !AppendString(fields.get(), "file_name", std::string()) || !AppendString(fields.get(), "mime", std::string()) ||
        !AppendInt(fields.get(), "size", 0) || !AppendInt64(fields.get(), "edited_at_ms", edited_at_ms) ||
        !AppendInt64(fields.get(), "deleted_at_ms", 0) || !AppendDocument(update.get(), "$set", fields))
    {
        return false;
    }
    return UpdateOneMatched(collection.get(), filter, update, "UpdateGroupMessageContent");
}

bool MongoDao::RevokeGroupMessage(const int64_t& group_id,
                                  const int& operator_uid,
                                  const std::string& msg_id,
                                  int64_t deleted_at_ms)
{
    if (!mongo_dao_modules::CanRequestGroupMessageRevoke(Enabled(), group_id, operator_uid, msg_id.empty()))
    {
        return false;
    }
    const auto now_ms = static_cast<int64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch())
            .count());
    deleted_at_ms = mongo_dao_modules::SelectOperationTimestamp(deleted_at_ms, now_ms);

    std::shared_ptr<GroupMessageInfo> message;
    if (!GetGroupMessageByMsgId(group_id, msg_id, message) || !message ||
        !mongo_dao_modules::CanApplyGroupMessageRevoke(message->from_uid,
                                                       operator_uid,
                                                       message->deleted_at_ms,
                                                       message->created_at,
                                                       deleted_at_ms,
                                                       kMessageRevokeWindowMsMongoDao))
    {
        return false;
    }

    auto client = pool_.Acquire();
    auto collection = OpenCollection(client.get(), database_name_, group_collection_name_);
    if (!client || !collection)
    {
        return false;
    }
    BsonDocument created_after;
    BsonDocument filter;
    BsonDocument fields;
    BsonDocument update;
    if (!AppendInt64(filter.get(), "group_id", group_id) || !AppendString(filter.get(), "_id", msg_id) ||
        !AppendInt(filter.get(), "from_uid", operator_uid) || !AppendInt64(filter.get(), "deleted_at_ms", 0) ||
        !AppendInt64(created_after.get(), "$gte", deleted_at_ms - kMessageRevokeWindowMsMongoDao) ||
        !AppendDocument(filter.get(), "created_at", created_after) ||
        !AppendString(fields.get(), "content", "[消息已撤回]") ||
        !AppendInt64(fields.get(), "deleted_at_ms", deleted_at_ms) || !AppendInt64(fields.get(), "edited_at_ms", 0) ||
        !AppendDocument(update.get(), "$set", fields))
    {
        return false;
    }
    return UpdateOneMatched(collection.get(), filter, update, "RevokeGroupMessage");
}
