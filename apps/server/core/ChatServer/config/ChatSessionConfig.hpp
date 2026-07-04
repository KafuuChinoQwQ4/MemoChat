#pragma once

#include "ports/IChatSessionConfig.hpp"

class ChatSessionConfig : public IChatSessionConfig
{
public:
    std::string ChatAuthSecret() const override;
    bool FeatureFlagDefaultTrue(const std::string& name) const override;
    int HeartbeatRouteRefreshSec() const override;
    int LoginOfflinePushDelayMs() const override;
    std::string SelfServerName() const override;

private:
    int ConfigInt(const std::string& section,
                  const std::string& key,
                  int default_value,
                  int min_value,
                  int max_value) const;
};
