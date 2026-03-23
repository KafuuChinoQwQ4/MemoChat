#include "WinCompat.h"
#include "DrogonRoutes.h"
#include "DrogonAuthHandlers.h"
#include "DrogonCallHandlers.h"
#include "DrogonMediaHandlers.h"
#include "DrogonProfileHandlers.h"
#include "DrogonHttpHandlers.h"
#include <drogon/drogon.h>

using namespace drogon;

void DrogonRoutes::RegisterRoutes()
{
    // Health check endpoints
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

    // Auth routes: registered automatically via DrogonAuthHandlers HttpController + METHOD_LIST macros
    // Profile routes: registered automatically via DrogonProfileHandlers HttpController + METHOD_LIST macros
    // Call routes: registered automatically via DrogonCallHandlers HttpController + METHOD_LIST macros
    // Media routes: registered automatically via DrogonMediaHandlers HttpController + METHOD_LIST macros

    // Test handlers (non-HttpController, need manual registration)
    app().registerHandler("/test/get",
        [](const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
            DrogonHttpHandlers::HandleGetTest(req, std::move(callback));
        }, {Get, "NoFilter"});

    app().registerHandler("/test/procedure",
        [](const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) {
            DrogonHttpHandlers::HandleTestProcedure(req, std::move(callback));
        }, {Post, "NoFilter"});
}
