#include "AIServiceJsonMapper.hpp"

#include "AIServiceAlgorithms.hpp"
#include "AIServiceJsonDtos.hpp"

namespace ai_service_json_mapper
{

bool PopulateModelListFromJson(const memochat::json::JsonValue& result, ai::AIListModelsRsp* reply)
{
    const AIModelListJsonDto dto = AIModelListFromJsonValue(result);
    PopulateModelListReply(dto, reply);
    return ai_service_algorithms::IsSuccessfulModelListPayload(result.isObject(), dto.code);
}

void PopulateRegisterApiProviderFromJson(const memochat::json::JsonValue& result, ai::AIRegisterApiProviderRsp* reply)
{
    PopulateRegisterApiProviderReply(AIRegisterApiProviderFromJsonValue(result), reply);
}

void PopulateKbListFromJson(const memochat::json::JsonValue& result, ai::AIKbListRsp* reply)
{
    PopulateKbListReply(AIKnowledgeBaseListFromJsonValue(result), reply);
}

void PopulateKbDeleteFromJson(const memochat::json::JsonValue& result, ai::AIKbDeleteRsp* reply)
{
    PopulateKbDeleteReply(AIKbDeleteFromJsonValue(result), reply);
}

} // namespace ai_service_json_mapper
