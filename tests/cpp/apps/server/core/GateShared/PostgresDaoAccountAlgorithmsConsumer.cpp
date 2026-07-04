import memochat.gate.postgres_dao_account_algorithms;

namespace memochat::tests::gate::postgres_dao_account
{
const char* DefaultSchema()
{
    return memochat::gate::postgres_dao_account::modules::DefaultSchema();
}

const char* DefaultUserIcon()
{
    return memochat::gate::postgres_dao_account::modules::DefaultUserIcon();
}

int UserPublicIdMaxAttempts()
{
    return memochat::gate::postgres_dao_account::modules::UserPublicIdMaxAttempts();
}

bool ShouldAcceptGeneratedPublicId(bool rows_empty)
{
    return memochat::gate::postgres_dao_account::modules::ShouldAcceptGeneratedPublicId(rows_empty);
}

bool HasGeneratedUserPublicId(bool public_id_empty)
{
    return memochat::gate::postgres_dao_account::modules::HasGeneratedUserPublicId(public_id_empty);
}

bool HasPositiveUid(int uid)
{
    return memochat::gate::postgres_dao_account::modules::HasPositiveUid(uid);
}

bool IsAffectedRowsPositive(unsigned long long affected_rows)
{
    return memochat::gate::postgres_dao_account::modules::IsAffectedRowsPositive(affected_rows);
}

bool ShouldUseAccountBridge(bool account_connection_empty)
{
    return memochat::gate::postgres_dao_account::modules::ShouldUseAccountBridge(account_connection_empty);
}
} // namespace memochat::tests::gate::postgres_dao_account
