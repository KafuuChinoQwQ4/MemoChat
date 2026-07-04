#pragma once

#include <string>

class IChatSessionConfig
{
public:
    virtual ~IChatSessionConfig() = default;

    virtual std::string ChatAuthSecret() const = 0;
    virtual bool FeatureFlagDefaultTrue(const std::string& name) const = 0;
    virtual int HeartbeatRouteRefreshSec() const = 0;
    virtual int LoginOfflinePushDelayMs() const = 0;
    virtual std::string SelfServerName() const = 0;
};
