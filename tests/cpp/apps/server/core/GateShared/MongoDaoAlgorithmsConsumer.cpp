import memochat.gate.mongo_dao_algorithms;

namespace memochat::tests::gate::mongo_dao
{
bool ParseBoolText(const char* text)
{
    return memochat::gate::mongo_dao::modules::ParseBoolText(text);
}

const char* DefaultMomentsCollection()
{
    return memochat::gate::mongo_dao::modules::DefaultMomentsCollection();
}

bool IsEnabled(bool enabled, bool init_ok, bool has_pool)
{
    return memochat::gate::mongo_dao::modules::IsEnabled(enabled, init_ok, has_pool);
}

bool ShouldInitializeMongo(bool enabled)
{
    return memochat::gate::mongo_dao::modules::ShouldInitializeMongo(enabled);
}

bool HasRequiredConfig(bool uri_empty, bool database_empty)
{
    return memochat::gate::mongo_dao::modules::HasRequiredConfig(uri_empty, database_empty);
}

bool ShouldUseDefaultMomentsCollection(bool collection_empty)
{
    return memochat::gate::mongo_dao::modules::ShouldUseDefaultMomentsCollection(collection_empty);
}

bool CanEnsureIndexes(bool has_pool)
{
    return memochat::gate::mongo_dao::modules::CanEnsureIndexes(has_pool);
}

bool HasPositiveMomentId(long long moment_id)
{
    return memochat::gate::mongo_dao::modules::HasPositiveMomentId(moment_id);
}

bool CanAccessMomentContent(bool enabled, long long moment_id)
{
    return memochat::gate::mongo_dao::modules::CanAccessMomentContent(enabled, moment_id);
}
} // namespace memochat::tests::gate::mongo_dao
