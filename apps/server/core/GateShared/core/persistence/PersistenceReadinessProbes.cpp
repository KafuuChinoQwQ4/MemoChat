#include "PersistenceReadinessProbes.hpp"

#include "MongoMgr.hpp"
#include "PostgresMgr.hpp"

// The probe factories reference the concrete manager singletons here — inside the
// GateInfraPersistence slice — so the shared GateRuntimeCore bootstrap never does.
// A service opts into a dependency by linking this slice and adding the probe to
// the vector it passes to RunGateDomainServer.
namespace memochat::gate::persistence
{

GateReadinessProbe PostgresReadinessProbe()
{
    return GateReadinessProbe{
        .name = "Postgres",
        .check = [](std::string* error) -> bool
        {
            const auto postgres = PostgresMgr::GetInstance();
            if (postgres->Ready())
            {
                return true;
            }
            if (error != nullptr)
            {
                *error = postgres->StartupError();
            }
            return false;
        },
    };
}

GateReadinessProbe MongoReadinessProbe()
{
    return GateReadinessProbe{
        .name = "Mongo",
        .check = [](std::string* error) -> bool
        {
            const auto mongo = MongoMgr::GetInstance();
            if (mongo->Ready())
            {
                return true;
            }
            if (error != nullptr)
            {
                *error = mongo->StartupError();
            }
            return false;
        },
    };
}

} // namespace memochat::gate::persistence
