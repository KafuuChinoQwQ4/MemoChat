#pragma once

#include <string>
#include <vector>

// Minimal relation-service client for MomentsService. After the Phase 2 DB
// split the moments database (memo_moments) no longer contains the friend
// tables, so moments visibility filtering cannot run as a local SQL query.
// This client calls the relation-query gRPC service (ChatRelationInternalService
// on port 50090, which owns memo_pg) to resolve which authors are friends of a
// viewer. It deliberately wraps only the one RPC moments needs.
namespace memochat::gate::services::moments
{

class MomentsRelationClient
{
public:
    // endpoint e.g. "127.0.0.1:50090". A lazily-created channel/stub is built on
    // first use so construction never blocks startup.
    explicit MomentsRelationClient(std::string endpoint);

    // Returns the subset of author_uids that are visible to viewer_uid under the
    // friends-only rule (bidirectional friend OR accepted friend_apply). On any
    // transport/parse error returns an empty vector (fail-closed: friends-only
    // posts from unverifiable authors are hidden rather than leaked).
    std::vector<int> FilterFriendUids(int viewer_uid, const std::vector<int>& author_uids);

    bool Enabled() const { return !_endpoint.empty(); }

private:
    std::string _endpoint;
};

} // namespace memochat::gate::services::moments
