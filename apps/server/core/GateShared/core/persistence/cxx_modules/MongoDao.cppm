export module memochat.gate.mongo_dao_algorithms;

export namespace memochat::gate::mongo_dao::modules
{
char ToLowerAscii(char ch)
{
    return ch >= 'A' && ch <= 'Z' ? static_cast<char>(ch - 'A' + 'a') : ch;
}

bool EqualsIgnoreCaseAscii(const char* text, const char* expected)
{
    if (text == nullptr || expected == nullptr)
    {
        return false;
    }
    while (*text != '\0' && *expected != '\0')
    {
        if (ToLowerAscii(*text) != *expected)
        {
            return false;
        }
        ++text;
        ++expected;
    }
    return *text == '\0' && *expected == '\0';
}

bool ParseBoolText(const char* text)
{
    return EqualsIgnoreCaseAscii(text, "1") || EqualsIgnoreCaseAscii(text, "true") ||
           EqualsIgnoreCaseAscii(text, "yes") || EqualsIgnoreCaseAscii(text, "on");
}

const char* DefaultMomentsCollection()
{
    return "moments_content";
}

bool IsEnabled(bool enabled, bool init_ok, bool has_pool)
{
    return enabled && init_ok && has_pool;
}

bool ShouldInitializeMongo(bool enabled)
{
    return enabled;
}

bool HasRequiredConfig(bool uri_empty, bool database_empty)
{
    return !uri_empty && !database_empty;
}

bool ShouldUseDefaultMomentsCollection(bool collection_empty)
{
    return collection_empty;
}

bool CanEnsureIndexes(bool has_pool)
{
    return has_pool;
}

bool HasPositiveMomentId(long long moment_id)
{
    return moment_id > 0;
}

bool CanAccessMomentContent(bool enabled, long long moment_id)
{
    return enabled && HasPositiveMomentId(moment_id);
}
} // namespace memochat::gate::mongo_dao::modules
