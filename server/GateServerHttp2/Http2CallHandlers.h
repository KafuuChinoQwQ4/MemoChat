#pragma once

#include "Http2Routes.h"

class Http2CallHandlers
{
public:
    static void HandleCallStart(const Http2Request& req, Http2Response& resp);
    static void HandleCallAccept(const Http2Request& req, Http2Response& resp);
    static void HandleCallReject(const Http2Request& req, Http2Response& resp);
    static void HandleCallCancel(const Http2Request& req, Http2Response& resp);
    static void HandleCallHangup(const Http2Request& req, Http2Response& resp);
    static void HandleCallToken(const Http2Request& req, Http2Response& resp);
    static void HandleCallTokenPost(const Http2Request& req, Http2Response& resp);
};
