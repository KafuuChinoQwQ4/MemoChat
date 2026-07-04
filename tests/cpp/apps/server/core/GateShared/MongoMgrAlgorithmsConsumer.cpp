import memochat.gate.mongo_mgr_algorithms;

namespace memochat::tests::gate::mongo_mgr
{
unsigned MomentContentForwardCount()
{
    return memochat::gate::mongo_mgr::modules::MomentContentForwardCount();
}

unsigned ForwardingSurfaceCount()
{
    return memochat::gate::mongo_mgr::modules::ForwardingSurfaceCount();
}

bool IsCompleteForwardingSurface(unsigned moment_content_count)
{
    return memochat::gate::mongo_mgr::modules::IsCompleteForwardingSurface(moment_content_count);
}
} // namespace memochat::tests::gate::mongo_mgr
