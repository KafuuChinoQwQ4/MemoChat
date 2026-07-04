#include "AIServiceAlgorithms.hpp"

import memochat.ai.service_algorithms;

namespace ai_service_algorithms
{

bool IsSuccessfulModelListPayload(bool payload_is_object, int code)
{
    return memochat::ai::service::modules::IsSuccessfulModelListPayload(payload_is_object, code);
}

int NormalizeAgentTaskListLimit(int limit, int default_limit)
{
    return memochat::ai::service::modules::NormalizeAgentTaskListLimit(limit, default_limit);
}

} // namespace ai_service_algorithms
