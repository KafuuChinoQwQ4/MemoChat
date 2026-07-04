export module memochat.runtime.etcd_config_algorithms;

export namespace memochat::runtime::modules
{
bool HasMultipleEndpoints(bool comma_found)
{
    return comma_found;
}

unsigned long long SchemeDelimiterSize()
{
    return 3;
}

bool HasSchemeDelimiter(bool delimiter_found)
{
    return delimiter_found;
}

bool HasExplicitPort(bool colon_found)
{
    return colon_found;
}

const char* DefaultEtcdPort()
{
    return "2379";
}

bool IsResponseWithKvs(bool kvs_token_found)
{
    return kvs_token_found;
}

bool IsResponseWithHeader(bool header_token_found)
{
    return header_token_found;
}

bool ShouldAttachLease(bool positive_ttl)
{
    return positive_ttl;
}

bool ShouldStartWatchThread(bool watch_thread_joinable)
{
    return !watch_thread_joinable;
}

bool ShouldSkipConfigWatchStart(bool already_watching)
{
    return already_watching;
}

bool ShouldAcceptConfigChangeKey(bool section_empty, bool key_empty)
{
    return !section_empty && !key_empty;
}

bool HasConfigPrefix(bool key_starts_with_prefix)
{
    return key_starts_with_prefix;
}

bool HasConfigSectionSeparator(bool slash_found)
{
    return slash_found;
}

bool ShouldCreateEtcdConfig(bool endpoints_empty)
{
    return !endpoints_empty;
}
} // namespace memochat::runtime::modules
