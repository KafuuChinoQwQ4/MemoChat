import memochat.logging.redaction_algorithms;

namespace memochat::tests::logging
{
int ClassifySensitiveKey(const char* key, unsigned long long size)
{
    return memochat::logging::redaction_modules::ClassifyLowerSensitiveKey(key, size);
}

bool ShouldCollapseTokenMask(unsigned long long size)
{
    return memochat::logging::redaction_modules::ShouldCollapseTokenMask(size);
}

unsigned long long VisibleEmailLocalPrefix(unsigned long long local_size)
{
    return memochat::logging::redaction_modules::VisibleEmailLocalPrefix(local_size);
}
} // namespace memochat::tests::logging
