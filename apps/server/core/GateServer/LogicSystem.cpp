#include "LogicSystem.h"
#include "HttpConnection.h"
#include "GateHttpJsonSupport.h"
#include "PostgresMgr.h"
#include "GateRouteModules.h"
#include "MomentsRouteModules.h"
#include "AIRouteModules.h"
#include "../GateServerHttp3/GateHttp3Connection.h"
#include "../GateServerHttp3/GateHttp3ServiceRoutes.h"
#include "logging/Logger.h"
#include "logging/TraceContext.h"
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <iostream>
#include "json/GlazeCompat.h"

LogicSystem::LogicSystem() {
	RegGet("/healthz", [](std::shared_ptr<HttpConnection> connection) {
		memochat::json::JsonValue root;
		root["status"] = "ok";
		root["service"] = "GateServer";
		connection->_response.result(http::status::ok);
		connection->_response.set(http::field::content_type, "application/json");
		beast::ostream(connection->_response.body()) << root.toStyledString();
		return true;
	});

	RegGet("/readyz", [](std::shared_ptr<HttpConnection> connection) {
		memochat::json::JsonValue root;
		root["status"] = "ready";
		root["service"] = "GateServer";
		connection->_response.result(http::status::ok);
		connection->_response.set(http::field::content_type, "application/json");
		beast::ostream(connection->_response.body()) << root.toStyledString();
		return true;
	});

	RegGet("/get_test", [](std::shared_ptr<HttpConnection> connection) {
		beast::ostream(connection->_response.body()) << "receive get_test req " << std::endl;
		int i = 0;
		for (auto& elem : connection->_get_params) {
			i++;
			beast::ostream(connection->_response.body()) << "param" << i << " key is " << elem.first;
			beast::ostream(connection->_response.body()) << ", " <<  " value is " << elem.second << std::endl;
		}

		connection->_response.set(http::field::content_type, "text/plain");
	});

	RegPost("/test_procedure", [](std::shared_ptr<HttpConnection> connection) {
		auto body_str = boost::beast::buffers_to_string(connection->_request.body().data());
		std::cout << "receive body is " << body_str << std::endl;
		connection->_response.set(http::field::content_type, "text/json");
		memochat::json::JsonValue root;
		memochat::json::JsonReader reader;
		memochat::json::JsonValue src_root;
		bool parse_success = reader.parse(body_str, src_root);
		if (!parse_success) {
			std::cout << "Failed to parse JSON data!" << std::endl;
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		if (!isMember(src_root, "email")) {
			std::cout << "Failed to parse JSON data!" << std::endl;
			root["error"] = ErrorCodes::Error_Json;
			std::string jsonstr = root.toStyledString();
			beast::ostream(connection->_response.body()) << jsonstr;
			return true;
		}

		auto email = src_root["email"].asString();
		int uid = 0;
		std::string name = "";
		PostgresMgr::GetInstance()->TestProcedure(email, uid, name);
		cout << "email is " << email << endl;
		root["error"] = ErrorCodes::Success;
		root["email"] = src_root["email"];
		root["name"] = name;
		root["uid"] = uid;
		std::string jsonstr = root.toStyledString();
		beast::ostream(connection->_response.body()) << jsonstr;
		return true;
	});
	
	// Register H1 routes
	AuthHttpService::RegisterRoutes(*this);
	MediaHttpService::RegisterRoutes(*this);
	ProfileHttpService::RegisterRoutes(*this);
	CallHttpServiceRoutes::RegisterRoutes(*this);
	MomentsHttpServiceRoutes::RegisterRoutes(*this);

	// Register AI routes
	AIHttpServiceRoutes::RegisterRoutes(*this);

	// Register H3 routes from GateServerHttp3
	GateHttp3Service::RegisterRoutes(*this);
}

void LogicSystem::RegGet(std::string url, HttpHandler handler) {
	_get_handlers.insert(make_pair(url, handler));
}

void LogicSystem::RegPost(std::string url, HttpHandler handler) {
	_post_handlers.insert(make_pair(url, handler));
}

LogicSystem::~LogicSystem() {
}

bool LogicSystem::HandleGet(std::string path, std::shared_ptr<HttpConnection> con) {
	if (_get_handlers.find(path) == _get_handlers.end()) {
		return false;
	}

	_get_handlers[path](con);
	return true;
}

bool LogicSystem::HandlePost(std::string path, std::shared_ptr<HttpConnection> con) {
	if (_post_handlers.find(path) == _post_handlers.end()) {
		memolog::LogWarn("gate.http.post.not_found", "post route not found", { {"route", path} });
		return false;
	}

	memolog::TraceContext::SetTraceId(con ? con->_trace_id : "");
	memolog::LogInfo("gate.http.post.dispatch", "dispatch post route", { {"route", path} });
	_post_handlers[path](con);
	return true;
}

void LogicSystem::RegGet(std::string url, Http3Handler handler) {
	_get3_handlers.insert(make_pair(url, handler));
}

void LogicSystem::RegPost(std::string url, Http3Handler handler) {
	_post3_handlers.insert(make_pair(url, handler));
}

bool LogicSystem::HandleGet(std::string path, std::shared_ptr<GateHttp3Connection> con) {
	if (_get3_handlers.find(path) == _get3_handlers.end()) {
		return false;
	}
	memolog::TraceContext::SetTraceId(con ? con->GetTraceId() : "");
	memolog::LogInfo("gate.http3.get.dispatch", "dispatch HTTP/3 GET route", { {"route", path} });
	_get3_handlers[path](con);
	return true;
}

bool LogicSystem::HandlePost(std::string path, std::shared_ptr<GateHttp3Connection> con) {
	if (_post3_handlers.find(path) == _post3_handlers.end()) {
		memolog::LogWarn("gate.http3.post.not_found", "HTTP/3 post route not found", { {"route", path} });
		return false;
	}
	memolog::TraceContext::SetTraceId(con ? con->GetTraceId() : "");
	memolog::LogInfo("gate.http3.post.dispatch", "dispatch HTTP/3 POST route", { {"route", path} });
	_post3_handlers[path](con);
	return true;
}
