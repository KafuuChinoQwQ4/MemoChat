export module memochat.ai.repository_algorithms;

export namespace memochat::ai::repository::modules
{
const char* DefaultPostgresSchema()
{
    return "public";
}

bool ShouldUseDefaultSchema(bool schema_empty)
{
    return schema_empty;
}

const char* DefaultSessionTitle()
{
    return "";
}

bool HasAffectedRows(unsigned long long affected_rows)
{
    return affected_rows > 0;
}

bool ShouldReturnNoSession(bool rows_empty)
{
    return rows_empty;
}
} // namespace memochat::ai::repository::modules
