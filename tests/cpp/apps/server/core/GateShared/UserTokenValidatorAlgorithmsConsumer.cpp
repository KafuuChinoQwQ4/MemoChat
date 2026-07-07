import memochat.gate.user_token_validator_algorithms;

namespace memochat::tests::gate::auth
{
bool ShouldValidateUserToken(int uid, bool token_empty)
{
    return memochat::gate::auth::modules::ShouldValidateUserToken(uid, token_empty);
}

bool ShouldAcceptStoredUserToken(bool redis_hit, bool stored_token_empty, bool token_matches)
{
    return memochat::gate::auth::modules::ShouldAcceptStoredUserToken(redis_hit, stored_token_empty, token_matches);
}

bool ShouldAcceptJwtAccessClaims(bool jwt_valid, int expected_uid, int claims_uid)
{
    return memochat::gate::auth::modules::ShouldAcceptJwtAccessClaims(jwt_valid, expected_uid, claims_uid);
}
} // namespace memochat::tests::gate::auth
