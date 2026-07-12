#include "MongoMgr.hpp"

import memochat.gate.mongo_mgr_algorithms;

namespace
{
namespace mongo_mgr_modules = memochat::gate::mongo_mgr::modules;

// Lock the pure-forwarding facade surface: every MongoMgr method forwards
// directly to the single shared MongoDao member, so the moment-content method
// count and total must match the module contract.
static_assert(mongo_mgr_modules::IsCompleteForwardingSurface(mongo_mgr_modules::MomentContentForwardCount()));
static_assert(mongo_mgr_modules::ForwardingSurfaceCount() == 3u);
} // namespace

MongoMgr::~MongoMgr()
{
}

MongoMgr::MongoMgr()
{
}

bool MongoMgr::Ready() const noexcept
{
    return _dao.Ready();
}

const std::string& MongoMgr::StartupError() const noexcept
{
    return _dao.StartupError();
}

bool MongoMgr::InsertMomentContent(const MomentContentInfo& content)
{
    return _dao.InsertMomentContent(content);
}

bool MongoMgr::GetMomentContent(int64_t moment_id, MomentContentInfo& content)
{
    return _dao.GetMomentContent(moment_id, content);
}

bool MongoMgr::DeleteMomentContent(int64_t moment_id)
{
    return _dao.DeleteMomentContent(moment_id);
}
