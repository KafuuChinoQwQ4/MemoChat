#include "RedisMgr.hpp"
#include "const.hpp"
#include "ConfigMgr.hpp"
#include <charconv>
#include <unordered_map>
#include <vector>
#include <cstring>

import memochat.gate.redis_mgr_algorithms;

namespace redis_mgr_modules = memochat::gate::redis_mgr::modules;

namespace
{
int ParseIntOrZero(const std::string& raw)
{
    int value = 0;
    const auto [ptr, ec] = std::from_chars(raw.data(), raw.data() + raw.size(), value);
    return !raw.empty() && ec == std::errc{} && ptr == raw.data() + raw.size() ? value : 0;
}
} // namespace

RedisMgr::RedisMgr()
{
    auto& gCfgMgr = ConfigMgr::Inst();
    auto host = gCfgMgr["Redis"]["Host"];
    auto port = gCfgMgr["Redis"]["Port"];
    auto pwd = gCfgMgr["Redis"]["Passwd"];
    auto pool_size_str = gCfgMgr["Redis"]["PoolSize"];
    int pool_size = redis_mgr_modules::NormalizePoolSize(ParseIntOrZero(pool_size_str));
    _con_pool.reset(new RedisConPool(pool_size, host.c_str(), ParseIntOrZero(port), pwd.c_str()));
}

RedisMgr::~RedisMgr()
{
}

bool RedisMgr::Ready() const noexcept
{
    return _con_pool && _con_pool->Ready();
}

const std::string& RedisMgr::StartupError() const noexcept
{
    static const std::string allocation_error = "redis pool allocation failed";
    return _con_pool ? _con_pool->StartupError() : allocation_error;
}

bool RedisMgr::Get(const std::string& key, std::string& value)
{
    auto connect = _con_pool->getConnection();
    if (connect == nullptr)
    {
        return false;
    }
    auto reply = (redisReply*) redisCommand(connect, "GET %s", key.c_str());
    if (reply == NULL)
    {
        _con_pool->returnConnection(connect);
        return false;
    }

    if (!redis_mgr_modules::IsReplyType(reply->type, REDIS_REPLY_STRING))
    {
        freeReplyObject(reply);
        _con_pool->returnConnection(connect);
        return false;
    }

    value = reply->str;
    freeReplyObject(reply);
    _con_pool->returnConnection(connect);
    return true;
}

bool RedisMgr::Set(const std::string& key, const std::string& value)
{
    auto connect = _con_pool->getConnection();
    if (connect == nullptr)
    {
        return false;
    }
    auto reply = (redisReply*) redisCommand(connect, "SET %s %s", key.c_str(), value.c_str());
    if (NULL == reply)
    {
        _con_pool->returnConnection(connect);
        return false;
    }
    if (!redis_mgr_modules::IsStatusOk(reply->type, REDIS_REPLY_STATUS, reply->str))
    {
        freeReplyObject(reply);
        _con_pool->returnConnection(connect);
        return false;
    }
    freeReplyObject(reply);
    _con_pool->returnConnection(connect);
    return true;
}

bool RedisMgr::SetEx(const std::string& key, const std::string& value, int expire_seconds)
{
    auto connect = _con_pool->getConnection();
    if (connect == nullptr)
    {
        return false;
    }
    auto reply =
        static_cast<redisReply*>(redisCommand(connect, "SETEX %s %d %s", key.c_str(), expire_seconds, value.c_str()));
    if (reply == nullptr)
    {
        _con_pool->returnConnection(connect);
        return false;
    }
    const bool ok = redis_mgr_modules::IsStatusOk(reply->type, REDIS_REPLY_STATUS, reply->str);
    freeReplyObject(reply);
    _con_pool->returnConnection(connect);
    return ok;
}

bool RedisMgr::LPush(const std::string& key, const std::string& value)
{
    auto connect = _con_pool->getConnection();
    if (connect == nullptr)
    {
        return false;
    }
    auto reply = (redisReply*) redisCommand(connect, "LPUSH %s %s", key.c_str(), value.c_str());
    if (NULL == reply)
    {
        std::cout << "Execut command [ LPUSH " << key << "  " << value << " ] failure ! " << std::endl;
        freeReplyObject(reply);
        _con_pool->returnConnection(connect);
        return false;
    }

    if (!redis_mgr_modules::IsPositiveIntegerReply(reply->type, REDIS_REPLY_INTEGER, reply->integer))
    {
        std::cout << "Execut command [ LPUSH " << key << "  " << value << " ] failure ! " << std::endl;
        freeReplyObject(reply);
        _con_pool->returnConnection(connect);
        return false;
    }

    freeReplyObject(reply);
    _con_pool->returnConnection(connect);
    return true;
}

bool RedisMgr::LPop(const std::string& key, std::string& value)
{
    auto connect = _con_pool->getConnection();
    if (connect == nullptr)
    {
        return false;
    }
    auto reply = (redisReply*) redisCommand(connect, "LPOP %s ", key.c_str());
    if (reply == nullptr)
    {
        std::cout << "Execut command [ LPOP " << key << " ] failure ! " << std::endl;
        _con_pool->returnConnection(connect);
        return false;
    }

    if (redis_mgr_modules::IsReplyType(reply->type, REDIS_REPLY_NIL))
    {
        std::cout << "Execut command [ LPOP " << key << " ] failure ! " << std::endl;
        freeReplyObject(reply);
        _con_pool->returnConnection(connect);
        return false;
    }

    value = reply->str;
    freeReplyObject(reply);
    _con_pool->returnConnection(connect);
    return true;
}

bool RedisMgr::RPush(const std::string& key, const std::string& value)
{
    auto connect = _con_pool->getConnection();
    if (connect == nullptr)
    {
        return false;
    }
    auto reply = (redisReply*) redisCommand(connect, "RPUSH %s %s", key.c_str(), value.c_str());
    if (NULL == reply)
    {
        std::cout << "Execut command [ RPUSH " << key << "  " << value << " ] failure ! " << std::endl;
        freeReplyObject(reply);
        _con_pool->returnConnection(connect);
        return false;
    }

    if (!redis_mgr_modules::IsPositiveIntegerReply(reply->type, REDIS_REPLY_INTEGER, reply->integer))
    {
        std::cout << "Execut command [ RPUSH " << key << "  " << value << " ] failure ! " << std::endl;
        freeReplyObject(reply);
        _con_pool->returnConnection(connect);
        return false;
    }

    freeReplyObject(reply);
    _con_pool->returnConnection(connect);
    return true;
}
bool RedisMgr::RPop(const std::string& key, std::string& value)
{
    auto connect = _con_pool->getConnection();
    if (connect == nullptr)
    {
        return false;
    }
    auto reply = (redisReply*) redisCommand(connect, "RPOP %s ", key.c_str());
    if (reply == nullptr)
    {
        std::cout << "Execut command [ RPOP " << key << " ] failure ! " << std::endl;
        _con_pool->returnConnection(connect);
        return false;
    }

    if (redis_mgr_modules::IsReplyType(reply->type, REDIS_REPLY_NIL))
    {
        std::cout << "Execut command [ RPOP " << key << " ] failure ! " << std::endl;
        freeReplyObject(reply);
        _con_pool->returnConnection(connect);
        return false;
    }
    value = reply->str;
    freeReplyObject(reply);
    _con_pool->returnConnection(connect);
    return true;
}

bool RedisMgr::HSet(const std::string& key, const std::string& hkey, const std::string& value)
{
    auto connect = _con_pool->getConnection();
    if (connect == nullptr)
    {
        return false;
    }
    auto reply = (redisReply*) redisCommand(connect, "HSET %s %s %s", key.c_str(), hkey.c_str(), value.c_str());
    if (reply == nullptr)
    {
        std::cout << "Execut command [ HSet " << key << "  " << hkey << "  " << value << " ] failure ! " << std::endl;
        _con_pool->returnConnection(connect);
        return false;
    }

    if (!redis_mgr_modules::IsReplyType(reply->type, REDIS_REPLY_INTEGER))
    {
        std::cout << "Execut command [ HSet " << key << "  " << hkey << "  " << value << " ] failure ! " << std::endl;
        freeReplyObject(reply);
        _con_pool->returnConnection(connect);
        return false;
    }

    freeReplyObject(reply);
    _con_pool->returnConnection(connect);
    return true;
}

bool RedisMgr::HSet(const char* key, const char* hkey, const char* hvalue, size_t hvaluelen)
{
    auto connect = _con_pool->getConnection();
    if (connect == nullptr)
    {
        return false;
    }
    const char* argv[4];
    size_t argvlen[4];
    argv[0] = "HSET";
    argvlen[0] = 4;
    argv[1] = key;
    argvlen[1] = strlen(key);
    argv[2] = hkey;
    argvlen[2] = strlen(hkey);
    argv[3] = hvalue;
    argvlen[3] = hvaluelen;

    auto reply = (redisReply*) redisCommandArgv(connect, 4, argv, argvlen);
    if (reply == nullptr)
    {
        std::cout << "Execut command [ HSet " << key << "  " << hkey << "  " << hvalue << " ] failure ! " << std::endl;
        _con_pool->returnConnection(connect);
        return false;
    }

    if (!redis_mgr_modules::IsReplyType(reply->type, REDIS_REPLY_INTEGER))
    {
        std::cout << "Execut command [ HSet " << key << "  " << hkey << "  " << hvalue << " ] failure ! " << std::endl;
        freeReplyObject(reply);
        _con_pool->returnConnection(connect);
        return false;
    }
    freeReplyObject(reply);
    _con_pool->returnConnection(connect);
    return true;
}

std::string RedisMgr::HGet(const std::string& key, const std::string& hkey)
{
    auto connect = _con_pool->getConnection();
    if (connect == nullptr)
    {
        return "";
    }
    const char* argv[3];
    size_t argvlen[3];
    argv[0] = "HGET";
    argvlen[0] = 4;
    argv[1] = key.c_str();
    argvlen[1] = key.length();
    argv[2] = hkey.c_str();
    argvlen[2] = hkey.length();

    auto reply = (redisReply*) redisCommandArgv(connect, 3, argv, argvlen);
    if (reply == nullptr)
    {
        std::cout << "Execut command [ HGet " << key << " " << hkey << "  ] failure ! " << std::endl;
        _con_pool->returnConnection(connect);
        return "";
    }

    if (redis_mgr_modules::IsReplyType(reply->type, REDIS_REPLY_NIL))
    {
        freeReplyObject(reply);
        std::cout << "Execut command [ HGet " << key << " " << hkey << "  ] failure ! " << std::endl;
        _con_pool->returnConnection(connect);
        return "";
    }

    std::string value = reply->str;
    freeReplyObject(reply);
    _con_pool->returnConnection(connect);
    return value;
}

bool RedisMgr::HDel(const std::string& key, const std::string& field)
{
    auto connect = _con_pool->getConnection();
    if (connect == nullptr)
    {
        return false;
    }

    Defer defer(
        [&connect, this]()
        {
            _con_pool->returnConnection(connect);
        });

    redisReply* reply = (redisReply*) redisCommand(connect, "HDEL %s %s", key.c_str(), field.c_str());
    if (reply == nullptr)
    {
        std::cerr << "HDEL command failed" << std::endl;
        return false;
    }

    bool success = false;
    if (redis_mgr_modules::IsReplyType(reply->type, REDIS_REPLY_INTEGER))
    {
        success = redis_mgr_modules::IsPositiveIntegerReply(reply->type, REDIS_REPLY_INTEGER, reply->integer);
    }

    freeReplyObject(reply);
    return success;
}

bool RedisMgr::Del(const std::string& key)
{
    auto connect = _con_pool->getConnection();
    if (connect == nullptr)
    {
        return false;
    }
    auto reply = (redisReply*) redisCommand(connect, "DEL %s", key.c_str());
    if (reply == nullptr)
    {
        std::cout << "Execut command [ Del " << key << " ] failure ! " << std::endl;
        _con_pool->returnConnection(connect);
        return false;
    }

    if (!redis_mgr_modules::IsReplyType(reply->type, REDIS_REPLY_INTEGER))
    {
        std::cout << "Execut command [ Del " << key << " ] failure ! " << std::endl;
        freeReplyObject(reply);
        _con_pool->returnConnection(connect);
        return false;
    }

    freeReplyObject(reply);
    _con_pool->returnConnection(connect);
    return true;
}

bool RedisMgr::ExistsKey(const std::string& key)
{
    auto connect = _con_pool->getConnection();
    if (connect == nullptr)
    {
        return false;
    }

    auto reply = (redisReply*) redisCommand(connect, "exists %s", key.c_str());
    if (reply == nullptr)
    {
        std::cout << "Not Found [ Key " << key << " ]  ! " << std::endl;
        _con_pool->returnConnection(connect);
        return false;
    }

    if (!redis_mgr_modules::IsPositiveIntegerReply(reply->type, REDIS_REPLY_INTEGER, reply->integer))
    {
        std::cout << "Not Found [ Key " << key << " ]  ! " << std::endl;
        _con_pool->returnConnection(connect);
        freeReplyObject(reply);
        return false;
    }
    freeReplyObject(reply);
    _con_pool->returnConnection(connect);
    return true;
}

bool RedisMgr::SCard(const std::string& key, int& count)
{
    count = 0;
    auto connect = _con_pool->getConnection();
    if (connect == nullptr)
    {
        return false;
    }

    auto reply = (redisReply*) redisCommand(connect, "SCARD %s", key.c_str());
    if (reply == nullptr)
    {
        _con_pool->returnConnection(connect);
        return false;
    }

    if (!redis_mgr_modules::IsReplyType(reply->type, REDIS_REPLY_INTEGER))
    {
        freeReplyObject(reply);
        _con_pool->returnConnection(connect);
        return false;
    }

    count = static_cast<int>(reply->integer);
    freeReplyObject(reply);
    _con_pool->returnConnection(connect);
    return true;
}

// =====================================================================
// Pipeline operations — reduce RTT by batching Redis commands
// =====================================================================

std::unordered_map<std::string, std::string> RedisMgr::MGet(const std::vector<std::string>& keys)
{
    std::unordered_map<std::string, std::string> results;

    if (redis_mgr_modules::ShouldSkipBatch(keys.size()))
    {
        return results;
    }

    auto connect = _con_pool->getConnection();
    if (connect == nullptr)
    {
        return results;
    }

    Defer defer(
        [&connect, this]()
        {
            _con_pool->returnConnection(connect);
        });

    // Use MGET command: O(N) single RTT instead of N RTTs
    // Build command arguments
    std::vector<const char*> argv;
    std::vector<size_t> argvlen;
    argv.reserve(keys.size() + 1);
    argvlen.reserve(keys.size() + 1);

    argv.push_back("MGET");
    argvlen.push_back(4);

    for (const auto& key : keys)
    {
        argv.push_back(key.c_str());
        argvlen.push_back(key.size());
    }

    auto* reply = (redisReply*) redisCommandArgv(connect, (int) argv.size(), argv.data(), argvlen.data());
    if (reply == nullptr)
    {
        return results;
    }

    if (redis_mgr_modules::IsReplyType(reply->type, REDIS_REPLY_ARRAY))
    {
        for (size_t i = 0; i < reply->elements && i < keys.size(); ++i)
        {
            auto* elem = reply->element[i];
            if (elem && redis_mgr_modules::IsNonEmptyStringReply(elem->type, REDIS_REPLY_STRING, elem->str, elem->len))
            {
                results[keys[i]] = std::string(elem->str, elem->len);
            }
        }
    }

    freeReplyObject(reply);
    return results;
}

bool RedisMgr::MSet(const std::unordered_map<std::string, std::string>& kvs)
{
    if (redis_mgr_modules::ShouldSkipBatch(kvs.size()))
    {
        return true;
    }

    auto connect = _con_pool->getConnection();
    if (connect == nullptr)
    {
        return false;
    }

    Defer defer(
        [&connect, this]()
        {
            _con_pool->returnConnection(connect);
        });

    // MSET: SET key1 val1 key2 val2 ...
    // Build command arguments
    std::vector<const char*> argv;
    std::vector<size_t> argvlen;
    argv.reserve(kvs.size() * 2 + 1);
    argvlen.reserve(kvs.size() * 2 + 1);

    argv.push_back("MSET");
    argvlen.push_back(4);

    for (const auto& [key, value] : kvs)
    {
        argv.push_back(key.c_str());
        argvlen.push_back(key.size());
        argv.push_back(value.c_str());
        argvlen.push_back(value.size());
    }

    auto* reply = (redisReply*) redisCommandArgv(connect, (int) argv.size(), argv.data(), argvlen.data());
    if (reply == nullptr)
    {
        return false;
    }

    bool ok = redis_mgr_modules::IsReplyType(reply->type, REDIS_REPLY_STATUS);
    freeReplyObject(reply);
    return ok;
}

std::vector<redisReply*> RedisMgr::MPipeline(const std::vector<std::string>& commands)
{
    std::vector<redisReply*> results;

    if (redis_mgr_modules::ShouldSkipBatch(commands.size()))
    {
        return results;
    }

    auto connect = _con_pool->getConnection();
    if (connect == nullptr)
    {
        return results;
    }

    // Append all commands using redisAppendCommand
    for (const auto& cmd : commands)
    {
        if (!redis_mgr_modules::IsRedisOk(redisAppendCommand(connect, cmd.c_str()), REDIS_OK))
        {
            // Connection error, return what we have
            break;
        }
    }

    // Collect all responses
    for (size_t i = 0; i < commands.size(); ++i)
    {
        redisReply* reply = nullptr;
        if (redis_mgr_modules::ShouldCollectPipelineReply(redisGetReply(connect, (void**) &reply),
                                                          REDIS_OK,
                                                          reply != nullptr))
        {
            results.push_back(reply);
        }
        else
        {
            // Early termination
            break;
        }
    }

    _con_pool->returnConnection(connect);
    return results;
}

int64_t
RedisMgr::Eval(const std::string& script, const std::vector<std::string>& keys, const std::vector<std::string>& args)
{
    auto connect = getRawConnection();
    if (connect == nullptr)
    {
        return -1;
    }

    // EVAL script numkeys key1 key2 ... arg1 arg2 ...
    std::vector<const char*> argv;
    std::vector<size_t> argvlen;
    argv.reserve(2 + keys.size() + args.size());
    argvlen.reserve(2 + keys.size() + args.size());

    argv.push_back("EVAL");
    argvlen.push_back(4);

    argv.push_back(script.c_str());
    argvlen.push_back(script.size());

    std::string numkeys_str = std::to_string(keys.size());
    argv.push_back(numkeys_str.c_str());
    argvlen.push_back(numkeys_str.size());

    for (const auto& key : keys)
    {
        argv.push_back(key.c_str());
        argvlen.push_back(key.size());
    }

    for (const auto& arg : args)
    {
        argv.push_back(arg.c_str());
        argvlen.push_back(arg.size());
    }

    auto* reply = (redisReply*) redisCommandArgv(connect, (int) argv.size(), argv.data(), argvlen.data());
    if (reply == nullptr)
    {
        returnConnection(connect);
        return -1;
    }

    int64_t result = -1;
    if (redis_mgr_modules::IsReplyType(reply->type, REDIS_REPLY_INTEGER))
    {
        result = reply->integer;
    }

    freeReplyObject(reply);
    returnConnection(connect);
    return result;
}
