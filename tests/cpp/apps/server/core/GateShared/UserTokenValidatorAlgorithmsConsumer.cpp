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
} // namespace memochat::tests::gate::auth
