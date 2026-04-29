#include "AIServiceJsonMapper.h"

namespace {

template <typename T>
void PopulateModelInfo(const T& model_json, ai::ModelInfo* out) {
    out->set_model_type(memochat::json::glaze_safe_get<std::string>(model_json["model_type"], ""));
    out->set_model_name(memochat::json::glaze_safe_get<std::string>(model_json["model_name"], ""));
    out->set_display_name(memochat::json::glaze_safe_get<std::string>(model_json["display_name"], out->model_name()));
    out->set_is_enabled(memochat::json::glaze_safe_get<bool>(model_json["is_enabled"], true));
    out->set_context_window(memochat::json::glaze_safe_get<int64_t>(model_json["context_window"], 0));
    out->set_supports_thinking(memochat::json::glaze_safe_get<bool>(model_json["supports_thinking"], false));
}

}  // namespace

namespace ai_service_json_mapper {

bool PopulateModelListFromJson(const memochat::json::JsonValue& result, ai::AIListModelsRsp* reply) {
    reply->clear_models();
    reply->clear_default_model();
    reply->set_code(memochat::json::glaze_safe_get<int>(result["code"], 0));

    if (result.isObject()) {
        const auto& object = result.impl().get<memochat::json::object_t>();
        auto models_it = object.find("models");
        if (models_it != object.end() && models_it->second.holds<memochat::json::array_t>()) {
            for (const auto& model_item : models_it->second.get<memochat::json::array_t>()) {
                auto* model = reply->add_models();
                PopulateModelInfo(memochat::json::JsonMemberRef(model_item), model);
            }
        }

        auto default_model_it = object.find("default_model");
        if (default_model_it != object.end() && default_model_it->second.holds<memochat::json::object_t>()) {
            PopulateModelInfo(memochat::json::JsonValue(default_model_it->second), reply->mutable_default_model());
        } else if (reply->models_size() > 0) {
            reply->mutable_default_model()->CopyFrom(reply->models(0));
        }
    }

    return reply->models_size() > 0;
}

void PopulateRegisterApiProviderFromJson(const memochat::json::JsonValue& result, ai::AIRegisterApiProviderRsp* reply) {
    reply->clear_models();
    reply->set_code(memochat::json::glaze_safe_get<int>(result["code"], 0));
    reply->set_message(memochat::json::glaze_safe_get<std::string>(result["message"], "ok"));
    reply->set_provider_id(memochat::json::glaze_safe_get<std::string>(result["provider_id"], ""));

    if (!result.isObject()) {
        return;
    }

    const auto& object = result.impl().get<memochat::json::object_t>();
    auto models_it = object.find("models");
    if (models_it == object.end() || !models_it->second.holds<memochat::json::array_t>()) {
        return;
    }

    for (const auto& model_item : models_it->second.get<memochat::json::array_t>()) {
        auto* model = reply->add_models();
        PopulateModelInfo(memochat::json::JsonMemberRef(model_item), model);
    }
}

void PopulateKbListFromJson(const memochat::json::JsonValue& result, ai::AIKbListRsp* reply) {
    reply->clear_knowledge_bases();
    reply->set_code(memochat::json::glaze_safe_get<int>(result["code"], 0));

    if (!result.isObject()) {
        return;
    }

    const auto& object = result.impl().get<memochat::json::object_t>();
    auto knowledge_bases_it = object.find("knowledge_bases");
    if (knowledge_bases_it == object.end() || !knowledge_bases_it->second.holds<memochat::json::array_t>()) {
        return;
    }

    for (const auto& kb_item : knowledge_bases_it->second.get<memochat::json::array_t>()) {
        memochat::json::JsonValue kb(kb_item);
        auto* info = reply->add_knowledge_bases();
        info->set_kb_id(memochat::json::glaze_safe_get<std::string>(kb["kb_id"], ""));
        info->set_name(memochat::json::glaze_safe_get<std::string>(kb["name"], info->kb_id()));
        info->set_chunk_count(memochat::json::glaze_safe_get<int>(kb["chunk_count"], 0));
        info->set_created_at(memochat::json::glaze_safe_get<int64_t>(kb["created_at"], 0));
        info->set_status(memochat::json::glaze_safe_get<std::string>(kb["status"], "ready"));
    }
}

void PopulateKbDeleteFromJson(const memochat::json::JsonValue& result, ai::AIKbDeleteRsp* reply) {
    reply->set_code(memochat::json::glaze_safe_get<int>(result["code"], 0));
    reply->set_message(memochat::json::glaze_safe_get<std::string>(result["message"], "ok"));
}

}  // namespace ai_service_json_mapper
