#pragma once

#include "common/proto/ai_message.grpc.pb.h"
#include "json/GlazeCompat.h"

namespace ai_service_json_mapper {

bool PopulateModelListFromJson(const memochat::json::JsonValue& result, ai::AIListModelsRsp* reply);
void PopulateRegisterApiProviderFromJson(const memochat::json::JsonValue& result, ai::AIRegisterApiProviderRsp* reply);
void PopulateKbListFromJson(const memochat::json::JsonValue& result, ai::AIKbListRsp* reply);
void PopulateKbDeleteFromJson(const memochat::json::JsonValue& result, ai::AIKbDeleteRsp* reply);

}  // namespace ai_service_json_mapper
