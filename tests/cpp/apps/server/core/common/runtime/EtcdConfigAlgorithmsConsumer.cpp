import memochat.runtime.etcd_config_algorithms;

namespace memochat::tests::runtime
{
bool HasMultipleEtcdEndpoints(bool comma_found)
{
    return memochat::runtime::modules::HasMultipleEndpoints(comma_found);
}

unsigned long long EtcdSchemeDelimiterSize()
{
    return memochat::runtime::modules::SchemeDelimiterSize();
}

bool HasEtcdSchemeDelimiter(bool delimiter_found)
{
    return memochat::runtime::modules::HasSchemeDelimiter(delimiter_found);
}

bool HasExplicitEtcdPort(bool colon_found)
{
    return memochat::runtime::modules::HasExplicitPort(colon_found);
}

const char* DefaultEtcdPort()
{
    return memochat::runtime::modules::DefaultEtcdPort();
}

bool IsEtcdResponseWithKvs(bool kvs_token_found)
{
    return memochat::runtime::modules::IsResponseWithKvs(kvs_token_found);
}

bool IsEtcdResponseWithHeader(bool header_token_found)
{
    return memochat::runtime::modules::IsResponseWithHeader(header_token_found);
}

bool ShouldAttachEtcdLease(bool positive_ttl)
{
    return memochat::runtime::modules::ShouldAttachLease(positive_ttl);
}

bool ShouldStartEtcdWatchThread(bool watch_thread_joinable)
{
    return memochat::runtime::modules::ShouldStartWatchThread(watch_thread_joinable);
}

bool ShouldSkipEtcdConfigWatchStart(bool already_watching)
{
    return memochat::runtime::modules::ShouldSkipConfigWatchStart(already_watching);
}

bool ShouldAcceptEtcdConfigChangeKey(bool section_empty, bool key_empty)
{
    return memochat::runtime::modules::ShouldAcceptConfigChangeKey(section_empty, key_empty);
}

bool HasEtcdConfigPrefix(bool key_starts_with_prefix)
{
    return memochat::runtime::modules::HasConfigPrefix(key_starts_with_prefix);
}

bool HasEtcdConfigSectionSeparator(bool slash_found)
{
    return memochat::runtime::modules::HasConfigSectionSeparator(slash_found);
}

bool ShouldCreateEtcdConfig(bool endpoints_empty)
{
    return memochat::runtime::modules::ShouldCreateEtcdConfig(endpoints_empty);
}
} // namespace memochat::tests::runtime
