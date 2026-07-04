import memochat.gate.redis_pipeline_algorithms;

namespace memochat::tests::gate::redis_pipeline
{
const char* LoginProfilePrefix()
{
    return memochat::gate::redis_pipeline::modules::LoginProfilePrefix();
}

const char* LoginProfileUidPrefix()
{
    return memochat::gate::redis_pipeline::modules::LoginProfileUidPrefix();
}

const char* UserTokenPrefix()
{
    return memochat::gate::redis_pipeline::modules::UserTokenPrefix();
}
} // namespace memochat::tests::gate::redis_pipeline
