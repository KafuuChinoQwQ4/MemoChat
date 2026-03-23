#include "GateHttp3Connection.h"
#include "LogicSystem.h"
#include "logging/Logger.h"
#include "logging/TraceContext.h"

#include <sstream>

GateHttp3Connection::GateHttp3Connection() = default;

GateHttp3Connection::~GateHttp3Connection() = default;

void GateHttp3Connection::OnStreamOpen(int64_t stream_id) {
    stream_id_ = stream_id;
    stream_open_ = true;
    trace_id_ = memolog::TraceContext::NewId();
    request_id_ = memolog::TraceContext::NewId();
    memolog::TraceContext::SetTraceId(trace_id_);
    memolog::LogInfo("http3.stream.open", "HTTP/3 stream opened",
        {{"stream_id", std::to_string(stream_id)},
         {"trace_id", trace_id_}});
}

void GateHttp3Connection::OnStreamClose(int64_t stream_id, uint64_t) {
    stream_open_ = false;
    memolog::LogInfo("http3.stream.close", "HTTP/3 stream closed",
        {{"stream_id", std::to_string(stream_id)}});
    memolog::TraceContext::Clear();
}

void GateHttp3Connection::OnStreamRead(int64_t, const uint8_t* data, size_t len) {
    request_body_.append(reinterpret_cast<const char*>(data), len);
    request_body_received_ += len;

    if (content_length_ >= 0 &&
        static_cast<int64_t>(request_body_received_) >= content_length_) {
        body_complete_ = true;
    }
}

void GateHttp3Connection::OnStreamWrite(int64_t stream_id) {
    (void)stream_id;
}

void GateHttp3Connection::OnStreamWriteComplete(int64_t, size_t written) {
    (void)written;
}

bool GateHttp3Connection::SendResponse(int status, const std::string& body,
                                     const std::string& content_type) {
    response_status_ = status;
    response_body_ = body;
    response_content_type_ = content_type;
    response_complete_ = true;
    memolog::LogInfo("http3.response.prepare", "HTTP/3 response prepared",
        {{"status", std::to_string(status)},
         {"body_len", std::to_string(body.size())},
         {"trace_id", trace_id_}});
    return true;
}

bool GateHttp3Connection::SendData(int64_t, const uint8_t*, size_t) {
    return true;
}

bool GateHttp3Connection::SendHeaders(int64_t, const std::string&, const std::string&) {
    return true;
}

std::string GateHttp3Connection::GetQueryParam(const std::string& key) const {
    auto it = query_params_.find(key);
    if (it != query_params_.end()) {
        return it->second;
    }
    return {};
}

bool GateHttp3Connection::HasQueryParam(const std::string& key) const {
    return query_params_.find(key) != query_params_.end();
}

void GateHttp3Connection::ParseQueryParams() {
    if (query_string_.empty()) return;
    std::istringstream ss(query_string_);
    std::string pair;
    while (std::getline(ss, pair, '&')) {
        auto eqpos = pair.find('=');
        if (eqpos != std::string::npos) {
            std::string key = pair.substr(0, eqpos);
            std::string value = pair.substr(eqpos + 1);
            // URL decode: replace %XX with actual char
            std::string decoded;
            decoded.reserve(value.size());
            for (size_t i = 0; i < value.size(); ++i) {
                if (value[i] == '%' && i + 2 < value.size()) {
                    int ch = std::stoi(value.substr(i + 1, 2), nullptr, 16);
                    decoded.push_back(static_cast<char>(ch));
                    i += 2;
                } else if (value[i] == '+') {
                    decoded.push_back(' ');
                } else {
                    decoded.push_back(value[i]);
                }
            }
            query_params_[key] = decoded;
        } else if (!pair.empty()) {
            query_params_[pair] = "";
        }
    }
}

void GateHttp3Connection::SetQueryString(const std::string& qs) {
    query_string_ = qs;
    ParseQueryParams();
}
