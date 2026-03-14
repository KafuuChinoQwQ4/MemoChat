#pragma once
#include "const.h"
#include "logging/Telemetry.h"

#include <memory>

class HttpConnection: public std::enable_shared_from_this<HttpConnection>
{
	friend class LogicSystem;
	friend class AuthHttpService;
	friend class MediaHttpService;
	friend class ProfileHttpService;
	friend class CallHttpServiceRoutes;
	friend class GateHttpJsonSupport;
public:
	HttpConnection(boost::asio::io_context& ioc);
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
	// The buffer for performing reads.
	beast::flat_buffer  _buffer{ 8192 };

	// The request message.
	http::request<http::dynamic_body> _request;

	// The response message.
	http::response<http::dynamic_body> _response;

	// The timer for putting a deadline on connection processing.
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
