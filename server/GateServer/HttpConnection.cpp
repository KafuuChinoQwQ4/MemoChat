#include "HttpConnection.h"

#include "LogicSystem.h"
#include "logging/Telemetry.h"
#include "logging/Logger.h"
#include "logging/TraceContext.h"
#include <map>

HttpConnection::HttpConnection(boost::asio::io_context& ioc)
    : _socket(ioc) {}

void HttpConnection::SetFileResponse(const std::string& file_path, const std::string& content_type) {
    _send_file_response = true;
    _send_file_path = file_path;
    _send_file_content_type = content_type;
}

void HttpConnection::Start() {
    auto self = shared_from_this();
    http::async_read(_socket, _buffer, _request,
                     [self](beast::error_code ec, std::size_t bytes_transferred) {
                         try {
                             if (ec) {
                                 memolog::LogWarn("http.read.failed", "http read failed", {{"error", ec.what()}});
                                 return;
                             }

                             boost::ignore_unused(bytes_transferred);
                             auto trace_it = self->_request.find("X-Trace-Id");
                             if (trace_it != self->_request.end()) {
                                 self->_trace_id = std::string(trace_it->value().data(), trace_it->value().size());
                             }
                             if (self->_trace_id.empty()) {
                                 self->_trace_id = memolog::TraceContext::NewId();
                             }
                             auto request_it = self->_request.find("X-Request-Id");
                             if (request_it != self->_request.end()) {
                                 self->_request_id = std::string(request_it->value().data(), request_it->value().size());
                             }
                             if (self->_request_id.empty()) {
                                 self->_request_id = memolog::TraceContext::NewId();
                             }
                             memolog::TraceContext::SetTraceId(self->_trace_id);
                             memolog::TraceContext::SetRequestId(self->_request_id);
                             self->_request_span.reset(new memolog::SpanScope(
                                 "GateServer.HttpRequest",
                                 "SERVER",
                                 {{"http.method", std::string(self->_request.method_string())}}));

                             const auto target_sv = self->_request.target();
                             const auto method_sv = self->_request.method_string();
                             std::map<std::string, std::string> fields;
                             fields["route"] = std::string(target_sv.data(), target_sv.size());
                             fields["method"] = std::string(method_sv.data(), method_sv.size());
                             fields["module"] = "http";
                             memolog::LogInfo("http.request.received", "incoming http request", fields);

                             self->HandleReq();
                             self->CheckDeadline();
                         } catch (std::exception& exp) {
                             memolog::LogError("http.request.exception", "http request exception", {{"error", exp.what()}});
                             self->_request_span.reset();
                             memolog::TraceContext::Clear();
                         }
                     });
}

unsigned char ToHex(unsigned char x) {
    return x > 9 ? x + 55 : x + 48;
}

unsigned char FromHex(unsigned char x) {
    unsigned char y;
    if (x >= 'A' && x <= 'Z') {
        y = x - 'A' + 10;
    } else if (x >= 'a' && x <= 'z') {
        y = x - 'a' + 10;
    } else if (x >= '0' && x <= '9') {
        y = x - '0';
    } else {
        assert(0);
    }
    return y;
}

std::string UrlEncode(const std::string& str) {
    std::string strTemp;
    const size_t length = str.length();
    for (size_t i = 0; i < length; i++) {
        if (isalnum((unsigned char)str[i]) || (str[i] == '-') || (str[i] == '_') ||
            (str[i] == '.') || (str[i] == '~')) {
            strTemp += str[i];
        } else if (str[i] == ' ') {
            strTemp += "+";
        } else {
            strTemp += '%';
            strTemp += ToHex((unsigned char)str[i] >> 4);
            strTemp += ToHex((unsigned char)str[i] & 0x0F);
        }
    }
    return strTemp;
}

std::string UrlDecode(const std::string& str) {
    std::string strTemp;
    const size_t length = str.length();
    for (size_t i = 0; i < length; i++) {
        if (str[i] == '+') {
            strTemp += ' ';
        } else if (str[i] == '%') {
            assert(i + 2 < length);
            unsigned char high = FromHex((unsigned char)str[++i]);
            unsigned char low = FromHex((unsigned char)str[++i]);
            strTemp += high * 16 + low;
        } else {
            strTemp += str[i];
        }
    }
    return strTemp;
}

void HttpConnection::PreParseGetParam() {
    auto uri = _request.target();
    auto query_pos = uri.find('?');
    if (query_pos == std::string::npos) {
        _get_url = uri;
        return;
    }

    _get_url = uri.substr(0, query_pos);
    std::string query_string = uri.substr(query_pos + 1);
    std::string key;
    std::string value;
    size_t pos = 0;
    while ((pos = query_string.find('&')) != std::string::npos) {
        auto pair = query_string.substr(0, pos);
        size_t eq_pos = pair.find('=');
        if (eq_pos != std::string::npos) {
            key = UrlDecode(pair.substr(0, eq_pos));
            value = UrlDecode(pair.substr(eq_pos + 1));
            _get_params[key] = value;
        }
        query_string.erase(0, pos + 1);
    }
    if (!query_string.empty()) {
        size_t eq_pos = query_string.find('=');
        if (eq_pos != std::string::npos) {
            key = UrlDecode(query_string.substr(0, eq_pos));
            value = UrlDecode(query_string.substr(eq_pos + 1));
            _get_params[key] = value;
        }
    }
}

void HttpConnection::HandleReq() {
    _response.version(_request.version());
    _response.keep_alive(false);
    _response.set("X-Trace-Id", _trace_id);
    _response.set("X-Request-Id", _request_id);
    _response.set(boost::beast::http::field::access_control_allow_origin, "*");

    if (_request.method() == http::verb::get) {
        PreParseGetParam();
        bool success = LogicSystem::GetInstance()->HandleGet(_get_url, shared_from_this());
        if (!success) {
            _response.result(http::status::not_found);
            _response.set(http::field::content_type, "text/plain");
            beast::ostream(_response.body()) << "url not found\r\n";
            WriteResponse();
            return;
        }

        _response.result(http::status::ok);
        _response.set(http::field::server, "GateServer");
        WriteResponse();
        return;
    }

    if (_request.method() == http::verb::post) {
        bool success = LogicSystem::GetInstance()->HandlePost(_request.target(), shared_from_this());
        if (!success) {
            _response.result(http::status::not_found);
            _response.set(http::field::content_type, "text/plain");
            beast::ostream(_response.body()) << "url not found\r\n";
            WriteResponse();
            return;
        }

        _response.result(http::status::ok);
        _response.set(http::field::server, "GateServer");
        WriteResponse();
        return;
    }
}

void HttpConnection::CheckDeadline() {
    auto self = shared_from_this();

    deadline_.async_wait([self](beast::error_code ec) {
        if (!ec) {
            self->_socket.close(ec);
            self->_request_span.reset();
            memolog::TraceContext::Clear();
        }
    });
}

void HttpConnection::WriteResponse() {
    auto self = shared_from_this();

    if (_send_file_response) {
        beast::error_code ec;
        auto file_res = std::make_shared<http::response<http::file_body>>();
        file_res->version(_response.version());
        file_res->result(_response.result());
        file_res->keep_alive(false);
        for (const auto& field : _response.base()) {
            file_res->set(field.name(), field.value());
        }

        http::file_body::value_type file;
        file.open(_send_file_path.c_str(), beast::file_mode::scan, ec);
        if (ec) {
            _send_file_response = false;
            _response.result(http::status::internal_server_error);
            _response.set(http::field::content_type, "text/plain");
            beast::ostream(_response.body()) << "open file failed\r\n";
            _response.content_length(_response.body().size());
            http::async_write(_socket, _response, [self](beast::error_code write_ec, std::size_t) {
                self->_socket.shutdown(tcp::socket::shutdown_send, write_ec);
                self->deadline_.cancel();
                self->_request_span.reset();
                memolog::TraceContext::Clear();
            });
            return;
        }

        file_res->body() = std::move(file);
        if (!_send_file_content_type.empty()) {
            file_res->set(http::field::content_type, _send_file_content_type);
        }
        file_res->prepare_payload();
        http::async_write(_socket, *file_res, [self, file_res](beast::error_code write_ec, std::size_t) {
            self->_socket.shutdown(tcp::socket::shutdown_send, write_ec);
            self->deadline_.cancel();
            self->_request_span.reset();
            memolog::TraceContext::Clear();
        });
        return;
    }

    _response.content_length(_response.body().size());

    http::async_write(_socket, _response, [self](beast::error_code ec, std::size_t) {
        self->_socket.shutdown(tcp::socket::shutdown_send, ec);
        self->deadline_.cancel();
        self->_request_span.reset();
        memolog::TraceContext::Clear();
    });
}
