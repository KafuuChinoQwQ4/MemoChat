#include "LogicSystem.h"
#include "HttpConnection.h"

// 注册 GET 路由
void LogicSystem::RegGet(std::string url, HttpHandler handler) {
    _get_handlers.insert(make_pair(url, handler));
}

// 新增：注册 POST 路由
void LogicSystem::RegPost(std::string url, HttpHandler handler) {
    _post_handlers.insert(make_pair(url, handler));
}

LogicSystem::LogicSystem() {
    // 之前的 GET 测试代码
    RegGet("/get_test", [](std::shared_ptr<HttpConnection> connection) {
        beast::ostream(connection->_response.body()) << "receive get_test req " << std::endl;
        int i = 0;
        for (auto& elem : connection->_get_params) {
            i++;
            beast::ostream(connection->_response.body()) << "param" << i << " key is " << elem.first;
            beast::ostream(connection->_response.body()) << ", " << " value is " << elem.second << std::endl;
        }
    });

    // 新增：POST 获取验证码逻辑
    RegPost("/get_varifycode", [](std::shared_ptr<HttpConnection> connection) {
        // 1. 获取请求体字符串
        auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
        std::cout << "receive body is " << body_str << std::endl;
        
        connection->_response.set(http::field::content_type, "text/json");
        
        // 2. 解析 JSON
        Json::Value root;
        Json::Reader reader;
        Json::Value src_root;
        
        bool parse_success = reader.parse(body_str, src_root);
        if (!parse_success) {
            std::cout << "Failed to parse JSON data!" << std::endl;
            root["error"] = ErrorCodes::Error_Json;
            std::string jsonstr = root.toStyledString();
            beast::ostream(connection->_response.body()) << jsonstr;
            return true;
        }

        // 3. 读取 email 字段并回显
        if (!src_root.isMember("email")) {
             std::cout << "JSON missing email field!" << std::endl;
             root["error"] = ErrorCodes::Error_Json;
             beast::ostream(connection->_response.body()) << root.toStyledString();
             return true;
        }

        auto email = src_root["email"].asString();
        std::cout << "email is " << email << std::endl;

        // 4. 构造返回 JSON
        root["error"] = 0;
        root["email"] = src_root["email"];
        std::string jsonstr = root.toStyledString();
        beast::ostream(connection->_response.body()) << jsonstr;
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