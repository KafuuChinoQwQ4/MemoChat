#pragma once

#include "ports/IMessageServiceConfig.hpp"

class MessageServiceConfig : public IMessageServiceConfig
{
public:
    std::string MessageServiceBackend() const override;
    std::string MessageServiceEndpoint() const override;
};
