#pragma once

#include <drogon/HttpSimpleController.h>

using namespace drogon;

class DrogonAuthHandlers : public drogon::HttpSimpleController<DrogonAuthHandlers>
{
  public:
    virtual void asyncHandleHttpRequest(const HttpRequestPtr& req, std::function<void(const HttpResponsePtr&)>&& callback) override;
};
