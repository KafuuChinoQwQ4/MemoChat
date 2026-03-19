#pragma once

#include <drogon/HttpController.h>
#include <drogon/HttpResponse.h>
#include <drogon/HttpRequest.h>

class DrogonHttpHandlers
{
public:
    static void HandleGetTest(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
    static void HandleTestProcedure(const drogon::HttpRequestPtr& req, std::function<void(const drogon::HttpResponsePtr&)>&& callback);
};
