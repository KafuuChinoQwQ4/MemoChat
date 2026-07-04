export module memochat.gate.mongo_mgr_algorithms;

// GateInfraCore shared Mongo manager facade surface contract.
// MongoMgr is a pure forwarding facade: every public method forwards directly
// to the single shared MongoDao member for Moments content. This module locks
// the facade forwarding surface shape (moment content method count) at compile
// time. It exports a primitive count only; it does NOT export MongoDao,
// mongocxx/bsoncxx types, MomentContentInfo DTOs, BSON construction, pool
// lifecycle, std::string, vectors, or MongoDB I/O.
export namespace memochat::gate::mongo_mgr::modules
{
constexpr unsigned MomentContentForwardCount()
{
    // InsertMomentContent, GetMomentContent, DeleteMomentContent
    return 3;
}

constexpr unsigned ForwardingSurfaceCount()
{
    return MomentContentForwardCount();
}

constexpr bool IsCompleteForwardingSurface(unsigned moment_content_count)
{
    return moment_content_count == MomentContentForwardCount();
}
} // namespace memochat::gate::mongo_mgr::modules
