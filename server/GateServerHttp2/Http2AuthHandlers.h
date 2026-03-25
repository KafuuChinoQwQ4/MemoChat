#pragma once

#include "Http2Routes.h"

class Http2AuthHandlers
{
public:
    static void HandleGetVarifyCode(const Http2Request& req, Http2Response& resp);
    static void HandleUserRegister(const Http2Request& req, Http2Response& resp);
    static void HandleResetPwd(const Http2Request& req, Http2Response& resp);
    static void HandleUserLogin(const Http2Request& req, Http2Response& resp);
    static void HandleUserLogout(const Http2Request& req, Http2Response& resp);
};
