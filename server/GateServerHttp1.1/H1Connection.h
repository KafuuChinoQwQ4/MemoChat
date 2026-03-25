#pragma once
#include "const.h"
#include "logging/Telemetry.h"

#include <memory>

class H1LogicSystem;
class H1AuthService;
class H1MediaService;
class H1ProfileService;
class H1CallServiceRoutes;
class H1MomentsServiceRoutes;
class H1JsonSupport;

class H1Connection: public std::enable_shared_from_this<H1Connection>
{
	friend class H1LogicSystem;
	friend class H1AuthService;
	friend class H1MediaService;
	friend class H1ProfileService;
	friend class H1CallServiceRoutes;
	friend class H1MomentsServiceRoutes;
	friend class H1JsonSupport;
public:
	H1Connection(boost::asio::io_context& ioc);
	void Start();
	void PreParseGetParam();
	void SetFileResponse(const std::string& file_path, const std::string& content_type);
	tcp::socket& GetSocket() {
		return _socket;
	}
private:
	using FileResponse = http::response<http::file_body>;

	void CheckDeadline();
	void FinishRequest(beast::error_code ec);
	void WriteErrorResponse(http::status status, const std::string& message);
	void WriteFileResponse();
	void WriteResponse();
	void HandleReq();
	tcp::socket  _socket;
	beast::flat_buffer  _buffer{ 8192 };
	http::request<http::dynamic_body> _request;
	http::response<http::dynamic_body> _response;
	net::steady_timer deadline_{
		_socket.get_executor(), std::chrono::seconds(60) };

	std::string _get_url;
	std::unordered_map<std::string, std::string> _get_params;
	std::string _trace_id;
	std::string _request_id;
	std::unique_ptr<memolog::SpanScope> _request_span;
	bool _send_file_response = false;
	std::string _send_file_path;
	std::string _send_file_content_type;
	std::shared_ptr<FileResponse> _file_response;
};
