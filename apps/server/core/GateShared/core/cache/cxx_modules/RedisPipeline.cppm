export module memochat.gate.redis_pipeline_algorithms;

export namespace memochat::gate::redis_pipeline::modules
{
const char* LoginProfilePrefix()
{
    return "ulogin_profile_";
}

const char* LoginProfileUidPrefix()
{
    return "ulogin_profile_uid_";
}

const char* UserTokenPrefix()
{
    return "utoken_";
}
} // namespace memochat::gate::redis_pipeline::modules
