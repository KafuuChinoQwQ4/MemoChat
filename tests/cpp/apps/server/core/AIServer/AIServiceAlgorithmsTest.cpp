#include "AIServiceAlgorithms.hpp"

#include <gtest/gtest.h>

TEST(AIServiceAlgorithmsTest, ModelListSuccessRequiresObjectPayloadAndZeroCode)
{
    EXPECT_TRUE(ai_service_algorithms::IsSuccessfulModelListPayload(true, 0));
    EXPECT_FALSE(ai_service_algorithms::IsSuccessfulModelListPayload(false, 0));
    EXPECT_FALSE(ai_service_algorithms::IsSuccessfulModelListPayload(true, 503));
}

TEST(AIServiceAlgorithmsTest, AgentTaskListLimitFallsBackOnlyForNonPositiveValues)
{
    EXPECT_EQ(ai_service_algorithms::NormalizeAgentTaskListLimit(25, 50), 25);
    EXPECT_EQ(ai_service_algorithms::NormalizeAgentTaskListLimit(0, 50), 50);
    EXPECT_EQ(ai_service_algorithms::NormalizeAgentTaskListLimit(-7, 50), 50);
}
