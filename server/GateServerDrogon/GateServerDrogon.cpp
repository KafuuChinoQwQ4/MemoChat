#include "GateServerDrogon.h"
#include "DrogonAppCfg.h"
#include <drogon/drogon.h>
#include <iostream>

using namespace drogon;

GateServerDrogon::GateServerDrogon()
{
}

void GateServerDrogon::Initialize()
{
    // Initialize Drogon configuration
    DrogonAppCfg::Configure();
    
    // Register routes
    DrogonRoutes::RegisterRoutes();
}

void GateServerDrogon::Start()
{
    // Start Drogon
    app().run();
}

void GateServerDrogon::Stop()
{
    // Cleanup if needed
}
