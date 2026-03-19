#include "DrogonRoutes.h"
#include "DrogonAuthHandlers.h"
#include "DrogonProfileHandlers.h"
#include "DrogonCallHandlers.h"
#include "DrogonMediaHandlers.h"
#include <drogon/drogon.h>

using namespace drogon;

void DrogonRoutes::RegisterRoutes()
{
    // Health check endpoints using lambda handlers
    app().registerHandler("/healthz", 
        [](const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
            auto resp = HttpResponse::newHttpJsonResponse(R"({"status":"ok","service":"GateServerDrogon"})");
            resp->setStatusCode(k200OK);
            callback(resp);
        }, {Get, "NoFilter"});
    
    app().registerHandler("/readyz", 
        [](const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
            auto resp = HttpResponse::newHttpJsonResponse(R"({"status":"ready","service":"GateServerDrogon"})");
            resp->setStatusCode(k200OK);
            callback(resp);
        }, {Get, "NoFilter"});
    
    // Auth routes using HttpController (auto-registration via METHOD_LIST)
    // The METHOD_ADD macros in the header files auto-register these handlers
    // No manual registration needed for HttpController subclasses
    
    // Call routes are handled by DrogonCallHandlers HttpController
    
    // Media routes are handled by DrogonMediaHandlers HttpController
    
    // Note: DrogonHttpHandlers are standalone functions, not HttpController
    // They need to be registered explicitly with lambdas
    app().registerHandler("/test/get", 
        [](const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
            DrogonHttpHandlers::HandleGetTest(req, std::move(callback));
        }, {Get, "NoFilter"});
    
    app().registerHandler("/test/procedure", 
        [](const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
            DrogonHttpHandlers::HandleTestProcedure(req, std::move(callback));
        }, {Post, "NoFilter"});
}
