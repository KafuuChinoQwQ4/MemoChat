#pragma once

#include "ports/ILogicSystemConfig.hpp"

// Concrete implementation of ILogicSystemConfig — reads values from the
// ini-based ConfigMgr singleton.  Only included by infrastructure/config
// code; domain files depend solely on the interface.
class LogicSystemConfig : public ILogicSystemConfig
{
public:
    std::size_t WorkerCount(std::size_t default_value, std::size_t min_value, std::size_t max_value) const override;
};
