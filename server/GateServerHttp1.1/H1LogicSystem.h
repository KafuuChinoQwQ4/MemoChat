#pragma once
#include "Singleton.h"
#include <functional>
#include <map>
#include "const.h"

class H1Connection;
typedef std::function<void(std::shared_ptr<H1Connection>)> HttpHandler;

class H1LogicSystem :public Singleton<H1LogicSystem>
{
	friend class Singleton<H1LogicSystem>;
public:
	~H1LogicSystem();
	bool HandleGet(std::string, std::shared_ptr<H1Connection>);
	void RegGet(std::string, HttpHandler handler);
	void RegPost(std::string, HttpHandler handler);
	bool HandlePost(std::string, std::shared_ptr<H1Connection>);
private:
	H1LogicSystem();
	std::map<std::string, HttpHandler> _post_handlers;
	std::map<std::string, HttpHandler> _get_handlers;
};
