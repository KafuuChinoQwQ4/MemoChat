export module memochat.ai.public_dto_algorithms;

export namespace memochat::gate::services::ai::modules
{
bool ShouldParseDynamicJson(bool json_text_is_empty)
{
    return !json_text_is_empty;
}

bool ShouldKeepOptionalResponseObject(bool include_object, bool root_is_object)
{
    return include_object || !root_is_object;
}
} // namespace memochat::gate::services::ai::modules
