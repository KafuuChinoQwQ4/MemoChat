#include "routing/RouteSchemaCatalog.h"

#include "modules/ai/AIRouteModule.h"
#include "modules/auth/AuthRouteModule.h"
#include "modules/call/CallRouteModule.h"
#include "modules/media/MediaRouteModule.h"
#include "modules/moments/MomentsRouteModule.h"
#include "modules/profile/ProfileRouteModule.h"
#include "modules/r18/R18RouteModule.h"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace
{

using memochat::gate::routing::FindDuplicateRoutePaths;
using memochat::gate::routing::RouteSchemaCatalog;

// Build a catalog from every real route module's RouteSchemas(). This is the
// cross-module integration the BL synthetic-only catalog test could not exercise:
// it links the actual *RouteSchemas.cpp for each module and aggregates their
// real descriptors.
RouteSchemaCatalog BuildRealCatalog()
{
    RouteSchemaCatalog catalog;
    catalog.Add("auth", memochat::gate::modules::auth::AuthRouteModule::RouteSchemas());
    catalog.Add("profile", memochat::gate::modules::profile::ProfileRouteModule::RouteSchemas());
    catalog.Add("media", memochat::gate::modules::media::MediaRouteModule::RouteSchemas());
    catalog.Add("call", memochat::gate::modules::call::CallRouteModule::RouteSchemas());
    catalog.Add("r18", memochat::gate::modules::r18::R18RouteModule::RouteSchemas());
    catalog.Add("ai", memochat::gate::modules::ai::AIRouteModule::RouteSchemas());
    catalog.Add("moments", memochat::gate::modules::moments::MomentsRouteModule::RouteSchemas());
    return catalog;
}

std::string JoinDuplicates(const std::vector<std::string>& keys)
{
    std::string joined;
    for (const auto& key : keys)
    {
        if (!joined.empty())
        {
            joined += ", ";
        }
        joined += key;
    }
    return joined;
}

} // namespace

// Guards that the catalog actually aggregates real descriptors, so the
// no-duplicate assertion below cannot silently pass on an empty catalog.
TEST(CrossModuleRouteSchemaCatalogTest, BuildsCatalogFromAllRealModules)
{
    const RouteSchemaCatalog catalog = BuildRealCatalog();

    EXPECT_EQ(catalog.Modules().size(), 7u);
    EXPECT_FALSE(catalog.AllRoutes().empty())
        << "expected the seven route modules to contribute at least one schema descriptor";
}

// The real guard: no two route modules may register the same method+path.
TEST(CrossModuleRouteSchemaCatalogTest, HasNoCrossModuleDuplicateRoutePaths)
{
    const RouteSchemaCatalog catalog = BuildRealCatalog();

    const std::vector<std::string> duplicates = FindDuplicateRoutePaths(catalog);

    EXPECT_TRUE(duplicates.empty())
        << "cross-module route collisions: " << JoinDuplicates(duplicates);
}

// Proves the guard actually fires: injecting a synthetic module that re-registers
// an existing real method+path must surface exactly that one colliding key.
TEST(CrossModuleRouteSchemaCatalogTest, DetectsAnInjectedDuplicate)
{
    RouteSchemaCatalog catalog = BuildRealCatalog();

    const std::vector<memochat::gate::routing::RouteSchemaDescriptor> all = catalog.AllRoutes();
    ASSERT_FALSE(all.empty());
    const memochat::gate::routing::RouteSchemaDescriptor& existing = all.front();

    memochat::gate::routing::RouteSchemaDescriptor collider;
    collider.method = existing.method;
    collider.path = existing.path;
    collider.name = "synthetic.collision";
    catalog.Add("synthetic_collision_module", {collider});

    const std::vector<std::string> duplicates = FindDuplicateRoutePaths(catalog);
    const std::string expected_key = existing.method + " " + existing.path;

    ASSERT_EQ(duplicates.size(), 1u) << JoinDuplicates(duplicates);
    EXPECT_EQ(duplicates.front(), expected_key);
}
