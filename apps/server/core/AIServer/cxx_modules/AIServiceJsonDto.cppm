export module memochat.ai.json_dto_algorithms;

export namespace memochat::ai::json_dto::modules
{
bool ShouldInspectObjectMembers(bool root_is_object)
{
    return root_is_object;
}

bool ShouldReadArrayMember(bool member_found, bool member_is_array)
{
    return member_found && member_is_array;
}

bool ShouldReadObjectMember(bool member_found, bool member_is_object)
{
    return member_found && member_is_object;
}

bool ShouldFallbackToFirstModel(bool explicit_default_model_found, bool models_empty)
{
    return !explicit_default_model_found && !models_empty;
}
} // namespace memochat::ai::json_dto::modules
