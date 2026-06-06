#pragma once

#include "ports/IMessageServiceConfig.h"

class MessageServiceConfig : public IMessageServiceConfig
{
public:
    std::string MessageServiceBackend() const override;
    std::string MessageServiceEndpoint() const override;
};
