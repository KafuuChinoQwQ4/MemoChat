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

bool ShouldAcceptJwtAccessClaims(bool jwt_valid, int expected_uid, int claims_uid)
{
    return jwt_valid && expected_uid > 0 && claims_uid == expected_uid;
}
} // namespace memochat::gate::auth::modules
