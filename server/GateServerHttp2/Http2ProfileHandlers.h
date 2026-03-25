#pragma once

#include "Http2Routes.h"

class Http2ProfileHandlers
{
public:
    static void HandleUserUpdateProfile(const Http2Request& req, Http2Response& resp);
    static void HandleGetUserInfo(const Http2Request& req, Http2Response& resp);
};
