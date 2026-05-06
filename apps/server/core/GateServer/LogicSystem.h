#pragma once
#include "Singleton.h"
#include <functional>
#include <map>
#include <vector>
#include "const.h"

class HttpConnection;
class GateHttp3Connection;
typedef std::function<void(std::shared_ptr<HttpConnection>)> HttpHandler;
typedef std::function<void(std::shared_ptr<GateHttp3Connection>)> Http3Handler;
class LogicSystem :public Singleton<LogicSystem>
{
	friend class Singleton<LogicSystem>;
public:
	~LogicSystem();
	bool HandleGet(std::string, std::shared_ptr<HttpConnection>);
	bool HandleGet(std::string, std::shared_ptr<GateHttp3Connection>);
	void RegGet(std::string, HttpHandler handler);
	void RegGetPrefix(std::string, HttpHandler handler);
	void RegGet(std::string, Http3Handler handler);
	void RegPost(std::string, HttpHandler handler);
	void RegPostPrefix(std::string, HttpHandler handler);
	void RegDelete(std::string, HttpHandler handler);
	void RegDeletePrefix(std::string, HttpHandler handler);
	void RegPost(std::string, Http3Handler handler);
	bool HandlePost(std::string, std::shared_ptr<HttpConnection>);
	bool HandleDelete(std::string, std::shared_ptr<HttpConnection>);
	bool HandlePost(std::string, std::shared_ptr<GateHttp3Connection>);
private:
	LogicSystem();
	std::map<std::string, HttpHandler> _post_handlers;
	std::map<std::string, HttpHandler> _get_handlers;
	std::map<std::string, HttpHandler> _delete_handlers;
	std::vector<std::pair<std::string, HttpHandler>> _post_prefix_handlers;
	std::vector<std::pair<std::string, HttpHandler>> _get_prefix_handlers;
	std::vector<std::pair<std::string, HttpHandler>> _delete_prefix_handlers;
	std::map<std::string, Http3Handler> _post3_handlers;
	std::map<std::string, Http3Handler> _get3_handlers;
};

