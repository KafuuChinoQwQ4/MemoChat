#include "HttpConnection.h"

#include "LogicSystem.h"
#include "GateWorkerPool.h"
#include "GateGlobals.h"
#include "logging/Telemetry.h"
#include "logging/Logger.h"
#include "logging/TraceContext.h"
#include <fstream>
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
        auto self = shared_from_this();
        GateWorkerPool::GetInstance()->post([self]() {
            try {
                auto* ioc = gateglobals::g_main_ioc;
                bool handled = LogicSystem::GetInstance()->HandlePost(self->_request.target(), self);
                boost::asio::post(*ioc, [self, handled]() {
                    if (!handled) {
                        self->_response.result(http::status::not_found);
                        self->_response.set(http::field::content_type, "text/plain");
                        beast::ostream(self->_response.body()) << "url not found\r\n";
                    }
                    self->_response.result(http::status::ok);
                    self->_response.set(http::field::server, "GateServer");
                    self->WriteResponse();
                });
            } catch (const std::exception& e) {
                memolog::LogError("gate.worker.exception", "worker pool exception", {{"error", e.what()}});
                if (auto* ioc = gateglobals::g_main_ioc) {
                    boost::asio::post(*ioc, [self]() {
                        self->WriteErrorResponse(http::status::internal_server_error, "internal error\r\n");
                    });
                }
            }
        });
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

void HttpConnection::FinishRequest(beast::error_code ec) {
    boost::ignore_unused(ec);
    beast::error_code shutdown_ec;
    _socket.shutdown(tcp::socket::shutdown_both, shutdown_ec);
    beast::error_code close_ec;
    _socket.close(close_ec);
    deadline_.cancel();
    _file_response.reset();
    _request_span.reset();
    memolog::TraceContext::Clear();
}

void HttpConnection::WriteErrorResponse(http::status status, const std::string& message) {
    _send_file_response = false;
    _send_file_path.clear();
    _send_file_content_type.clear();
    _response.result(status);
    _response.set(http::field::content_type, "text/plain");
    _response.body().clear();
    beast::ostream(_response.body()) << message;
    WriteResponse();
}

void HttpConnection::WriteFileResponse() {
    _send_file_response = false;
    auto response = std::make_shared<FileResponse>();
    response->version(_request.version());
    response->result(http::status::ok);
    response->keep_alive(false);
    response->set(http::field::server, "GateServer");
    response->set("X-Trace-Id", _trace_id);
    response->set("X-Request-Id", _request_id);
    response->set(http::field::access_control_allow_origin, "*");
    if (!_send_file_content_type.empty()) {
        response->set(http::field::content_type, _send_file_content_type);
    }

    beast::error_code ec;
    response->body().open(_send_file_path.c_str(), beast::file_mode::scan, ec);
    _send_file_path.clear();
    _send_file_content_type.clear();
    if (ec) {
        WriteErrorResponse(http::status::internal_server_error, "open file failed\r\n");
        return;
    }

    response->prepare_payload();
    _file_response = response;
    auto self = shared_from_this();
    http::async_write(_socket, *response, [self, response](beast::error_code write_ec, std::size_t) {
        self->FinishRequest(write_ec);
    });
}

void HttpConnection::WriteResponse() {
    auto self = shared_from_this();

    if (_send_file_response) {
        WriteFileResponse();
        return;
    }

    _response.prepare_payload();

    http::async_write(_socket, _response, [self](beast::error_code ec, std::size_t) {
        self->FinishRequest(ec);
    });
}
