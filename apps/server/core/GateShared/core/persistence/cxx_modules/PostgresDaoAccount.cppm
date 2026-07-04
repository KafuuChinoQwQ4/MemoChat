export module memochat.gate.postgres_dao_account_algorithms;

export namespace memochat::gate::postgres_dao_account::modules
{
const char* DefaultSchema()
{
    return "public";
}

const char* DefaultUserIcon()
{
    return ":/res/head_1.jpg";
}

int UserPublicIdMaxAttempts()
{
    return 20;
}

bool ShouldAcceptGeneratedPublicId(bool rows_empty)
{
    return rows_empty;
}

bool HasGeneratedUserPublicId(bool public_id_empty)
{
    return !public_id_empty;
}

bool HasPositiveUid(int uid)
{
    return uid > 0;
}

bool IsAffectedRowsPositive(unsigned long long affected_rows)
{
    return affected_rows > 0;
}

bool ShouldUseAccountBridge(bool account_connection_empty)
{
    return !account_connection_empty;
}

} // namespace memochat::gate::postgres_dao_account::modules
