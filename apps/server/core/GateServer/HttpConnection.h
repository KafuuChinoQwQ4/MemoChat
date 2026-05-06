#pragma once
#include "const.h"
#include "logging/Telemetry.h"

#include <atomic>
#include <deque>
#include <memory>
#include <string>

class HttpConnection: public std::enable_shared_from_this<HttpConnection>
{
	friend class LogicSystem;
	friend class AuthHttpService;
	friend class MediaHttpService;
	friend class ProfileHttpService;
	friend class CallHttpServiceRoutes;
	friend class GateHttpJsonSupport;
	friend class AIHttpServiceRoutes;
public:
	HttpConnection(boost::asio::io_context& ioc);
	void Start();
	void PreParseGetParam();
	void SetFileResponse(const std::string& file_path, const std::string& content_type);
	tcp::socket& GetSocket() {
		return _socket;
	}
	void SetResponseField(http::field f, const std::string& value) {
		_response.set(f, value);
	}
	http::response<http::dynamic_body>& GetResponse() { return _response; }
	std::string RequestTargetString() const;
	std::string RequestBodyString() const;
	const std::string& GetTraceId() const { return _trace_id; }
	const std::string& GetRequestId() const { return _request_id; }
	void StartSseStream();
	void WriteStreamChunk(std::string chunk);
	void FinishStream();
	bool HasStreamingResponse() const {
		return _streaming_response.load(std::memory_order_acquire);
	}
private:
	using FileResponse = http::response<http::file_body>;
	struct StreamWrite {
		std::string data;
		bool closes = false;
	};

	void CheckDeadline();
	void FinishRequest(beast::error_code ec);
	void WriteErrorResponse(http::status status, const std::string& message);
	void WriteFileResponse();
	void WriteResponse();
	void HandleReq();
	void EnsureSseStreamStarted();
	void QueueStreamWrite(std::string data, bool closes);
	void DoStreamWrite();
	tcp::socket  _socket;
	// The buffer for performing reads.
	beast::flat_buffer  _buffer{ 8192 };

	// The request message.
	http::request<http::dynamic_body> _request;

	// The response message.
	http::response<http::dynamic_body> _response;

	// The timer for putting a deadline on connection processing.
	net::steady_timer deadline_{
		_socket.get_executor(), std::chrono::seconds(600) };

	std::string _get_url;
	std::unordered_map<std::string, std::string> _get_params;
	std::string _trace_id;
	std::string _request_id;
	std::unique_ptr<memolog::SpanScope> _request_span;
	bool _send_file_response = false;
	std::string _send_file_path;
	std::string _send_file_content_type;
	std::shared_ptr<FileResponse> _file_response;
	std::atomic_bool _streaming_response{false};
	bool _stream_header_started = false;
	bool _stream_writing = false;
	bool _stream_finish_requested = false;
	bool _stream_closed = false;
	std::deque<StreamWrite> _stream_write_queue;
};
