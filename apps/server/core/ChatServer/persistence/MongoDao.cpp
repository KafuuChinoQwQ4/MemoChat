#include "MongoDao.hpp"

#include "ConfigMgr.hpp"

#include <chrono>
#include <iostream>

#include <bsoncxx/builder/basic/array.hpp>
#include <bsoncxx/builder/basic/document.hpp>
#include <bsoncxx/builder/basic/kvp.hpp>
#include <bsoncxx/json.hpp>
#include <bsoncxx/types.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/exception/exception.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/options/find.hpp>
#include <mongocxx/options/index.hpp>
#include <mongocxx/options/replace.hpp>
#include <mongocxx/pool.hpp>
#include <mongocxx/uri.hpp>

import memochat.chat.mongo_dao_algorithms;

namespace mongo_dao_modules = memochat::chat::persistence::mongo_dao::modules;

namespace
{
using bsoncxx::builder::basic::kvp;
using bsoncxx::builder::basic::make_document;
using bsoncxx::builder::basic::sub_array;
using bsoncxx::builder::basic::sub_document;

constexpr int64_t kMessageRevokeWindowMsMongoDao = 5 * 60 * 1000;

mongocxx::instance& MongoInstance()
{
    static mongocxx::instance instance{};
    return instance;
}

bool ParseBool(const std::string& raw)
{
    return mongo_dao_modules::ParseBoolText(raw.c_str());
}

std::string GetString(const bsoncxx::document::view& view, const char* key, const std::string& default_value = "")
{
    auto element = view[key];
    if (!element || element.type() != bsoncxx::type::k_string)
    {
        return default_value;
    }
    return std::string(element.get_string().value);
}

int GetInt(const bsoncxx::document::view& view, const char* key, int default_value = 0)
{
    auto element = view[key];
    if (!element)
    {
        return default_value;
    }
    if (element.type() == bsoncxx::type::k_int32)
    {
        return element.get_int32().value;
    }
    if (element.type() == bsoncxx::type::k_int64)
    {
        return static_cast<int>(element.get_int64().value);
    }
    return default_value;
}

int64_t GetInt64(const bsoncxx::document::view& view, const char* key, int64_t default_value = 0)
{
    auto element = view[key];
    if (!element)
    {
        return default_value;
    }
    if (element.type() == bsoncxx::type::k_int64)
    {
        return element.get_int64().value;
    }
    if (element.type() == bsoncxx::type::k_int32)
    {
        return element.get_int32().value;
    }
    return default_value;
}
} // namespace

MongoDao::MongoDao()
{
    init_ok_ = Init();
}

MongoDao::~MongoDao()
{
}

bool MongoDao::Enabled() const
{
    return mongo_dao_modules::IsEnabled(enabled_, init_ok_, pool_ != nullptr);
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

    try
    {
        (void) MongoInstance();
        pool_.reset(new mongocxx::pool(mongocxx::uri{uri_}));
        return EnsureIndexes();
    }
    catch (const std::exception& e)
    {
        std::cerr << "[MongoDao] init failed: " << e.what() << std::endl;
        pool_.reset();
        return false;
    }
}

bool MongoDao::EnsureIndexes()
{
    if (!mongo_dao_modules::CanEnsureIndexes(pool_ != nullptr))
    {
        return false;
    }

    try
    {
        auto client = pool_->acquire();
        auto db = (*client)[database_name_];
        auto private_collection = db[private_collection_name_];
        auto group_collection = db[group_collection_name_];

        mongocxx::options::index private_conv_opts;
        private_conv_opts.name("idx_conv_time");
        private_collection.create_index(make_document(kvp("conv_key", 1), kvp("created_at", -1), kvp("_id", -1)),
                                        private_conv_opts);

        mongocxx::options::index group_seq_opts;
        group_seq_opts.name("idx_group_seq");
        group_seq_opts.unique(true);
        group_collection.create_index(make_document(kvp("group_id", 1), kvp("group_seq", -1), kvp("server_msg_id", -1)),
                                      group_seq_opts);

        mongocxx::options::index group_time_opts;
        group_time_opts.name("idx_group_time");
        group_collection.create_index(make_document(kvp("group_id", 1), kvp("created_at", -1), kvp("_id", -1)),
                                      group_time_opts);

        return true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[MongoDao] ensure indexes failed: " << e.what() << std::endl;
        return false;
    }
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

    try
    {
        auto client = pool_->acquire();
        auto collection = (*client)[database_name_][private_collection_name_];
        bsoncxx::builder::basic::document doc;
        doc.append(kvp("_id", msg.msg_id),
                   kvp("msg_id", msg.msg_id),
                   kvp("conv_key", BuildPrivateConvKey(msg.conv_uid_min, msg.conv_uid_max)),
                   kvp("conv_uid_min", msg.conv_uid_min),
                   kvp("conv_uid_max", msg.conv_uid_max),
                   kvp("from_uid", msg.from_uid),
                   kvp("to_uid", msg.to_uid),
                   kvp("content", msg.content),
                   kvp("reply_to_server_msg_id", msg.reply_to_server_msg_id),
                   kvp("forward_meta_json", msg.forward_meta_json),
                   kvp("edited_at_ms", msg.edited_at_ms),
                   kvp("deleted_at_ms", msg.deleted_at_ms),
                   kvp("created_at", msg.created_at));

        mongocxx::options::replace replace_opts;
        replace_opts.upsert(true);
        collection.replace_one(make_document(kvp("_id", msg.msg_id)), doc.view(), replace_opts);
        return true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[MongoDao] SavePrivateMessage failed: " << e.what() << std::endl;
        return false;
    }
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

    try
    {
        auto client = pool_->acquire();
        auto collection = (*client)[database_name_][private_collection_name_];
        const int final_limit = mongo_dao_modules::ClampHistoryLimit(limit);

        bsoncxx::builder::basic::document filter;
        filter.append(kvp("conv_key", BuildPrivateConvKey(uid, peer_uid)));
        if (mongo_dao_modules::ShouldApplyPrivateTieBreaker(before_ts, before_msg_id.empty()))
        {
            filter.append(kvp("$or",
                              [&](sub_array arr)
                              {
                                  arr.append(make_document(kvp("created_at", make_document(kvp("$lt", before_ts)))));
                                  arr.append(make_document(kvp("created_at", before_ts),
                                                           kvp("_id", make_document(kvp("$lt", before_msg_id)))));
                              }));
        }
        else if (mongo_dao_modules::ShouldApplyTimestampFilter(before_ts))
        {
            filter.append(kvp("created_at", make_document(kvp("$lt", before_ts))));
        }

        mongocxx::options::find opts;
        opts.sort(make_document(kvp("created_at", -1), kvp("_id", -1)));
        opts.limit(final_limit + 1);

        auto cursor = collection.find(filter.view(), opts);
        for (auto&& doc : cursor)
        {
            auto info = std::make_shared<PrivateMessageInfo>();
            info->msg_id = GetString(doc, "_id");
            info->conv_uid_min = GetInt(doc, "conv_uid_min");
            info->conv_uid_max = GetInt(doc, "conv_uid_max");
            info->from_uid = GetInt(doc, "from_uid");
            info->to_uid = GetInt(doc, "to_uid");
            info->content = GetString(doc, "content");
            info->reply_to_server_msg_id = GetInt64(doc, "reply_to_server_msg_id");
            info->forward_meta_json = GetString(doc, "forward_meta_json");
            info->edited_at_ms = GetInt64(doc, "edited_at_ms");
            info->deleted_at_ms = GetInt64(doc, "deleted_at_ms");
            info->created_at = GetInt64(doc, "created_at");
            messages.push_back(info);
        }

        if (mongo_dao_modules::HasMorePage(static_cast<int>(messages.size()), final_limit))
        {
            has_more = true;
            messages.resize(final_limit);
        }
        return true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[MongoDao] GetPrivateHistory failed: " << e.what() << std::endl;
        messages.clear();
        has_more = false;
        return false;
    }
}

bool MongoDao::GetPrivateMessageByMsgId(const std::string& msg_id, std::shared_ptr<PrivateMessageInfo>& message)
{
    message = nullptr;
    if (!mongo_dao_modules::CanFindPrivateMessage(Enabled(), msg_id.empty()))
    {
        return false;
    }

    try
    {
        auto client = pool_->acquire();
        auto collection = (*client)[database_name_][private_collection_name_];
        auto result = collection.find_one(make_document(kvp("_id", msg_id)));
        if (!result)
        {
            return false;
        }

        auto view = result->view();
        auto info = std::make_shared<PrivateMessageInfo>();
        info->msg_id = GetString(view, "_id");
        info->conv_uid_min = GetInt(view, "conv_uid_min");
        info->conv_uid_max = GetInt(view, "conv_uid_max");
        info->from_uid = GetInt(view, "from_uid");
        info->to_uid = GetInt(view, "to_uid");
        info->content = GetString(view, "content");
        info->reply_to_server_msg_id = GetInt64(view, "reply_to_server_msg_id");
        info->forward_meta_json = GetString(view, "forward_meta_json");
        info->edited_at_ms = GetInt64(view, "edited_at_ms");
        info->deleted_at_ms = GetInt64(view, "deleted_at_ms");
        info->created_at = GetInt64(view, "created_at");
        message = info;
        return true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[MongoDao] GetPrivateMessageByMsgId failed: " << e.what() << std::endl;
        return false;
    }
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
    if (!GetPrivateMessageByMsgId(msg_id, message) || !message)
    {
        return false;
    }
    if (!mongo_dao_modules::IsPrivateMessageOwner(message->conv_uid_min,
                                                  message->conv_uid_max,
                                                  message->from_uid,
                                                  uid,
                                                  peer_uid))
    {
        return false;
    }

    try
    {
        auto client = pool_->acquire();
        auto collection = (*client)[database_name_][private_collection_name_];
        auto update = make_document(kvp("$set",
                                        make_document(kvp("content", content),
                                                      kvp("edited_at_ms", edited_at_ms),
                                                      kvp("deleted_at_ms", static_cast<int64_t>(0)))));
        auto result = collection.update_one(make_document(kvp("_id", msg_id)), update.view());
        return result && result->matched_count() > 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[MongoDao] UpdatePrivateMessageContent failed: " << e.what() << std::endl;
        return false;
    }
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

    try
    {
        auto client = pool_->acquire();
        auto collection = (*client)[database_name_][private_collection_name_];
        const int conv_min = mongo_dao_modules::MinInt(uid, peer_uid);
        const int conv_max = mongo_dao_modules::MaxInt(uid, peer_uid);
        auto update = make_document(kvp("$set",
                                        make_document(kvp("content", "[消息已撤回]"),
                                                      kvp("deleted_at_ms", deleted_at_ms),
                                                      kvp("edited_at_ms", static_cast<int64_t>(0)))));
        auto filter = make_document(
            kvp("_id", msg_id),
            kvp("conv_uid_min", conv_min),
            kvp("conv_uid_max", conv_max),
            kvp("from_uid", uid),
            kvp("deleted_at_ms", static_cast<int64_t>(0)),
            kvp("created_at", make_document(kvp("$gte", deleted_at_ms - kMessageRevokeWindowMsMongoDao))));
        auto result = collection.update_one(filter.view(), update.view());
        return result && result->matched_count() > 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[MongoDao] RevokePrivateMessage failed: " << e.what() << std::endl;
        return false;
    }
}

bool MongoDao::SaveGroupMessage(const GroupMessageInfo& msg)
{
    if (!mongo_dao_modules::CanSaveGroupMessage(Enabled(), msg.msg_id.empty(), msg.group_id))
    {
        return false;
    }

    try
    {
        auto client = pool_->acquire();
        auto collection = (*client)[database_name_][group_collection_name_];
        bsoncxx::builder::basic::document doc;
        doc.append(kvp("_id", msg.msg_id),
                   kvp("msg_id", msg.msg_id),
                   kvp("group_id", msg.group_id),
                   kvp("server_msg_id", msg.server_msg_id),
                   kvp("group_seq", msg.group_seq),
                   kvp("from_uid", msg.from_uid),
                   kvp("msg_type", msg.msg_type),
                   kvp("content", msg.content),
                   kvp("mentions_json", msg.mentions_json),
                   kvp("file_name", msg.file_name),
                   kvp("mime", msg.mime),
                   kvp("size", msg.size),
                   kvp("reply_to_server_msg_id", msg.reply_to_server_msg_id),
                   kvp("forward_meta_json", msg.forward_meta_json),
                   kvp("edited_at_ms", msg.edited_at_ms),
                   kvp("deleted_at_ms", msg.deleted_at_ms),
                   kvp("created_at", msg.created_at),
                   kvp("from_name", msg.from_name),
                   kvp("from_nick", msg.from_nick),
                   kvp("from_icon", msg.from_icon));

        mongocxx::options::replace replace_opts;
        replace_opts.upsert(true);
        collection.replace_one(make_document(kvp("_id", msg.msg_id)), doc.view(), replace_opts);
        return true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[MongoDao] SaveGroupMessage failed: " << e.what() << std::endl;
        return false;
    }
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

    try
    {
        auto client = pool_->acquire();
        auto collection = (*client)[database_name_][group_collection_name_];
        const int final_limit = mongo_dao_modules::ClampHistoryLimit(limit);

        bsoncxx::builder::basic::document filter;
        filter.append(kvp("group_id", group_id));
        if (mongo_dao_modules::ShouldApplyGroupSeqFilter(before_seq))
        {
            filter.append(kvp("group_seq", make_document(kvp("$lt", before_seq))));
        }
        else if (mongo_dao_modules::ShouldApplyGroupTimestampFilter(before_seq, before_ts))
        {
            filter.append(kvp("created_at", make_document(kvp("$lt", before_ts))));
        }

        mongocxx::options::find opts;
        opts.sort(make_document(kvp("group_seq", -1), kvp("server_msg_id", -1)));
        opts.limit(final_limit + 1);

        auto cursor = collection.find(filter.view(), opts);
        for (auto&& doc : cursor)
        {
            auto info = std::make_shared<GroupMessageInfo>();
            info->msg_id = GetString(doc, "_id");
            info->group_id = GetInt64(doc, "group_id");
            info->server_msg_id = GetInt64(doc, "server_msg_id");
            info->group_seq = GetInt64(doc, "group_seq");
            info->from_uid = GetInt(doc, "from_uid");
            info->msg_type = GetString(doc, "msg_type", "text");
            info->content = GetString(doc, "content");
            info->mentions_json = GetString(doc, "mentions_json", "[]");
            info->file_name = GetString(doc, "file_name");
            info->mime = GetString(doc, "mime");
            info->size = GetInt(doc, "size");
            info->reply_to_server_msg_id = GetInt64(doc, "reply_to_server_msg_id");
            info->forward_meta_json = GetString(doc, "forward_meta_json");
            info->edited_at_ms = GetInt64(doc, "edited_at_ms");
            info->deleted_at_ms = GetInt64(doc, "deleted_at_ms");
            info->created_at = GetInt64(doc, "created_at");
            info->from_name = GetString(doc, "from_name");
            info->from_nick = GetString(doc, "from_nick");
            info->from_icon = GetString(doc, "from_icon");
            messages.push_back(info);
        }

        if (mongo_dao_modules::HasMorePage(static_cast<int>(messages.size()), final_limit))
        {
            has_more = true;
            messages.resize(final_limit);
        }
        return true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[MongoDao] GetGroupHistory failed: " << e.what() << std::endl;
        messages.clear();
        has_more = false;
        return false;
    }
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

    try
    {
        auto client = pool_->acquire();
        auto collection = (*client)[database_name_][group_collection_name_];
        auto result = collection.find_one(make_document(kvp("group_id", group_id), kvp("_id", msg_id)));
        if (!result)
        {
            return false;
        }

        auto view = result->view();
        auto info = std::make_shared<GroupMessageInfo>();
        info->msg_id = GetString(view, "_id");
        info->group_id = GetInt64(view, "group_id");
        info->server_msg_id = GetInt64(view, "server_msg_id");
        info->group_seq = GetInt64(view, "group_seq");
        info->from_uid = GetInt(view, "from_uid");
        info->msg_type = GetString(view, "msg_type", "text");
        info->content = GetString(view, "content");
        info->mentions_json = GetString(view, "mentions_json", "[]");
        info->file_name = GetString(view, "file_name");
        info->mime = GetString(view, "mime");
        info->size = GetInt(view, "size");
        info->reply_to_server_msg_id = GetInt64(view, "reply_to_server_msg_id");
        info->forward_meta_json = GetString(view, "forward_meta_json");
        info->edited_at_ms = GetInt64(view, "edited_at_ms");
        info->deleted_at_ms = GetInt64(view, "deleted_at_ms");
        info->created_at = GetInt64(view, "created_at");
        info->from_name = GetString(view, "from_name");
        info->from_nick = GetString(view, "from_nick");
        info->from_icon = GetString(view, "from_icon");
        message = info;
        return true;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[MongoDao] GetGroupMessageByMsgId failed: " << e.what() << std::endl;
        return false;
    }
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

    try
    {
        auto client = pool_->acquire();
        auto collection = (*client)[database_name_][group_collection_name_];
        auto update = make_document(kvp("$set",
                                        make_document(kvp("content", content),
                                                      kvp("msg_type", "text"),
                                                      kvp("mentions_json", "[]"),
                                                      kvp("file_name", ""),
                                                      kvp("mime", ""),
                                                      kvp("size", 0),
                                                      kvp("edited_at_ms", edited_at_ms),
                                                      kvp("deleted_at_ms", static_cast<int64_t>(0)))));
        auto result =
            collection.update_one(make_document(kvp("group_id", group_id), kvp("_id", msg_id)), update.view());
        return result && result->matched_count() > 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[MongoDao] UpdateGroupMessageContent failed: " << e.what() << std::endl;
        return false;
    }
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
    if (!GetGroupMessageByMsgId(group_id, msg_id, message) || !message)
    {
        return false;
    }
    if (!mongo_dao_modules::CanApplyGroupMessageRevoke(message->from_uid,
                                                       operator_uid,
                                                       message->deleted_at_ms,
                                                       message->created_at,
                                                       deleted_at_ms,
                                                       kMessageRevokeWindowMsMongoDao))
    {
        return false;
    }

    try
    {
        auto client = pool_->acquire();
        auto collection = (*client)[database_name_][group_collection_name_];
        auto update = make_document(kvp("$set",
                                        make_document(kvp("content", "[消息已撤回]"),
                                                      kvp("deleted_at_ms", deleted_at_ms),
                                                      kvp("edited_at_ms", static_cast<int64_t>(0)))));
        auto filter = make_document(
            kvp("group_id", group_id),
            kvp("_id", msg_id),
            kvp("from_uid", operator_uid),
            kvp("deleted_at_ms", static_cast<int64_t>(0)),
            kvp("created_at", make_document(kvp("$gte", deleted_at_ms - kMessageRevokeWindowMsMongoDao))));
        auto result = collection.update_one(filter.view(), update.view());
        return result && result->matched_count() > 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "[MongoDao] RevokeGroupMessage failed: " << e.what() << std::endl;
        return false;
    }
}
