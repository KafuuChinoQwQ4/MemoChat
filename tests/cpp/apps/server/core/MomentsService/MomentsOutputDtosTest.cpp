#include <gtest/gtest.h>

#include "json/GlazeCompat.hpp"
#include "persistence/MomentTypes.hpp"
#include "reflection/StdReflectionIntrospection.hpp"
#include "services/moments/MomentsOutputDtos.hpp"
#include "services/moments/MomentsPublicDtos.hpp"

#include <array>
#include <string>
#include <string_view>
#include <vector>

#if MEMOCHAT_ENABLE_CPP26_REFLECTION
static_assert(memochat::reflection::FieldNamesEqual<memochat::gate::services::moments::MomentUserProfileDto>(
    std::array<std::string_view, 4>{"user_id", "user_name", "user_nick", "user_icon"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::gate::services::moments::MomentItemOutputDto>(
    std::array<std::string_view,
               8>{"seq", "media_type", "media_key", "thumb_key", "content", "width", "height", "duration_ms"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::gate::services::moments::MomentLikeOutputDto>(
    std::array<std::string_view, 4>{"uid", "user_nick", "user_icon", "created_at"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::gate::services::moments::MomentCommentOutputDto>(
    std::array<std::string_view, 13>{"id",
                                     "moment_id",
                                     "uid",
                                     "user_nick",
                                     "user_icon",
                                     "content",
                                     "reply_uid",
                                     "reply_nick",
                                     "created_at",
                                     "like_count",
                                     "has_liked",
                                     "like_names",
                                     "likes"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::gate::services::moments::MomentOutputDto>(
    std::array<std::string_view, 16>{"moment_id",
                                     "uid",
                                     "visibility",
                                     "location",
                                     "created_at",
                                     "like_count",
                                     "comment_count",
                                     "has_liked",
                                     "user_id",
                                     "user_name",
                                     "user_nick",
                                     "user_icon",
                                     "items",
                                     "like_names",
                                     "likes",
                                     "comments"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::gate::services::moments::MomentIdResponseDto>(
    std::array<std::string_view, 2>{"error", "moment_id"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::gate::services::moments::MomentListResponseDto>(
    std::array<std::string_view, 3>{"error", "has_more", "moments"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::gate::services::moments::MomentDetailResponseDto>(
    std::array<std::string_view, 2>{"error", "moment"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::gate::services::moments::MomentLikeResponseDto>(
    std::array<std::string_view, 4>{"error", "moment_id", "has_liked", "like_count"}));
static_assert(
    memochat::reflection::FieldNamesEqual<memochat::gate::services::moments::MomentCommentMutationResponseDto>(
        std::array<std::string_view, 4>{"error", "moment_id", "delete_", "comment_count"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::gate::services::moments::MomentCommentListResponseDto>(
    std::array<std::string_view, 5>{"error", "moment_id", "has_more", "comment_count", "comments"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::gate::services::moments::MomentCommentLikeResponseDto>(
    std::array<std::string_view, 6>{"error", "comment_id", "has_liked", "like_count", "like_names", "likes"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::gate::services::moments::MomentPublishRequestDto>(
    std::array<std::string_view, 3>{"visibility", "location", "items"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::gate::services::moments::MomentListRequestDto>(
    std::array<std::string_view, 3>{"last_moment_id", "limit", "author_uid"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::gate::services::moments::MomentIdRequestDto>(
    std::array<std::string_view, 1>{"moment_id"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::gate::services::moments::MomentLikeRequestDto>(
    std::array<std::string_view, 2>{"moment_id", "like"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::gate::services::moments::MomentCommentRequestDto>(
    std::array<std::string_view, 6>{"moment_id", "content", "reply_uid", "comment_id", "delete_", "delete_mode"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::gate::services::moments::MomentCommentListRequestDto>(
    std::array<std::string_view, 3>{"moment_id", "last_comment_id", "limit"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::gate::services::moments::MomentCommentLikeRequestDto>(
    std::array<std::string_view, 2>{"comment_id", "like"}));
#endif

namespace
{

memochat::json::JsonValue Parse(std::string_view body)
{
    memochat::json::JsonValue root;
    EXPECT_TRUE(memochat::json::glaze_parse(root, body));
    return root;
}

MomentItemInfo MakeItem()
{
    MomentItemInfo item;
    item.seq = 2;
    item.media_type = "image";
    item.media_key = "media-key";
    item.thumb_key = "thumb-key";
    item.content = "caption";
    item.width = 640;
    item.height = 480;
    item.duration_ms = 1234;
    return item;
}

MomentLikeInfo MakeLike(int uid, std::string_view nick, int64_t created_at)
{
    MomentLikeInfo like;
    like.uid = uid;
    like.user_nick = std::string(nick);
    like.user_icon = "icon-" + std::to_string(uid);
    like.created_at = created_at;
    return like;
}

MomentCommentInfo MakeComment()
{
    MomentCommentInfo comment;
    comment.id = 900;
    comment.moment_id = 100;
    comment.uid = 43;
    comment.user_nick = "Commenter";
    comment.user_icon = "commenter-icon";
    comment.content = "reply";
    comment.reply_uid = 42;
    comment.reply_nick = "Author";
    comment.created_at = 3000;
    comment.like_count = 2;
    comment.has_liked = true;
    comment.likes = {MakeLike(50, "Like A", 3100), MakeLike(51, "", 3200), MakeLike(52, "Like C", 3300)};
    return comment;
}

MomentInfo MakeMoment()
{
    MomentInfo moment;
    moment.moment_id = 100;
    moment.uid = 42;
    moment.visibility = 1;
    moment.location = "Shanghai";
    moment.created_at = 2000;
    moment.like_count = 3;
    moment.comment_count = 1;
    return moment;
}

MomentContentInfo MakeContent()
{
    MomentContentInfo content;
    content.moment_id = 100;
    content.items = {MakeItem()};
    content.server_time = 2050;
    return content;
}

memochat::gate::services::moments::MomentUserProfileDto MakeProfile()
{
    memochat::gate::services::moments::MomentUserProfileDto profile;
    profile.user_id = "u-42";
    profile.user_name = "alice";
    profile.user_nick = "Alice";
    profile.user_icon = "alice.png";
    return profile;
}

} // namespace

TEST(MomentsOutputDtosTest, ConvertsItemAndWritesExistingWireFields)
{
    const auto dto = memochat::gate::services::moments::ToMomentItemOutputDto(MakeItem());
    EXPECT_EQ(dto.seq, 2);
    EXPECT_EQ(dto.media_type, "image");
    EXPECT_EQ(dto.media_key, "media-key");
    EXPECT_EQ(dto.thumb_key, "thumb-key");
    EXPECT_EQ(dto.content, "caption");
    EXPECT_EQ(dto.width, 640);
    EXPECT_EQ(dto.height, 480);
    EXPECT_EQ(dto.duration_ms, 1234);

    const memochat::json::JsonValue root = memochat::gate::services::moments::ToJsonValue(dto);
    EXPECT_EQ(root["seq"].asInt(), 2);
    EXPECT_EQ(root["media_type"].asString(), "image");
    EXPECT_EQ(root["media_key"].asString(), "media-key");
    EXPECT_EQ(root["thumb_key"].asString(), "thumb-key");
    EXPECT_EQ(root["content"].asString(), "caption");
    EXPECT_EQ(root["width"].asInt(), 640);
    EXPECT_EQ(root["height"].asInt(), 480);
    EXPECT_EQ(root["duration_ms"].asInt(), 1234);
}

TEST(MomentsOutputDtosTest, ConvertsLikesAndFiltersLikeNames)
{
    const std::vector<MomentLikeInfo> likes = {MakeLike(50, "Like A", 3100), MakeLike(51, "", 3200)};

    const auto names = memochat::gate::services::moments::ToMomentLikeNames(likes);
    ASSERT_EQ(names.size(), 1);
    EXPECT_EQ(names[0], "Like A");

    const memochat::json::JsonValue likes_json = memochat::gate::services::moments::ToJsonValue(
        memochat::gate::services::moments::ToMomentLikeOutputDtos(likes));
    ASSERT_TRUE(likes_json.isArray());
    ASSERT_EQ(likes_json.size(), 2);
    EXPECT_EQ(likes_json[0]["uid"].asInt(), 50);
    EXPECT_EQ(likes_json[0]["user_nick"].asString(), "Like A");
    EXPECT_EQ(likes_json[0]["user_icon"].asString(), "icon-50");
    EXPECT_EQ(likes_json[0]["created_at"].asInt64(), 3100);
    EXPECT_EQ(likes_json[1]["user_nick"].asString(), "");
}

TEST(MomentsOutputDtosTest, ConvertsCommentWithNestedLikesAndNames)
{
    const auto dto = memochat::gate::services::moments::ToMomentCommentOutputDto(MakeComment());
    EXPECT_EQ(dto.id, 900);
    EXPECT_EQ(dto.moment_id, 100);
    EXPECT_EQ(dto.uid, 43);
    EXPECT_EQ(dto.user_nick, "Commenter");
    EXPECT_EQ(dto.reply_uid, 42);
    EXPECT_EQ(dto.reply_nick, "Author");
    EXPECT_EQ(dto.created_at, 3000);
    EXPECT_EQ(dto.like_count, 2);
    EXPECT_TRUE(dto.has_liked);
    ASSERT_EQ(dto.like_names.size(), 2);
    EXPECT_EQ(dto.like_names[0], "Like A");
    EXPECT_EQ(dto.like_names[1], "Like C");
    ASSERT_EQ(dto.likes.size(), 3);

    const memochat::json::JsonValue root = memochat::gate::services::moments::ToJsonValue(dto);
    EXPECT_EQ(root["id"].asInt64(), 900);
    EXPECT_EQ(root["moment_id"].asInt64(), 100);
    EXPECT_EQ(root["content"].asString(), "reply");
    EXPECT_TRUE(root["like_names"].isArray());
    EXPECT_EQ(root["like_names"].size(), 2);
    EXPECT_EQ(root["like_names"][0].asString(), "Like A");
    EXPECT_TRUE(root["likes"].isArray());
    EXPECT_EQ(root["likes"].size(), 3);
    EXPECT_EQ(root["likes"][2]["user_nick"].asString(), "Like C");
}

TEST(MomentsOutputDtosTest, ConvertsMomentWithFlattenedProfileAndArrays)
{
    const MomentInfo moment = MakeMoment();
    const MomentContentInfo content = MakeContent();
    const std::vector<MomentLikeInfo> likes = {MakeLike(50, "Like A", 3100), MakeLike(51, "", 3200)};
    const std::vector<MomentCommentInfo> comments = {MakeComment()};

    const auto dto =
        memochat::gate::services::moments::ToMomentOutputDto(moment, true, MakeProfile(), &content, &likes, &comments);
    EXPECT_EQ(dto.moment_id, 100);
    EXPECT_EQ(dto.uid, 42);
    EXPECT_EQ(dto.visibility, 1);
    EXPECT_EQ(dto.location, "Shanghai");
    EXPECT_EQ(dto.created_at, 2000);
    EXPECT_EQ(dto.like_count, 3);
    EXPECT_EQ(dto.comment_count, 1);
    EXPECT_TRUE(dto.has_liked);
    EXPECT_EQ(dto.user_id, "u-42");
    EXPECT_EQ(dto.user_name, "alice");
    EXPECT_EQ(dto.user_nick, "Alice");
    EXPECT_EQ(dto.user_icon, "alice.png");
    ASSERT_EQ(dto.items.size(), 1);
    ASSERT_EQ(dto.like_names.size(), 1);
    ASSERT_EQ(dto.likes.size(), 2);
    ASSERT_EQ(dto.comments.size(), 1);

    std::string body;
    std::string error;
    bool encoded = false;
    encoded = memochat::gate::services::moments::EncodeMomentOutput(dto, &body, &error);
    ASSERT_TRUE(encoded) << error;
    SCOPED_TRACE(body);

    memochat::json::JsonValue parsed_root;
    parsed_root = memochat::gate::services::moments::ToJsonValue(dto);
    const memochat::json::JsonValue root = parsed_root;
    ASSERT_TRUE(root.isObject()) << root.toStyledString();
    ASSERT_TRUE(root.isMember("moment_id")) << root.toStyledString();
    EXPECT_EQ(memochat::json::glaze_safe_get<int64_t>(root, "moment_id", 0), 100);
    EXPECT_EQ(memochat::json::glaze_safe_get<int>(root, "uid", 0), 42);
    EXPECT_EQ(memochat::json::glaze_safe_get<int>(root, "visibility", 0), 1);
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(root, "location", ""), "Shanghai");
    EXPECT_EQ(memochat::json::glaze_safe_get<int64_t>(root, "created_at", 0), 2000);
    EXPECT_TRUE(memochat::json::glaze_safe_get<bool>(root, "has_liked", false));
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(root, "user_id", ""), "u-42");
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(root, "user_name", ""), "alice");
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(root, "user_nick", ""), "Alice");
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(root, "user_icon", ""), "alice.png");

    const memochat::json::JsonValue items = root["items"];
    ASSERT_TRUE(items.isArray()) << root.toStyledString();
    ASSERT_EQ(items.size(), 1);
    const memochat::json::JsonValue first_item = items[0];
    ASSERT_TRUE(first_item.isObject()) << first_item.toStyledString();
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(first_item, "media_key", ""), "media-key");

    const memochat::json::JsonValue like_names = root["like_names"];
    ASSERT_TRUE(like_names.isArray()) << root.toStyledString();
    EXPECT_EQ(like_names.size(), 1);

    const memochat::json::JsonValue likes_json = root["likes"];
    ASSERT_TRUE(likes_json.isArray()) << root.toStyledString();
    EXPECT_EQ(likes_json.size(), 2);

    const memochat::json::JsonValue comments_json = root["comments"];
    ASSERT_TRUE(comments_json.isArray()) << root.toStyledString();
    EXPECT_EQ(comments_json.size(), 1);
}

TEST(MomentsOutputDtosTest, MissingOptionalListsStillSerializeAsEmptyArrays)
{
    const auto dto = memochat::gate::services::moments::ToMomentOutputDto(MakeMoment(),
                                                                          false,
                                                                          MakeProfile(),
                                                                          nullptr,
                                                                          nullptr,
                                                                          nullptr);
    const memochat::json::JsonValue root = memochat::gate::services::moments::ToJsonValue(dto);

    ASSERT_TRUE(root["items"].isArray());
    EXPECT_EQ(root["items"].size(), 0);
    ASSERT_TRUE(root["like_names"].isArray());
    EXPECT_EQ(root["like_names"].size(), 0);
    ASSERT_TRUE(root["likes"].isArray());
    EXPECT_EQ(root["likes"].size(), 0);
    ASSERT_TRUE(root["comments"].isArray());
    EXPECT_EQ(root["comments"].size(), 0);
}

TEST(MomentsOutputDtosTest, WritesMomentIdLikeListAndDetailResponses)
{
    const memochat::json::JsonValue id_json =
        memochat::gate::services::moments::ToJsonValue(memochat::gate::services::moments::MomentIdResponseDto{0, 100});
    ASSERT_TRUE(id_json.isObject()) << id_json.toStyledString();
    EXPECT_EQ(memochat::json::glaze_safe_get<int>(id_json, "error", -1), 0);
    EXPECT_EQ(memochat::json::glaze_safe_get<int64_t>(id_json, "moment_id", 0), 100);

    memochat::gate::services::moments::MomentLikeResponseDto like_response;
    like_response.error = 0;
    like_response.moment_id = 100;
    like_response.has_liked = true;
    like_response.like_count = 9;
    const memochat::json::JsonValue like_json = memochat::gate::services::moments::ToJsonValue(like_response);
    ASSERT_TRUE(like_json.isObject()) << like_json.toStyledString();
    EXPECT_EQ(memochat::json::glaze_safe_get<int64_t>(like_json, "moment_id", 0), 100);
    EXPECT_TRUE(memochat::json::glaze_safe_get<bool>(like_json, "has_liked", false));
    EXPECT_EQ(memochat::json::glaze_safe_get<int>(like_json, "like_count", 0), 9);

    const MomentContentInfo content = MakeContent();
    const auto moment = memochat::gate::services::moments::ToMomentOutputDto(MakeMoment(),
                                                                             true,
                                                                             MakeProfile(),
                                                                             &content,
                                                                             nullptr,
                                                                             nullptr);
    memochat::gate::services::moments::MomentListResponseDto list_response;
    list_response.error = 0;
    list_response.has_more = true;
    list_response.moments = {moment};
    const memochat::json::JsonValue list_json = memochat::gate::services::moments::ToJsonValue(list_response);
    ASSERT_TRUE(list_json.isObject()) << list_json.toStyledString();
    EXPECT_TRUE(memochat::json::glaze_safe_get<bool>(list_json, "has_more", false));
    const memochat::json::JsonValue list_moments = list_json["moments"];
    ASSERT_TRUE(list_moments.isArray()) << list_json.toStyledString();
    ASSERT_EQ(list_moments.size(), 1);
    const memochat::json::JsonValue list_first_moment = list_moments[0];
    EXPECT_EQ(memochat::json::glaze_safe_get<int64_t>(list_first_moment, "moment_id", 0), 100);

    const memochat::json::JsonValue detail_json = memochat::gate::services::moments::ToJsonValue(
        memochat::gate::services::moments::MomentDetailResponseDto{0, moment});
    ASSERT_TRUE(detail_json.isObject()) << detail_json.toStyledString();
    const memochat::json::JsonValue detail_moment = detail_json["moment"];
    ASSERT_TRUE(detail_moment.isObject()) << detail_json.toStyledString();
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(detail_moment, "user_nick", ""), "Alice");
}

TEST(MomentsOutputDtosTest, WritesCommentMutationResponsesWithDeleteWireNameAndOptionalCount)
{
    memochat::gate::services::moments::MomentCommentMutationResponseDto add_response;
    add_response.error = 0;
    add_response.moment_id = 100;
    add_response.delete_ = false;
    add_response.comment_count = 7;
    const memochat::json::JsonValue add_json = memochat::gate::services::moments::ToJsonValue(add_response);
    ASSERT_TRUE(add_json.isObject()) << add_json.toStyledString();
    EXPECT_EQ(memochat::json::glaze_safe_get<int64_t>(add_json, "moment_id", 0), 100);
    EXPECT_FALSE(memochat::json::glaze_safe_get<bool>(add_json, "delete", true));
    EXPECT_FALSE(add_json.isMember("delete_")) << add_json.toStyledString();
    EXPECT_EQ(memochat::json::glaze_safe_get<int>(add_json, "comment_count", 0), 7);

    memochat::gate::services::moments::MomentCommentMutationResponseDto delete_response;
    delete_response.error = 0;
    delete_response.moment_id = 100;
    delete_response.delete_ = true;
    const memochat::json::JsonValue delete_json = memochat::gate::services::moments::ToJsonValue(delete_response);
    ASSERT_TRUE(delete_json.isObject()) << delete_json.toStyledString();
    EXPECT_TRUE(memochat::json::glaze_safe_get<bool>(delete_json, "delete", false));
    EXPECT_FALSE(delete_json.isMember("comment_count")) << delete_json.toStyledString();
}

TEST(MomentsOutputDtosTest, WritesCommentListAndCommentLikeResponses)
{
    memochat::gate::services::moments::MomentCommentListResponseDto list_response;
    list_response.error = 0;
    list_response.moment_id = 100;
    list_response.has_more = true;
    list_response.comment_count = 3;
    list_response.comments = memochat::gate::services::moments::ToMomentCommentOutputDtos({MakeComment()});
    const memochat::json::JsonValue list_json = memochat::gate::services::moments::ToJsonValue(list_response);
    ASSERT_TRUE(list_json.isObject()) << list_json.toStyledString();
    EXPECT_EQ(memochat::json::glaze_safe_get<int64_t>(list_json, "moment_id", 0), 100);
    EXPECT_TRUE(memochat::json::glaze_safe_get<bool>(list_json, "has_more", false));
    EXPECT_EQ(memochat::json::glaze_safe_get<int>(list_json, "comment_count", 0), 3);
    const memochat::json::JsonValue comments = list_json["comments"];
    ASSERT_TRUE(comments.isArray()) << list_json.toStyledString();
    ASSERT_EQ(comments.size(), 1);
    const memochat::json::JsonValue first_comment = comments[0];
    EXPECT_EQ(memochat::json::glaze_safe_get<int64_t>(first_comment, "id", 0), 900);

    memochat::gate::services::moments::MomentCommentListResponseDto no_count_response;
    no_count_response.error = 0;
    no_count_response.moment_id = 100;
    const memochat::json::JsonValue no_count_json = memochat::gate::services::moments::ToJsonValue(no_count_response);
    ASSERT_TRUE(no_count_json.isObject()) << no_count_json.toStyledString();
    EXPECT_FALSE(no_count_json.isMember("comment_count")) << no_count_json.toStyledString();
    ASSERT_TRUE(no_count_json["comments"].isArray()) << no_count_json.toStyledString();

    const std::vector<MomentLikeInfo> likes = {MakeLike(50, "Like A", 3100), MakeLike(51, "", 3200)};
    memochat::gate::services::moments::MomentCommentLikeResponseDto like_response;
    like_response.error = 0;
    like_response.comment_id = 900;
    like_response.has_liked = true;
    like_response.like_count = 2;
    like_response.like_names = memochat::gate::services::moments::ToMomentLikeNames(likes);
    like_response.likes = memochat::gate::services::moments::ToMomentLikeOutputDtos(likes);
    const memochat::json::JsonValue like_json = memochat::gate::services::moments::ToJsonValue(like_response);
    ASSERT_TRUE(like_json.isObject()) << like_json.toStyledString();
    EXPECT_EQ(memochat::json::glaze_safe_get<int64_t>(like_json, "comment_id", 0), 900);
    EXPECT_TRUE(memochat::json::glaze_safe_get<bool>(like_json, "has_liked", false));
    EXPECT_EQ(memochat::json::glaze_safe_get<int>(like_json, "like_count", 0), 2);
    const memochat::json::JsonValue like_names = like_json["like_names"];
    ASSERT_TRUE(like_names.isArray()) << like_json.toStyledString();
    ASSERT_EQ(like_names.size(), 1);
    EXPECT_EQ(like_names[0].asString(), "Like A");
    const memochat::json::JsonValue likes_json = like_json["likes"];
    ASSERT_TRUE(likes_json.isArray()) << like_json.toStyledString();
    ASSERT_EQ(likes_json.size(), 2);
    const memochat::json::JsonValue second_like = likes_json[1];
    EXPECT_EQ(memochat::json::glaze_safe_get<std::string>(second_like, "user_nick", "fallback"), "");
}

TEST(MomentsOutputDtosTest, ParsesMomentRequestNumericStringAndBoolCoercions)
{
    const memochat::json::JsonValue root = Parse(
        R"({"moment_id":"100","comment_id":"900","last_moment_id":"88","last_comment_id":"77","reply_uid":"55","author_uid":"66","limit":"60","like":"0","delete":"true","content":""})");

    EXPECT_EQ(memochat::gate::services::moments::MomentsReadInt64(root, "moment_id", 0), 100);
    EXPECT_EQ(memochat::gate::services::moments::MomentsReadInt(root, "reply_uid", 0), 55);
    EXPECT_FALSE(memochat::gate::services::moments::MomentsReadBool(root, "like", true));
    EXPECT_TRUE(memochat::gate::services::moments::MomentsReadBool(root, "delete", false));

    const auto list = memochat::gate::services::moments::MomentListRequestFromJsonValue(root);
    EXPECT_EQ(list.last_moment_id, 88);
    EXPECT_EQ(list.author_uid, 66);
    EXPECT_EQ(list.limit, 50);

    const auto comments = memochat::gate::services::moments::MomentCommentListRequestFromJsonValue(root);
    EXPECT_EQ(comments.moment_id, 100);
    EXPECT_EQ(comments.last_comment_id, 77);
    EXPECT_EQ(comments.limit, 50);

    const auto comment = memochat::gate::services::moments::MomentCommentRequestFromJsonValue(root);
    EXPECT_EQ(comment.moment_id, 100);
    EXPECT_EQ(comment.comment_id, 900);
    EXPECT_EQ(comment.reply_uid, 55);
    EXPECT_TRUE(comment.delete_);
    EXPECT_TRUE(comment.delete_mode);

    const auto comment_like = memochat::gate::services::moments::MomentCommentLikeRequestFromJsonValue(root);
    EXPECT_EQ(comment_like.comment_id, 900);
    EXPECT_FALSE(comment_like.like);
}

TEST(MomentsOutputDtosTest, PreservesMomentRequestDefaultsAndWrongTypeFallbacks)
{
    const memochat::json::JsonValue root =
        Parse(R"({"moment_id":"bad","limit":0,"like":"bad","delete":{},"content":123})");

    const auto id = memochat::gate::services::moments::MomentIdRequestFromJsonValue(root);
    EXPECT_EQ(id.moment_id, 0);

    const auto list = memochat::gate::services::moments::MomentListRequestFromJsonValue(root);
    EXPECT_EQ(list.limit, 20);
    EXPECT_EQ(list.author_uid, 0);

    const auto like = memochat::gate::services::moments::MomentLikeRequestFromJsonValue(root);
    EXPECT_TRUE(like.like);

    const auto comment = memochat::gate::services::moments::MomentCommentRequestFromJsonValue(root);
    EXPECT_EQ(comment.content, "");
    EXPECT_FALSE(comment.delete_);
    EXPECT_FALSE(comment.delete_mode);
}

TEST(MomentsOutputDtosTest, ParsesPublishItemsAndTextContentFallback)
{
    const memochat::json::JsonValue with_items = Parse(
        R"({"visibility":"1","location":"Shanghai","items":[{"media_type":"image","media_key":"","thumb_key":"thumb","content":"caption","width":640,"height":480,"duration_ms":123},{"media_type":"video","media_key":"video-key","thumb_key":"thumb2","content":"clip","width":1280,"height":720,"duration_ms":9000},{"media_type":"unknown","media_key":"ignored","thumb_key":"ignored","content":"text","width":10,"height":20,"duration_ms":30}]})");

    const auto request = memochat::gate::services::moments::MomentPublishRequestFromJsonValue(with_items);
    EXPECT_EQ(request.visibility, 1);
    EXPECT_EQ(request.location, "Shanghai");
    ASSERT_EQ(request.items.size(), 3);

    EXPECT_EQ(request.items[0].media_type, "text");
    EXPECT_EQ(request.items[0].media_key, "");
    EXPECT_EQ(request.items[0].thumb_key, "");
    EXPECT_EQ(request.items[0].width, 0);
    EXPECT_EQ(request.items[0].height, 0);
    EXPECT_EQ(request.items[0].duration_ms, 0);
    EXPECT_EQ(request.items[0].content, "caption");

    EXPECT_EQ(request.items[1].media_type, "video");
    EXPECT_EQ(request.items[1].media_key, "video-key");
    EXPECT_EQ(request.items[1].thumb_key, "thumb2");
    EXPECT_EQ(request.items[1].duration_ms, 9000);

    EXPECT_EQ(request.items[2].media_type, "text");
    EXPECT_EQ(request.items[2].media_key, "");
    EXPECT_EQ(request.items[2].thumb_key, "");
    EXPECT_EQ(request.items[2].width, 0);

    const memochat::json::JsonValue text_only = Parse(R"({"content":"plain text"})");
    const auto text_only_request = memochat::gate::services::moments::MomentPublishRequestFromJsonValue(text_only);
    ASSERT_EQ(text_only_request.items.size(), 1);
    EXPECT_EQ(text_only_request.items[0].media_type, "text");
    EXPECT_EQ(text_only_request.items[0].content, "plain text");
}
