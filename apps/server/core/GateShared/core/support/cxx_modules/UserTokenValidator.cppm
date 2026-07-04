export module memochat.gate.user_token_validator_algorithms;

export namespace memochat::gate::auth::modules
{
bool ShouldValidateUserToken(int uid, bool token_empty)
{
    return uid > 0 && !token_empty;
}

bool ShouldAcceptStoredUserToken(bool redis_hit, bool stored_token_empty, bool token_matches)
{
    return redis_hit && !stored_token_empty && token_matches;
}
} // namespace memochat::gate::auth::modules
