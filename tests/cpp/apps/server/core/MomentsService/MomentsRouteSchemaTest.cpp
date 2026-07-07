#include "modules/moments/MomentsRouteModule.hpp"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace memochat::tests::moments::route_schema
{
const char* PostMethod();
const char* PublishPath();
const char* PublishRouteName();
const char* PublishRequestTypeName();
const char* MomentIdResponseTypeName();
const char* ListPath();
const char* ListRouteName();
const char* ListRequestTypeName();
const char* ListResponseTypeName();
const char* DetailPath();
const char* DetailRouteName();
const char* MomentIdRequestTypeName();
const char* DetailResponseTypeName();
const char* DeletePath();
const char* DeleteRouteName();
const char* LikePath();
const char* LikeRouteName();
const char* LikeRequestTypeName();
const char* LikeResponseTypeName();
const char* CommentPath();
const char* CommentRouteName();
const char* CommentRequestTypeName();
const char* CommentMutationResponseTypeName();
const char* CommentListPath();
const char* CommentListRouteName();
const char* CommentListRequestTypeName();
const char* CommentListResponseTypeName();
const char* CommentLikePath();
const char* CommentLikeRouteName();
const char* CommentLikeRequestTypeName();
const char* CommentLikeResponseTypeName();
} // namespace memochat::tests::moments::route_schema

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
           "  - moment_id\n"
           "response: MomentDetailResponseDto\n"
           "  - error\n"
           "  - moment\n"
           "\n"
           "route: moments.delete\n"
           "method: POST\n"
           "path: /api/moments/delete\n"
           "request: MomentIdRequestDto\n"
           "  - moment_id\n"
           "response: MomentIdResponseDto\n"
           "  - error\n"
           "  - moment_id\n"
           "\n"
           "route: moments.like\n"
           "method: POST\n"
           "path: /api/moments/like\n"
           "request: MomentLikeRequestDto\n"
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
    EXPECT_EQ(schemas[0].name, memochat::tests::moments::route_schema::PublishRouteName());
    EXPECT_EQ(schemas[0].method, memochat::tests::moments::route_schema::PostMethod());
    EXPECT_EQ(schemas[0].path, memochat::tests::moments::route_schema::PublishPath());
    EXPECT_EQ(schemas[0].request.type_name, memochat::tests::moments::route_schema::PublishRequestTypeName());
    EXPECT_EQ(schemas[0].response.type_name, memochat::tests::moments::route_schema::MomentIdResponseTypeName());

    EXPECT_EQ(schemas[1].name, memochat::tests::moments::route_schema::ListRouteName());
    EXPECT_EQ(schemas[1].method, memochat::tests::moments::route_schema::PostMethod());
    EXPECT_EQ(schemas[1].path, memochat::tests::moments::route_schema::ListPath());
    EXPECT_EQ(schemas[1].request.type_name, memochat::tests::moments::route_schema::ListRequestTypeName());
    EXPECT_EQ(schemas[1].response.type_name, memochat::tests::moments::route_schema::ListResponseTypeName());

    EXPECT_EQ(schemas[2].name, memochat::tests::moments::route_schema::DetailRouteName());
    EXPECT_EQ(schemas[2].method, memochat::tests::moments::route_schema::PostMethod());
    EXPECT_EQ(schemas[2].path, memochat::tests::moments::route_schema::DetailPath());
    EXPECT_EQ(schemas[2].request.type_name, memochat::tests::moments::route_schema::MomentIdRequestTypeName());
    EXPECT_EQ(schemas[2].response.type_name, memochat::tests::moments::route_schema::DetailResponseTypeName());

    EXPECT_EQ(schemas[3].name, memochat::tests::moments::route_schema::DeleteRouteName());
    EXPECT_EQ(schemas[3].method, memochat::tests::moments::route_schema::PostMethod());
    EXPECT_EQ(schemas[3].path, memochat::tests::moments::route_schema::DeletePath());
    EXPECT_EQ(schemas[3].request.type_name, memochat::tests::moments::route_schema::MomentIdRequestTypeName());
    EXPECT_EQ(schemas[3].response.type_name, memochat::tests::moments::route_schema::MomentIdResponseTypeName());

    EXPECT_EQ(schemas[4].name, memochat::tests::moments::route_schema::LikeRouteName());
    EXPECT_EQ(schemas[4].method, memochat::tests::moments::route_schema::PostMethod());
    EXPECT_EQ(schemas[4].path, memochat::tests::moments::route_schema::LikePath());
    EXPECT_EQ(schemas[4].request.type_name, memochat::tests::moments::route_schema::LikeRequestTypeName());
    EXPECT_EQ(schemas[4].response.type_name, memochat::tests::moments::route_schema::LikeResponseTypeName());

    EXPECT_EQ(schemas[5].name, memochat::tests::moments::route_schema::CommentRouteName());
    EXPECT_EQ(schemas[5].method, memochat::tests::moments::route_schema::PostMethod());
    EXPECT_EQ(schemas[5].path, memochat::tests::moments::route_schema::CommentPath());
    EXPECT_EQ(schemas[5].request.type_name, memochat::tests::moments::route_schema::CommentRequestTypeName());
    EXPECT_EQ(schemas[5].response.type_name, memochat::tests::moments::route_schema::CommentMutationResponseTypeName());

    EXPECT_EQ(schemas[6].name, memochat::tests::moments::route_schema::CommentListRouteName());
    EXPECT_EQ(schemas[6].method, memochat::tests::moments::route_schema::PostMethod());
    EXPECT_EQ(schemas[6].path, memochat::tests::moments::route_schema::CommentListPath());
    EXPECT_EQ(schemas[6].request.type_name, memochat::tests::moments::route_schema::CommentListRequestTypeName());
    EXPECT_EQ(schemas[6].response.type_name, memochat::tests::moments::route_schema::CommentListResponseTypeName());

    EXPECT_EQ(schemas[7].name, memochat::tests::moments::route_schema::CommentLikeRouteName());
    EXPECT_EQ(schemas[7].method, memochat::tests::moments::route_schema::PostMethod());
    EXPECT_EQ(schemas[7].path, memochat::tests::moments::route_schema::CommentLikePath());
    EXPECT_EQ(schemas[7].request.type_name, memochat::tests::moments::route_schema::CommentLikeRequestTypeName());
    EXPECT_EQ(schemas[7].response.type_name, memochat::tests::moments::route_schema::CommentLikeResponseTypeName());
}

TEST(MomentsRouteSchemaTest, BuildsFieldInventoriesFromMomentsDtos)
{
    const auto schemas = memochat::gate::modules::moments::MomentsRouteModule::RouteSchemas();
    ASSERT_EQ(schemas.size(), 8U);

    ExpectFields(schemas[0].request, {"visibility", "location", "items"});
    ExpectFields(schemas[0].response, {"error", "moment_id"});

    ExpectFields(schemas[1].request, {"last_moment_id", "limit", "author_uid"});
    ExpectFields(schemas[1].response, {"error", "has_more", "moments"});

    ExpectFields(schemas[2].request, {"moment_id"});
    ExpectFields(schemas[2].response, {"error", "moment"});

    ExpectFields(schemas[3].request, {"moment_id"});
    ExpectFields(schemas[3].response, {"error", "moment_id"});

    ExpectFields(schemas[4].request, {"moment_id", "like"});
    ExpectFields(schemas[4].response, {"error", "moment_id", "has_liked", "like_count"});

    ExpectFields(schemas[5].request, {"moment_id", "content", "reply_uid", "comment_id", "delete", "delete_mode"});
    ExpectFields(schemas[5].response, {"error", "moment_id", "delete", "comment_count"});

    ExpectFields(schemas[6].request, {"moment_id", "last_comment_id", "limit"});
    ExpectFields(schemas[6].response, {"error", "moment_id", "has_more", "comment_count", "comments"});

    ExpectFields(schemas[7].request, {"comment_id", "like"});
    ExpectFields(schemas[7].response, {"error", "comment_id", "has_liked", "like_count", "like_names", "likes"});
}

TEST(MomentsRouteSchemaTest, RendersDeterministicSnapshotForReview)
{
    const auto schemas = memochat::gate::modules::moments::MomentsRouteModule::RouteSchemas();
    const std::string snapshot = memochat::gate::routing::RenderRouteSchemaSnapshot(schemas);

    EXPECT_EQ(snapshot, ExpectedMomentsRouteSchemaSnapshot());
}
