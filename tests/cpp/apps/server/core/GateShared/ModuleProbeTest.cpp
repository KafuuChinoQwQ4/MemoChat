#include "modules/cxx_modules/GateModuleProbeConsumer.hpp"

#include <gtest/gtest.h>

TEST(GateSharedModuleProbeTest, ImportsProjectModuleThroughGateRuntimeCore)
{
    EXPECT_EQ(memochat::gate::modules::cxx::ImportedModuleProbeValue(), 1601);
}
