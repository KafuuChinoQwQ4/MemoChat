#pragma once

#include "Http2Routes.h"

class Http2Handlers
{
public:
    static void HandleGetTest(const Http2Request& req, Http2Response& resp);
    static void HandleTestProcedure(const Http2Request& req, Http2Response& resp);
};
