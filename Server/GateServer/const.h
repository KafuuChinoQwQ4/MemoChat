#pragma once
#include <boost/beast/http.hpp>
#include <boost/beast.hpp>
#include <boost/asio.hpp>
#include <memory>
#include <iostream>
#include <unordered_map>
#include <map>
#include <string>

// --- 新增 JsonCpp 头文件 ---
#include <json/json.h>
#include <json/value.h>
#include <json/reader.h>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

// --- 新增错误码枚举 ---
enum ErrorCodes {
    Success = 0,
    Error_Json = 1001,  // Json解析错误
    RPCFailed = 1002,   // RPC请求错误
};