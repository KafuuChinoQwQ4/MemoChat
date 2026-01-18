#include "LogicSystem.h"
#include "HttpConnection.h"
#include "VerifyGrpcClient.h"
#include "RedisMgr.h"

// 注册 GET 路由
void LogicSystem::RegGet(std::string url, HttpHandler handler) {
    _get_handlers.insert(make_pair(url, handler));
}

// 新增：注册 POST 路由
void LogicSystem::RegPost(std::string url, HttpHandler handler) {
    _post_handlers.insert(make_pair(url, handler));
}

LogicSystem::LogicSystem() {
    // ... GET 请求注册省略 ...

    // 修改 /get_varifycode 的处理逻辑
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
        std::cout << "email is " << email << std::endl;

        // === Day 7 核心修改：调用 gRPC 获取验证码 ===
        GetVarifyRsp rsp = VerifyGrpcClient::GetInstance()->GetVarifyCode(email);
        
        root["error"] = rsp.error();
        root["email"] = src_root["email"];
        if(rsp.error() == ErrorCodes::Success) {
            // 虽然这里还没有真正发邮件，但我们通过 RPC 拿到了假数据
            std::cout << "RPC Success, code: " << rsp.code() << std::endl;
        }

        bool b_set = RedisMgr::GetInstance()->Set(email, rsp.code());
            if(b_set) {
                std::cout << "Saved code to Redis success!" << std::endl;
            } else {
                std::cout << "Failed to save code to Redis!" << std::endl;
            }
        
        beast::ostream(connection->_response.body()) << root.toStyledString();
        return true;
    });
}

// GET 处理
bool LogicSystem::HandleGet(std::string path, std::shared_ptr<HttpConnection> con) {
    if (_get_handlers.find(path) == _get_handlers.end()) {
        return false;
    }
    _get_handlers[path](con);
    return true;
}

// 新增：POST 处理
bool LogicSystem::HandlePost(std::string path, std::shared_ptr<HttpConnection> con) {
    if (_post_handlers.find(path) == _post_handlers.end()) {
        return false;
    }
    _post_handlers[path](con);
    return true;
}