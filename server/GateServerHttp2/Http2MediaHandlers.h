#pragma once

#include "Http2Routes.h"

class Http2MediaHandlers
{
public:
    static void HandleUploadMediaInit(const Http2Request& req, Http2Response& resp);
    static void HandleUploadMediaChunk(const Http2Request& req, Http2Response& resp);
    static void HandleUploadMediaStatus(const Http2Request& req, Http2Response& resp);
    static void HandleUploadMediaComplete(const Http2Request& req, Http2Response& resp);
    static void HandleUploadMedia(const Http2Request& req, Http2Response& resp);
    static void HandleMediaDownload(const Http2Request& req, Http2Response& resp);
};
