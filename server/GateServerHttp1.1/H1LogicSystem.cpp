#include "H1LogicSystem.h"
#include "H1Connection.h"
#include "H1JsonSupport.h"
#include "H1AuthRoutes.h"
#include "H1MediaService.h"
#include "logging/Logger.h"
#include "logging/TraceContext.h"
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <iostream>
#include <json/json.h>

namespace beast = boost::beast;
namespace http = beast::http;

H1LogicSystem::H1LogicSystem() {
	RegGet("/healthz", [](std::shared_ptr<H1Connection> connection) {
		Json::Value root;
		root["status"] = "ok";
		root["service"] = "GateServerHttp1.1";
		connection->_response.result(http::status::ok);
		connection->_response.set(http::field::content_type, "application/json");
		beast::ostream(connection->_response.body()) << root.toStyledString();
		return true;
	});

	RegGet("/readyz", [](std::shared_ptr<H1Connection> connection) {
		Json::Value root;
		root["status"] = "ready";
		root["service"] = "GateServerHttp1.1";
		connection->_response.result(http::status::ok);
		connection->_response.set(http::field::content_type, "application/json");
		beast::ostream(connection->_response.body()) << root.toStyledString();
		return true;
	});

	RegGet("/get_test", [](std::shared_ptr<H1Connection> connection) {
		beast::ostream(connection->_response.body()) << "receive get_test req " << std::endl;
		int i = 0;
		for (auto& elem : connection->_get_params) {
			i++;
			beast::ostream(connection->_response.body()) << "param" << i << " key is " << elem.first;
			beast::ostream(connection->_response.body()) << ", " <<  " value is " << elem.second << std::endl;
		}
		connection->_response.set(http::field::content_type, "text/plain");
	});

	RegPost("/test_procedure", [](std::shared_ptr<H1Connection> connection) {
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
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}
		if (!src_root.isMember("email")) {
			std::cout << "Failed to parse JSON data!" << std::endl;
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}
		auto email = src_root["email"].asString();
		root["error"] = ErrorCodes::Success;
		root["email"] = src_root["email"];
		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonstr;
		return true;
	});

	H1AuthService::RegisterRoutes(*this);
	H1ProfileService::RegisterRoutes(*this);
	H1CallServiceRoutes::RegisterRoutes(*this);
	H1MomentsServiceRoutes::RegisterRoutes(*this);
	H1MediaService::RegisterRoutes(*this);
}

void H1LogicSystem::RegGet(std::string url, HttpHandler handler) {
	_get_handlers.insert(make_pair(url, handler));
}

void H1LogicSystem::RegPost(std::string url, HttpHandler handler) {
	_post_handlers.insert(make_pair(url, handler));
}

H1LogicSystem::~H1LogicSystem() {
}

bool H1LogicSystem::HandleGet(std::string path, std::shared_ptr<H1Connection> con) {
	if (_get_handlers.find(path) == _get_handlers.end()) {
		return false;
	}
	memolog::TraceContext::SetTraceId(con ? con->_trace_id : "");
	memolog::LogInfo("gate.http1.get.dispatch", "dispatch HTTP/1.1 GET route", { {"route", path} });
	_get_handlers[path](con);
	return true;
}

bool H1LogicSystem::HandlePost(std::string path, std::shared_ptr<H1Connection> con) {
	if (_post_handlers.find(path) == _post_handlers.end()) {
		memolog::LogWarn("gate.http1.post.not_found", "HTTP/1.1 post route not found", { {"route", path} });
		return false;
	}
	memolog::TraceContext::SetTraceId(con ? con->_trace_id : "");
	memolog::LogInfo("gate.http1.post.dispatch", "dispatch HTTP/1.1 POST route", { {"route", path} });
	_post_handlers[path](con);
	return true;
}
