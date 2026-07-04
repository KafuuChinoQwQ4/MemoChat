import memochat.ai.repository_algorithms;

namespace memochat::tests::ai::repository
{
const char* DefaultPostgresSchema()
{
    return memochat::ai::repository::modules::DefaultPostgresSchema();
}

bool ShouldUseDefaultSchema(bool schema_empty)
{
    return memochat::ai::repository::modules::ShouldUseDefaultSchema(schema_empty);
}

const char* DefaultSessionTitle()
{
    return memochat::ai::repository::modules::DefaultSessionTitle();
}

bool HasAffectedRows(unsigned long long affected_rows)
{
    return memochat::ai::repository::modules::HasAffectedRows(affected_rows);
}

bool ShouldReturnNoSession(bool rows_empty)
{
    return memochat::ai::repository::modules::ShouldReturnNoSession(rows_empty);
}
} // namespace memochat::tests::ai::repository
