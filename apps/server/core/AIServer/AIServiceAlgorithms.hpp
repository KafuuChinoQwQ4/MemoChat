#pragma once

namespace ai_service_algorithms
{

bool IsSuccessfulModelListPayload(bool payload_is_object, int code);
int NormalizeAgentTaskListLimit(int limit, int default_limit);

} // namespace ai_service_algorithms
