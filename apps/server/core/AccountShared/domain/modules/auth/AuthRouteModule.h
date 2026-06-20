#pragma once

#include "routing/RouteModule.h"
#include "routing/RouteSchema.h"

#include <vector>

namespace memochat::gate::modules::auth
{

class AuthRouteModule final : public memochat::gate::routing::RouteModule
{
public:
    // Registers every auth route (register + login). Used by the GateServer
    // monolith (Full profile).
    void RegisterRoutes(memochat::gate::routing::RouteRegistry& registry) override;

    // Granular registration for the fine account split (gateserver split
    // Phase 5): RegisterService owns account creation/recovery, LoginService
    // owns authentication. Both reach account data only via account-core
    // (AccountPersistence), per D-ACCOUNT.
    static void RegisterRegisterRoutes(
        memochat::gate::routing::RouteRegistry& registry); // /get_varifycode,/user_register,/reset_pwd
    static void RegisterLoginRoutes(memochat::gate::routing::RouteRegistry& registry); // /user_login

    static std::vector<memochat::gate::routing::RouteSchemaDescriptor> RouteSchemas();
};

} // namespace memochat::gate::modules::auth
