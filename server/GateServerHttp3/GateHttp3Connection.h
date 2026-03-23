#pragma once
#ifndef MEMOCHAT_GATE_HTTP3_CONNECTION_H
#define MEMOCHAT_GATE_HTTP3_CONNECTION_H

#include "const.h"

#include <atomic>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

class GateHttp3Connection : public std::enable_shared_from_this<GateHttp3Connection>
{
public:
    GateHttp3Connection();
    ~GateHttp3Connection();

    int32_t ConnectionIndex() const { return conn_index_; }
    int64_t StreamId() const { return stream_id_; }
    uint32_t Priority() const { return priority_; }

    void OnStreamOpen(int64_t stream_id);
    void OnStreamClose(int64_t stream_id, uint64_t app_errorcode);
    void OnStreamRead(int64_t stream_id, const uint8_t* data, size_t len);
    void OnStreamWrite(int64_t stream_id);
    void OnStreamWriteComplete(int64_t stream_id, size_t written);

    bool SendResponse(int status, const std::string& body,
                      const std::string& content_type = "application/json");
    bool SendData(int64_t stream_id, const uint8_t* data, size_t len);
    bool SendHeaders(int64_t stream_id, const std::string& path,
                     const std::string& method = "GET");

    std::string GetRequestPath() const { return request_path_; }
    std::string GetRequestMethod() const { return request_method_; }
    std::string GetRequestBody() const { return request_body_; }
    std::string GetTraceId() const { return trace_id_; }
    std::string GetRequestId() const { return request_id_; }
    std::unordered_map<std::string, std::string> GetRequestHeaders() const { return request_headers_; }
    std::string GetQueryString() const { return query_string_; }
    std::string GetQueryParam(const std::string& key) const;
    bool HasQueryParam(const std::string& key) const;

    // Internal setters for connection setup
    void SetConnIndex(int32_t idx) { conn_index_ = idx; }
    void SetHeadersComplete() { headers_received_ = true; }
    void SetStreamId(int64_t sid) { stream_id_ = sid; }
    void SetRequestBody(const std::string& body) { request_body_ = body; }
    int ResponseStatus() const { return response_status_; }
    std::string ResponseBody() const { return response_body_; }
    std::string ResponseContentType() const { return response_content_type_; }
    bool IsResponseComplete() const { return response_complete_.load(); }
    bool IsStreamOpen() const { return stream_open_.load(); }

    // Methods for GateHttp3Listener to use
    std::string& GetRequestMethodRef() { return request_method_; }
    std::string& GetRequestPathRef() { return request_path_; }
    int64_t& GetContentLengthRef() { return content_length_; }
    void SetQueryString(const std::string& qs);

private:
    void ParseQueryParams();

private:
    int32_t conn_index_ = -1;
    int64_t stream_id_ = -1;
    uint32_t priority_ = 0;

    std::atomic<bool> stream_open_{false};
    std::atomic<bool> response_complete_{false};
    std::atomic<bool> body_complete_{false};

    std::string request_path_;
    std::string request_method_;
    std::string request_body_;
    std::string trace_id_;
    std::string request_id_;
    std::unordered_map<std::string, std::string> request_headers_;
    std::string response_body_;
    std::string response_content_type_;
    int response_status_ = 200;

    size_t request_body_received_ = 0;
    int64_t content_length_ = -1;
    bool headers_received_ = false;

    std::vector<uint8_t> pending_data_;
    std::mutex pending_mu_;

    std::string query_string_;
    std::unordered_map<std::string, std::string> query_params_;
};

struct Http3ServerSettings
{
    uint64_t max_data = 16777216;
    uint64_t max_stream_data = 16777216;
    uint64_t max_window = 16777216;
    uint64_t max_cwnd = 16777216;
    uint32_t max_stream_window = 16777216;
    int64_t idle_timeout = 60000;
};

#endif  // MEMOCHAT_GATE_HTTP3_CONNECTION_H
