#pragma once
#include <boost/beast/http.hpp>
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <memory>
#include <iostream>
#include <unordered_map>
#include <map>
#include <string>
#include <functional>

// --- 新增 JsonCpp 头文件 ---
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

// --- 新增错误码枚举 ---
enum ErrorCodes {
    Success = 0,
    Error_Json = 1001,
    RPCFailed = 1002,
    VarifyExpired = 1003,
    VarifyCodeErr = 1004,
    UserExist = 1005,
    PasswdErr = 1006,
    EmailNotMatch = 1007,
    PasswdUpFailed = 1008,
    PasswdInvalid = 1009,
    TokenInvalid = 1010,
    UidInvalid = 1011, // [新增]
    UserNotExist = 1012,
};

#ifndef USER_INFO_DEF
#define USER_INFO_DEF
struct UserInfo {
    int uid = 0;
    std::string name;
    std::string pwd;
    std::string email;
    std::string nick;
    std::string desc;
    int sex = 0;
    std::string icon;
    std::string back;
};
#endif

struct ApplyInfo {
    ApplyInfo(int uid, std::string name, std::string desc,
        std::string icon, std::string nick, int sex, int status)
        : _uid(uid), _name(name), _desc(desc),
        _icon(icon), _nick(nick), _sex(sex), _status(status) {}
    int _uid;
    std::string _name;
    std::string _desc;
    std::string _icon;
    std::string _nick;
    int _sex;
    int _status;
};

// 2. 定义 Defer 类 (解决 MysqlDao 报错)
class Defer {
public:
    Defer(std::function<void()> func) : _func(func) {}
    ~Defer() {
        if (_func) {
            _func();
        }
    }
private:
    std::function<void()> _func;
};

inline const std::string LOGIN_COUNT = "login_count";
inline const std::string USERTOKENPREFIX = "utoken_";
inline const std::string USERIPPREFIX = "uip_";