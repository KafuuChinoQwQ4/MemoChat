#pragma once

#include <string>

class IMessageServiceConfig
{
public:
    virtual ~IMessageServiceConfig() = default;

    virtual std::string MessageServiceBackend() const = 0;
    virtual std::string MessageServiceEndpoint() const = 0;
};
