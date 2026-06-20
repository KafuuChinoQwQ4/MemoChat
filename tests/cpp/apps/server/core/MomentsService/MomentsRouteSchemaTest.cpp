#include "modules/moments/MomentsRouteModule.h"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace
{

void ExpectFields(const memochat::gate::routing::RouteBodySchema& schema, const std::vector<std::string>& names)
{
    ASSERT_EQ(schema.fields.size(), names.size());
    for (std::size_t i = 0; i < names.size(); ++i)
    {
        EXPECT_EQ(schema.fields[i].name, names[i]);
    }
}

const char* ExpectedMomentsRouteSchemaSnapshot()
{
    return "route: moments.publish\n"
           "method: POST\n"
           "path: /api/moments/publish\n"
           "request: MomentPublishRequestDto\n"
           "  - uid\n"
           "  - login_ticket\n"
           "  - visibility\n"
           "  - location\n"
           "  - items\n"
           "response: MomentIdResponseDto\n"
           "  - error\n"
           "  - moment_id\n"
           "\n"
           "route: moments.list\n"
           "method: POST\n"
           "path: /api/moments/list\n"
           "request: MomentListRequestDto\n"
           "  - uid\n"
           "  - login_ticket\n"
           "  - last_moment_id\n"
           "  - limit\n"
           "  - author_uid\n"
           "response: MomentListResponseDto\n"
           "  - error\n"
           "  - has_more\n"
           "  - moments\n"
           "\n"
           "route: moments.detail\n"
           "method: POST\n"
           "path: /api/moments/detail\n"
           "request: MomentIdRequestDto\n"
           "  - uid\n"
           "  - login_ticket\n"
           "  - moment_id\n"
           "response: MomentDetailResponseDto\n"
           "  - error\n"
           "  - moment\n"
           "\n"
           "route: moments.delete\n"
           "method: POST\n"
           "path: /api/moments/delete\n"
           "request: MomentIdRequestDto\n"
           "  - uid\n"
           "  - login_ticket\n"
           "  - moment_id\n"
           "response: MomentIdResponseDto\n"
           "  - error\n"
           "  - moment_id\n"
           "\n"
           "route: moments.like\n"
           "method: POST\n"
           "path: /api/moments/like\n"
           "request: MomentLikeRequestDto\n"
           "  - uid\n"
           "  - login_ticket\n"
           "  - moment_id\n"
           "  - like\n"
           "response: MomentLikeResponseDto\n"
           "  - error\n"
           "  - moment_id\n"
           "  - has_liked\n"
           "  - like_count\n"
           "\n"
           "route: moments.comment\n"
           "method: POST\n"
           "path: /api/moments/comment\n"
           "request: MomentCommentRequestDto\n"
           "  - uid\n"
           "  - login_ticket\n"
           "  - moment_id\n"
           "  - content\n"
           "  - reply_uid\n"
           "  - comment_id\n"
           "  - delete\n"
           "  - delete_mode\n"
           "response: MomentCommentMutationResponseDto\n"
           "  - error\n"
           "  - moment_id\n"
           "  - delete\n"
           "  - comment_count\n"
           "\n"
           "route: moments.comment.list\n"
           "method: POST\n"
           "path: /api/moments/comment/list\n"
           "request: MomentCommentListRequestDto\n"
           "  - uid\n"
           "  - login_ticket\n"
           "  - moment_id\n"
           "  - last_comment_id\n"
           "  - limit\n"
           "response: MomentCommentListResponseDto\n"
           "  - error\n"
           "  - moment_id\n"
           "  - has_more\n"
           "  - comment_count\n"
           "  - comments\n"
           "\n"
           "route: moments.comment.like\n"
           "method: POST\n"
           "path: /api/moments/comment/like\n"
           "request: MomentCommentLikeRequestDto\n"
           "  - uid\n"
           "  - login_ticket\n"
           "  - comment_id\n"
           "  - like\n"
           "response: MomentCommentLikeResponseDto\n"
           "  - error\n"
           "  - comment_id\n"
           "  - has_liked\n"
           "  - like_count\n"
           "  - like_names\n"
           "  - likes\n"
           "\n";
}

} // namespace

TEST(MomentsRouteSchemaTest, ListsStableMomentsRoutesWithWireNameOverrides)
{
    const auto schemas = memochat::gate::modules::moments::MomentsRouteModule::RouteSchemas();

    ASSERT_EQ(schemas.size(), 8U);
    EXPECT_EQ(schemas[0].name, "moments.publish");
    EXPECT_EQ(schemas[0].method, "POST");
    EXPECT_EQ(schemas[0].path, "/api/moments/publish");
    EXPECT_EQ(schemas[0].request.type_name, "MomentPublishRequestDto");
    EXPECT_EQ(schemas[0].response.type_name, "MomentIdResponseDto");

    EXPECT_EQ(schemas[1].name, "moments.list");
    EXPECT_EQ(schemas[1].method, "POST");
    EXPECT_EQ(schemas[1].path, "/api/moments/list");
    EXPECT_EQ(schemas[1].request.type_name, "MomentListRequestDto");
    EXPECT_EQ(schemas[1].response.type_name, "MomentListResponseDto");

    EXPECT_EQ(schemas[2].name, "moments.detail");
    EXPECT_EQ(schemas[2].method, "POST");
    EXPECT_EQ(schemas[2].path, "/api/moments/detail");
    EXPECT_EQ(schemas[2].request.type_name, "MomentIdRequestDto");
    EXPECT_EQ(schemas[2].response.type_name, "MomentDetailResponseDto");

    EXPECT_EQ(schemas[3].name, "moments.delete");
    EXPECT_EQ(schemas[3].method, "POST");
    EXPECT_EQ(schemas[3].path, "/api/moments/delete");
    EXPECT_EQ(schemas[3].request.type_name, "MomentIdRequestDto");
    EXPECT_EQ(schemas[3].response.type_name, "MomentIdResponseDto");

    EXPECT_EQ(schemas[4].name, "moments.like");
    EXPECT_EQ(schemas[4].method, "POST");
    EXPECT_EQ(schemas[4].path, "/api/moments/like");
    EXPECT_EQ(schemas[4].request.type_name, "MomentLikeRequestDto");
    EXPECT_EQ(schemas[4].response.type_name, "MomentLikeResponseDto");

    EXPECT_EQ(schemas[5].name, "moments.comment");
    EXPECT_EQ(schemas[5].method, "POST");
    EXPECT_EQ(schemas[5].path, "/api/moments/comment");
    EXPECT_EQ(schemas[5].request.type_name, "MomentCommentRequestDto");
    EXPECT_EQ(schemas[5].response.type_name, "MomentCommentMutationResponseDto");

    EXPECT_EQ(schemas[6].name, "moments.comment.list");
    EXPECT_EQ(schemas[6].method, "POST");
    EXPECT_EQ(schemas[6].path, "/api/moments/comment/list");
    EXPECT_EQ(schemas[6].request.type_name, "MomentCommentListRequestDto");
    EXPECT_EQ(schemas[6].response.type_name, "MomentCommentListResponseDto");

    EXPECT_EQ(schemas[7].name, "moments.comment.like");
    EXPECT_EQ(schemas[7].method, "POST");
    EXPECT_EQ(schemas[7].path, "/api/moments/comment/like");
    EXPECT_EQ(schemas[7].request.type_name, "MomentCommentLikeRequestDto");
    EXPECT_EQ(schemas[7].response.type_name, "MomentCommentLikeResponseDto");
}

TEST(MomentsRouteSchemaTest, BuildsFieldInventoriesFromMomentsDtos)
{
    const auto schemas = memochat::gate::modules::moments::MomentsRouteModule::RouteSchemas();
    ASSERT_EQ(schemas.size(), 8U);

    ExpectFields(schemas[0].request, {"uid", "login_ticket", "visibility", "location", "items"});
    ExpectFields(schemas[0].response, {"error", "moment_id"});

    ExpectFields(schemas[1].request, {"uid", "login_ticket", "last_moment_id", "limit", "author_uid"});
    ExpectFields(schemas[1].response, {"error", "has_more", "moments"});

    ExpectFields(schemas[2].request, {"uid", "login_ticket", "moment_id"});
    ExpectFields(schemas[2].response, {"error", "moment"});

    ExpectFields(schemas[3].request, {"uid", "login_ticket", "moment_id"});
    ExpectFields(schemas[3].response, {"error", "moment_id"});

    ExpectFields(schemas[4].request, {"uid", "login_ticket", "moment_id", "like"});
    ExpectFields(schemas[4].response, {"error", "moment_id", "has_liked", "like_count"});

    ExpectFields(schemas[5].request,
                 {"uid", "login_ticket", "moment_id", "content", "reply_uid", "comment_id", "delete", "delete_mode"});
    ExpectFields(schemas[5].response, {"error", "moment_id", "delete", "comment_count"});

    ExpectFields(schemas[6].request, {"uid", "login_ticket", "moment_id", "last_comment_id", "limit"});
    ExpectFields(schemas[6].response, {"error", "moment_id", "has_more", "comment_count", "comments"});

    ExpectFields(schemas[7].request, {"uid", "login_ticket", "comment_id", "like"});
    ExpectFields(schemas[7].response, {"error", "comment_id", "has_liked", "like_count", "like_names", "likes"});
}

TEST(MomentsRouteSchemaTest, RendersDeterministicSnapshotForReview)
{
    const auto schemas = memochat::gate::modules::moments::MomentsRouteModule::RouteSchemas();
    const std::string snapshot = memochat::gate::routing::RenderRouteSchemaSnapshot(schemas);

    EXPECT_EQ(snapshot, ExpectedMomentsRouteSchemaSnapshot());
}
