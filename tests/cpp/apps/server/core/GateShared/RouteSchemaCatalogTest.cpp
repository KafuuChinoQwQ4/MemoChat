#include "routing/RouteSchema.hpp"
#include "routing/RouteSchemaCatalog.hpp"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace
{

struct MediaRequestDto
{
    int uid = 0;
    std::string media_id;
};

struct MediaResponseDto
{
    int error = 0;
    std::string media_url;
};

struct AuthRequestDto
{
    std::string token;
};

struct AuthResponseDto
{
    int error = 0;
};

} // namespace

TEST(RouteSchemaCatalogTest, CatalogPreservesModuleAndRouteOrder)
{
    using memochat::gate::routing::MakeRouteSchema;
    using memochat::gate::routing::RouteSchemaCatalog;

    RouteSchemaCatalog catalog;
    catalog.Add("media",
                {MakeRouteSchema<MediaRequestDto, MediaResponseDto>("POST",
                                                                    "/media/status",
                                                                    "media.status",
                                                                    "MediaRequestDto",
                                                                    "MediaResponseDto"),
                 MakeRouteSchema<MediaRequestDto, MediaResponseDto>("POST",
                                                                    "/media/upload",
                                                                    "media.upload",
                                                                    "MediaRequestDto",
                                                                    "MediaResponseDto")});
    catalog.Add("auth",
                {MakeRouteSchema<AuthRequestDto, AuthResponseDto>("POST",
                                                                  "/auth/login",
                                                                  "auth.login",
                                                                  "AuthRequestDto",
                                                                  "AuthResponseDto")});

    ASSERT_EQ(catalog.Modules().size(), 2U);
    EXPECT_EQ(catalog.Modules()[0].module, "media");
    EXPECT_EQ(catalog.Modules()[1].module, "auth");

    const auto all = catalog.AllRoutes();
    ASSERT_EQ(all.size(), 3U);
    EXPECT_EQ(all[0].name, "media.status");
    EXPECT_EQ(all[1].name, "media.upload");
    EXPECT_EQ(all[2].name, "auth.login");
}

TEST(RouteSchemaCatalogTest, RendersDeterministicCatalogSnapshot)
{
    using memochat::gate::routing::MakeRouteSchema;
    using memochat::gate::routing::MakeRouteSchemaWithoutBody;
    using memochat::gate::routing::RenderRouteSchemaCatalogSnapshot;
    using memochat::gate::routing::RouteSchemaCatalog;

    RouteSchemaCatalog catalog;
    catalog.Add("media",
                {MakeRouteSchema<MediaRequestDto, MediaResponseDto>("POST",
                                                                    "/media/status",
                                                                    "media.status",
                                                                    "MediaRequestDto",
                                                                    "MediaResponseDto")});
    catalog.Add("auth", {MakeRouteSchemaWithoutBody("GET", "/auth/health", "auth.health")});

    const std::string snapshot = RenderRouteSchemaCatalogSnapshot(catalog);

    EXPECT_EQ(snapshot,
              "== module: media ==\n"
              "route: media.status\n"
              "method: POST\n"
              "path: /media/status\n"
              "request: MediaRequestDto\n"
              "  - uid\n"
              "  - media_id\n"
              "response: MediaResponseDto\n"
              "  - error\n"
              "  - media_url\n"
              "\n"
              "== module: auth ==\n"
              "route: auth.health\n"
              "method: GET\n"
              "path: /auth/health\n"
              "request: <none>\n"
              "response: <none>\n"
              "\n");
}

TEST(RouteSchemaCatalogTest, RendersDeterministicCatalogJsonWithEscaping)
{
    using memochat::gate::routing::MakeRouteSchema;
    using memochat::gate::routing::RenderRouteSchemaCatalogJson;
    using memochat::gate::routing::RouteSchemaCatalog;

    RouteSchemaCatalog catalog;
    // The request type_name carries a backslash and a double-quote to exercise JSON escaping.
    catalog.Add("media",
                {MakeRouteSchema<MediaRequestDto, MediaResponseDto>("POST",
                                                                    "/media/status",
                                                                    "media.status",
                                                                    "Weird\\\"Name",
                                                                    "MediaResponseDto")});

    const std::string json = RenderRouteSchemaCatalogJson(catalog);

    EXPECT_EQ(json,
              "{\"modules\":[{\"module\":\"media\",\"routes\":["
              "{\"name\":\"media.status\",\"method\":\"POST\",\"path\":\"/media/status\","
              "\"request\":{\"type\":\"Weird\\\\\\\"Name\",\"fields\":[\"uid\",\"media_id\"]},"
              "\"response\":{\"type\":\"MediaResponseDto\",\"fields\":[\"error\",\"media_url\"]}}"
              "]}]}");
}

TEST(RouteSchemaCatalogTest, FindsDuplicateRoutePathsAcrossModules)
{
    using memochat::gate::routing::FindDuplicateRoutePaths;
    using memochat::gate::routing::MakeRouteSchema;
    using memochat::gate::routing::RouteSchemaCatalog;

    RouteSchemaCatalog catalog;
    catalog.Add("media",
                {MakeRouteSchema<MediaRequestDto,
                                 MediaResponseDto>("POST", "/x", "media.x", "MediaRequestDto", "MediaResponseDto")});
    catalog.Add("auth",
                {MakeRouteSchema<AuthRequestDto,
                                 AuthResponseDto>("POST", "/x", "auth.x", "AuthRequestDto", "AuthResponseDto")});

    const auto duplicates = FindDuplicateRoutePaths(catalog);
    ASSERT_EQ(duplicates.size(), 1U);
    EXPECT_EQ(duplicates[0], "POST /x");

    RouteSchemaCatalog unique;
    unique.Add("media",
               {MakeRouteSchema<MediaRequestDto, MediaResponseDto>("POST",
                                                                   "/media/a",
                                                                   "media.a",
                                                                   "MediaRequestDto",
                                                                   "MediaResponseDto")});
    unique.Add("auth",
               {MakeRouteSchema<AuthRequestDto,
                                AuthResponseDto>("POST", "/auth/b", "auth.b", "AuthRequestDto", "AuthResponseDto")});

    EXPECT_TRUE(FindDuplicateRoutePaths(unique).empty());
}
