export module memochat.account.async_side_effect_dto_algorithms;

export namespace memochat::account::async_side_effect::modules
{
bool IsValidCacheInvalidatePayloadShape(bool email_is_empty)
{
    return !email_is_empty;
}
} // namespace memochat::account::async_side_effect::modules
