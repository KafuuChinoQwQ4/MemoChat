#include "WinCompat.h"
#include "GateServerHttp2.h"
#include "Http2App.h"
#include "Http2Routes.h"
#include <h2o.h>
#include <iostream>
#include <cstdlib>

struct GateServerHttp2Ctx {
    h2o_context_t http2_ctx;
};

static std::unique_ptr<GateServerHttp2Ctx> g_ctx;

void GateServerHttp2::Initialize()
{
    Http2AppCfg::Configure();
    Http2Routes::RegisterRoutes();
}

void GateServerHttp2::Run()
{
    Http2AppCfg::RunEventLoop();
}

void GateServerHttp2::Stop()
{
    // Cleanup if needed
}
