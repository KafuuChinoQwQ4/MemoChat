#include "LogicSystem.h"
#include "HttpConnection.h"
#include "VerifyGrpcClient.h"
#include "RedisMgr.h"
#include "MysqlMgr.h"
#include "const.h"  // 确保包含 ErrorCodes 定义

// 构造函数开始
LogicSystem::LogicSystem() {
    // 1. 获取验证码路由
    RegPost("/get_varifycode", [](std::shared_ptr<HttpConnection> connection) {
        auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
        std::cout << "receive body is " << body_str << std::endl;
        connection->_response.set(http::field::content_type, "text/json");
        Json::Value root;
        Json::Reader reader;
        Json::Value src_root;
        bool parse_success = reader.parse(body_str, src_root);
        if (!parse_success) {
            std::cout << "Failed to parse JSON data!" << std::endl;
            root["error"] = ErrorCodes::Error_Json;
            beast::ostream(connection->_response.body()) << root.toStyledString();
            return true;
        }

        auto email = src_root["email"].asString();
        GetVarifyRsp rsp = VerifyGrpcClient::GetInstance()->GetVarifyCode(email);
        root["error"] = rsp.error();
        root["email"] = src_root["email"];

        if (rsp.error() == ErrorCodes::Success) {
            RedisMgr::GetInstance()->Set(email, rsp.code());
            std::cout << "Saved code to Redis success!" << std::endl;
        }

        beast::ostream(connection->_response.body()) << root.toStyledString();
        return true;
    });

    // 2. 用户注册路由 (我们刚才加的)
    RegPost("/user_register", [](std::shared_ptr<HttpConnection> connection) {
        auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
        std::cout << "receive body is " << body_str << std::endl;
        
        connection->_response.set(http::field::content_type, "text/json");
        Json::Value root;
        Json::Reader reader;
        Json::Value src_root;
        
        if (!reader.parse(body_str, src_root)) {
            root["error"] = ErrorCodes::Error_Json;
            beast::ostream(connection->_response.body()) << root.toStyledString();
            return true;
        }

        auto email = src_root["email"].asString();
        auto name = src_root["user"].asString();
        auto pwd = src_root["passwd"].asString();
        auto confirm = src_root["confirm"].asString();
        auto varify_code = src_root["varifycode"].asString();

        if (pwd != confirm) {
            root["error"] = ErrorCodes::Error_Json; 
            beast::ostream(connection->_response.body()) << root.toStyledString();
            return true;
        }

        std::string redis_code;
        bool b_get_varify = RedisMgr::GetInstance()->Get(email, redis_code);
        if (!b_get_varify) {
            root["error"] = ErrorCodes::Error_Json;
            beast::ostream(connection->_response.body()) << root.toStyledString();
            return true;
        }

        if (redis_code != varify_code) {
            root["error"] = ErrorCodes::Error_Json;
            beast::ostream(connection->_response.body()) << root.toStyledString();
            return true;
        }

        int uid = MysqlMgr::GetInstance()->RegUser(name, email, pwd);
        if (uid == 0 || uid == -1) {
            root["error"] = ErrorCodes::Error_Json;
        } else {
            root["error"] = ErrorCodes::Success;
            root["uid"] = uid;
            root["email"] = email;
            root["user"] = name;
        }

        beast::ostream(connection->_response.body()) << root.toStyledString();
        return true;
    }); 

    RegPost("/reset_pwd", [](std::shared_ptr<HttpConnection> connection) {
        auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
        std::cout << "receive body is " << body_str << std::endl;
        connection->_response.set(http::field::content_type, "text/json");
        Json::Value root;
        Json::Reader reader;
        Json::Value src_root;
        if (!reader.parse(body_str, src_root)) {
            root["error"] = ErrorCodes::Error_Json;
            beast::ostream(connection->_response.body()) << root.toStyledString();
            return true;
        }
    
        auto email = src_root["email"].asString();
        auto name = src_root["user"].asString();
        auto pwd = src_root["passwd"].asString();
        auto varify_code_user = src_root["varifycode"].asString();
    
        // 1. 验证码检查
        std::string varify_code_redis;
        bool b_get_varify = RedisMgr::GetInstance()->Get(email, varify_code_redis);
        if (!b_get_varify) {
            root["error"] = ErrorCodes::VarifyExpired;
            beast::ostream(connection->_response.body()) << root.toStyledString();
            return true;
        }
        if (varify_code_user != varify_code_redis) {
            root["error"] = ErrorCodes::VarifyCodeErr;
            beast::ostream(connection->_response.body()) << root.toStyledString();
            return true;
        }
    
        // 2. 检查邮箱匹配
        bool email_valid = MysqlMgr::GetInstance()->CheckEmail(name, email);
        if (!email_valid) {
            root["error"] = ErrorCodes::EmailNotMatch;
            beast::ostream(connection->_response.body()) << root.toStyledString();
            return true;
        }
    
        // 3. 更新密码
        bool b_up = MysqlMgr::GetInstance()->UpdatePwd(name, pwd);
        if (!b_up) {
            root["error"] = ErrorCodes::PasswdUpFailed;
            beast::ostream(connection->_response.body()) << root.toStyledString();
            return true;
        }
    
        root["error"] = ErrorCodes::Success;
        root["email"] = email;
        root["user"] = name;
        beast::ostream(connection->_response.body()) << root.toStyledString();
        return true;
    });

} // <--- ！！！关键点：这里必须有一个右大括号，结束构造函数！！！

// 下面是其他函数的定义，必须在构造函数外面
bool LogicSystem::HandleGet(std::string path, std::shared_ptr<HttpConnection> con) {
    if (_get_handlers.find(path) == _get_handlers.end()) {
        return false;
    }
    _get_handlers[path](con);
    return true;
}

bool LogicSystem::HandlePost(std::string path, std::shared_ptr<HttpConnection> con) {
    if (_post_handlers.find(path) == _post_handlers.end()) {
        return false;
    }
    _post_handlers[path](con);
    return true;
}

void LogicSystem::RegGet(std::string url, HttpHandler handler) {
    _get_handlers.insert(make_pair(url, handler));
}

void LogicSystem::RegPost(std::string url, HttpHandler handler) {
    _post_handlers.insert(make_pair(url, handler));
}