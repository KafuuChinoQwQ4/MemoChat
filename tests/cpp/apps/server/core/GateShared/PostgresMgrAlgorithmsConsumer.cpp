import memochat.gate.postgres_mgr_algorithms;

namespace memochat::tests::gate::postgres_mgr
{
unsigned AccountForwardCount()
{
    return memochat::gate::postgres_mgr::modules::AccountForwardCount();
}

unsigned CallForwardCount()
{
    return memochat::gate::postgres_mgr::modules::CallForwardCount();
}

unsigned MediaForwardCount()
{
    return memochat::gate::postgres_mgr::modules::MediaForwardCount();
}

unsigned MomentForwardCount()
{
    return memochat::gate::postgres_mgr::modules::MomentForwardCount();
}

unsigned ForwardingSurfaceCount()
{
    return memochat::gate::postgres_mgr::modules::ForwardingSurfaceCount();
}

bool IsCompleteForwardingSurface(unsigned account_count,
                                 unsigned call_count,
                                 unsigned media_count,
                                 unsigned moment_count)
{
    return memochat::gate::postgres_mgr::modules::IsCompleteForwardingSurface(account_count,
                                                                              call_count,
                                                                              media_count,
                                                                              moment_count);
}
} // namespace memochat::tests::gate::postgres_mgr
