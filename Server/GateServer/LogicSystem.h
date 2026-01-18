#pragma once
#include "Singleton.h"
#include <functional>
#include <map>
#include "const.h"

class HttpConnection;
typedef std::function<void(std::shared_ptr<HttpConnection>)> HttpHandler;

class LogicSystem : public Singleton<LogicSystem>
{
    friend class Singleton<LogicSystem>;
public:
    ~LogicSystem() {}
    bool HandleGet(std::string, std::shared_ptr<HttpConnection>);
    bool HandlePost(std::string, std::shared_ptr<HttpConnection>); // 新增
    void RegGet(std::string, HttpHandler handler);
    void RegPost(std::string, HttpHandler handler); // 新增

private:
    LogicSystem();
    std::map<std::string, HttpHandler> _post_handlers; // 存放POST回调
    std::map<std::string, HttpHandler> _get_handlers;
};