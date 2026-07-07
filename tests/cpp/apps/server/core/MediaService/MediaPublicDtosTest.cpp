#include <gtest/gtest.h>

#include "services/media/MediaPublicDtos.hpp"
#include "reflection/StdReflectionIntrospection.hpp"

#include <array>
#include <string>
#include <string_view>

#if MEMOCHAT_ENABLE_CPP26_REFLECTION
static_assert(memochat::reflection::FieldNamesEqual<memochat::media::MediaUploadInitRequestDto>(
    std::array<std::string_view, 8>{"media_type",
                                    "file_name",
                                    "mime",
                                    "file_size",
                                    "grant_uids",
                                    "grant_group_id",
                                    "grant_public",
                                    "grant_friends"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::media::MediaUploadChunkJsonRequestDto>(
    std::array<std::string_view, 3>{"upload_id", "index", "data_base64"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::media::MediaUploadCompleteRequestDto>(
    std::array<std::string_view, 1>{"upload_id"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::media::MediaUploadSimpleRequestDto>(
    std::array<std::string_view, 8>{"media_type",
                                    "file_name",
                                    "mime",
                                    "data_base64",
                                    "grant_uids",
                                    "grant_group_id",
                                    "grant_public",
                                    "grant_friends"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::media::MediaUploadInitResponseDto>(
    std::array<std::string_view, 4>{"upload_id", "chunk_size", "total_chunks", "uploaded_chunks"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::media::MediaUploadChunkResponseDto>(
    std::array<std::string_view, 3>{"upload_id", "index", "size"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::media::MediaUploadStatusResponseDto>(
    std::array<std::string_view, 4>{"upload_id", "total_chunks", "chunk_size", "uploaded_chunks"}));
static_assert(memochat::reflection::FieldNamesEqual<memochat::media::MediaUploadAssetResponseDto>(
    std::array<std::string_view, 6>{"media_key", "media_type", "file_name", "mime", "size", "url"}));
#endif

TEST(MediaPublicDtosTest, DecodesUploadInitRequestBusinessFieldsOnly)
{
    memochat::media::MediaUploadInitRequestDto request;
    ASSERT_TRUE(memochat::media::DecodeMediaUploadInitRequest(
        R"({"media_type":"image","file_name":"photo.png","mime":"image/png","file_size":4096})",
        &request));

    EXPECT_EQ(request.media_type, "image");
    EXPECT_EQ(request.file_name, "photo.png");
    EXPECT_EQ(request.mime, "image/png");
    EXPECT_EQ(request.file_size, 4096);
    EXPECT_TRUE(request.grant_uids.empty());
    EXPECT_EQ(request.grant_group_id, 0);
    EXPECT_FALSE(request.grant_public);
    EXPECT_FALSE(request.grant_friends);
}

TEST(MediaPublicDtosTest, DecodesUploadInitGrantAudience)
{
    memochat::media::MediaUploadInitRequestDto request;
    ASSERT_TRUE(memochat::media::DecodeMediaUploadInitRequest(
        R"({"file_name":"photo.png","file_size":4096,"grant_uids":[7,7,0,-1,8],"grant_group_id":99,"grant_public":true,"grant_friends":true})",
        &request));

    ASSERT_EQ(request.grant_uids.size(), 2U);
    EXPECT_EQ(request.grant_uids[0], 7);
    EXPECT_EQ(request.grant_uids[1], 8);
    EXPECT_EQ(request.grant_group_id, 99);
    EXPECT_TRUE(request.grant_public);
    EXPECT_TRUE(request.grant_friends);
}

TEST(MediaPublicDtosTest, DefaultsUploadInitMediaTypeToFile)
{
    memochat::media::MediaUploadInitRequestDto request;
    ASSERT_TRUE(memochat::media::DecodeMediaUploadInitRequest(R"({"file_name":"doc.bin","file_size":1})", &request));
    EXPECT_EQ(request.media_type, "file");
    EXPECT_TRUE(request.mime.empty());

    ASSERT_TRUE(
        memochat::media::DecodeMediaUploadInitRequest(R"({"media_type":"","file_name":"doc.bin","file_size":1})",
                                                      &request));
    EXPECT_EQ(request.media_type, "file");
}

TEST(MediaPublicDtosTest, DecodesChunkJsonRequestBusinessFieldsOnly)
{
    memochat::media::MediaUploadChunkJsonRequestDto request;
    ASSERT_TRUE(
        memochat::media::DecodeMediaUploadChunkJsonRequest(R"({"upload_id":"upload-1","index":3,"data_base64":"YWJj"})",
                                                           &request));

    EXPECT_EQ(request.upload_id, "upload-1");
    EXPECT_EQ(request.index, 3);
    EXPECT_EQ(request.data_base64, "YWJj");
}

TEST(MediaPublicDtosTest, DefaultsChunkJsonIndexToMinusOne)
{
    memochat::media::MediaUploadChunkJsonRequestDto request;
    ASSERT_TRUE(memochat::media::DecodeMediaUploadChunkJsonRequest(R"({"upload_id":"upload-1","data_base64":"YWJj"})",
                                                                   &request));

    EXPECT_EQ(request.index, -1);
}

TEST(MediaPublicDtosTest, KeepsSafeGetDefaultsForWrongTypes)
{
    memochat::media::MediaUploadInitRequestDto init;
    ASSERT_TRUE(memochat::media::DecodeMediaUploadInitRequest(
        R"({"media_type":12,"file_name":false,"mime":{},"file_size":"bad"})",
        &init));
    EXPECT_EQ(init.media_type, "file");
    EXPECT_TRUE(init.file_name.empty());
    EXPECT_TRUE(init.mime.empty());
    EXPECT_EQ(init.file_size, 0);
    EXPECT_TRUE(init.grant_uids.empty());
    EXPECT_EQ(init.grant_group_id, 0);
    EXPECT_FALSE(init.grant_public);
    EXPECT_FALSE(init.grant_friends);

    memochat::media::MediaUploadChunkJsonRequestDto chunk;
    ASSERT_TRUE(
        memochat::media::DecodeMediaUploadChunkJsonRequest(R"({"upload_id":false,"index":"bad","data_base64":{}})",
                                                           &chunk));
    EXPECT_TRUE(chunk.upload_id.empty());
    EXPECT_EQ(chunk.index, -1);
    EXPECT_TRUE(chunk.data_base64.empty());
}

TEST(MediaPublicDtosTest, DecodesCompleteRequestBusinessFieldsOnly)
{
    memochat::media::MediaUploadCompleteRequestDto request;
    ASSERT_TRUE(memochat::media::DecodeMediaUploadCompleteRequest(R"({"upload_id":"upload-1"})", &request));

    EXPECT_EQ(request.upload_id, "upload-1");
}

TEST(MediaPublicDtosTest, DecodesSimpleRequestBusinessFieldsOnly)
{
    memochat::media::MediaUploadSimpleRequestDto request;
    ASSERT_TRUE(memochat::media::DecodeMediaUploadSimpleRequest(
        R"({"media_type":"image","file_name":"photo.png","mime":"image/png","data_base64":"YWJj"})",
        &request));

    EXPECT_EQ(request.media_type, "image");
    EXPECT_EQ(request.file_name, "photo.png");
    EXPECT_EQ(request.mime, "image/png");
    EXPECT_EQ(request.data_base64, "YWJj");
    EXPECT_TRUE(request.grant_uids.empty());
    EXPECT_EQ(request.grant_group_id, 0);
    EXPECT_FALSE(request.grant_public);
    EXPECT_FALSE(request.grant_friends);
}

TEST(MediaPublicDtosTest, DecodesSimpleRequestGrantAudience)
{
    memochat::media::MediaUploadSimpleRequestDto request;
    ASSERT_TRUE(memochat::media::DecodeMediaUploadSimpleRequest(
        R"({"file_name":"photo.png","data_base64":"YWJj","grant_uids":[99,12,12],"grant_group_id":123,"grant_public":false,"grant_friends":true})",
        &request));

    ASSERT_EQ(request.grant_uids.size(), 2U);
    EXPECT_EQ(request.grant_uids[0], 12);
    EXPECT_EQ(request.grant_uids[1], 99);
    EXPECT_EQ(request.grant_group_id, 123);
    EXPECT_FALSE(request.grant_public);
    EXPECT_TRUE(request.grant_friends);
}

TEST(MediaPublicDtosTest, DefaultsSimpleMediaTypeToFile)
{
    memochat::media::MediaUploadSimpleRequestDto request;
    ASSERT_TRUE(
        memochat::media::DecodeMediaUploadSimpleRequest(R"({"file_name":"doc.bin","data_base64":"YWJj"})", &request));
    EXPECT_EQ(request.media_type, "file");
    EXPECT_TRUE(request.mime.empty());

    ASSERT_TRUE(memochat::media::DecodeMediaUploadSimpleRequest(
        R"({"media_type":"","file_name":"doc.bin","data_base64":"YWJj"})",
        &request));
    EXPECT_EQ(request.media_type, "file");
}

TEST(MediaPublicDtosTest, RejectsMalformedJsonAndNullOutputs)
{
    memochat::media::MediaUploadInitRequestDto init;
    EXPECT_FALSE(memochat::media::DecodeMediaUploadInitRequest("not-json", &init));
    EXPECT_FALSE(memochat::media::DecodeMediaUploadInitRequest(
        R"({})",
        static_cast<memochat::media::MediaUploadInitRequestDto*>(nullptr)));

    memochat::media::MediaUploadChunkJsonRequestDto chunk;
    EXPECT_FALSE(memochat::media::DecodeMediaUploadChunkJsonRequest("not-json", &chunk));
    EXPECT_FALSE(memochat::media::DecodeMediaUploadChunkJsonRequest(
        R"({})",
        static_cast<memochat::media::MediaUploadChunkJsonRequestDto*>(nullptr)));

    memochat::media::MediaUploadCompleteRequestDto complete;
    EXPECT_FALSE(memochat::media::DecodeMediaUploadCompleteRequest("not-json", &complete));
    EXPECT_FALSE(memochat::media::DecodeMediaUploadCompleteRequest(
        R"({})",
        static_cast<memochat::media::MediaUploadCompleteRequestDto*>(nullptr)));

    memochat::media::MediaUploadSimpleRequestDto simple;
    EXPECT_FALSE(memochat::media::DecodeMediaUploadSimpleRequest("not-json", &simple));
    EXPECT_FALSE(memochat::media::DecodeMediaUploadSimpleRequest(
        R"({})",
        static_cast<memochat::media::MediaUploadSimpleRequestDto*>(nullptr)));
}

TEST(MediaPublicDtosTest, DetectsJsonContentTypeLikeExistingHandlers)
{
    EXPECT_TRUE(memochat::media::IsJsonContentType("application/json"));
    EXPECT_TRUE(memochat::media::IsJsonContentType("Application/Json; charset=utf-8"));
    EXPECT_FALSE(memochat::media::IsJsonContentType("application/octet-stream"));
}

TEST(MediaPublicDtosTest, WritesUploadInitResponseWithEmptyUploadedChunksArray)
{
    memochat::media::MediaUploadInitResponseDto response;
    response.upload_id = "upload-1";
    response.chunk_size = 4096;
    response.total_chunks = 3;

    const memochat::json::JsonValue root = memochat::media::MediaUploadInitResponseToJsonValue(response);

    EXPECT_EQ(root["upload_id"].asString(), "upload-1");
    EXPECT_EQ(root["chunk_size"].asInt(), 4096);
    EXPECT_EQ(root["total_chunks"].asInt(), 3);
    ASSERT_TRUE(root["uploaded_chunks"].isArray());
    EXPECT_EQ(root["uploaded_chunks"].size(), 0);
}

TEST(MediaPublicDtosTest, WritesUploadChunkResponseWithExistingWireFields)
{
    memochat::media::MediaUploadChunkResponseDto response;
    response.upload_id = "upload-1";
    response.index = 2;
    response.size = 1024;

    const memochat::json::JsonValue root = memochat::media::MediaUploadChunkResponseToJsonValue(response);

    EXPECT_EQ(root["upload_id"].asString(), "upload-1");
    EXPECT_EQ(root["index"].asInt(), 2);
    EXPECT_EQ(root["size"].asInt64(), 1024);
}

TEST(MediaPublicDtosTest, WritesUploadStatusResponseWithUploadedChunks)
{
    memochat::media::MediaUploadStatusResponseDto response;
    response.upload_id = "upload-1";
    response.total_chunks = 3;
    response.chunk_size = 4096;
    response.uploaded_chunks = {0, 2};

    const memochat::json::JsonValue root = memochat::media::MediaUploadStatusResponseToJsonValue(response);

    EXPECT_EQ(root["upload_id"].asString(), "upload-1");
    EXPECT_EQ(root["total_chunks"].asInt(), 3);
    EXPECT_EQ(root["chunk_size"].asInt(), 4096);
    ASSERT_TRUE(root["uploaded_chunks"].isArray());
    ASSERT_EQ(root["uploaded_chunks"].size(), 2);
    EXPECT_EQ(root["uploaded_chunks"][0].asInt(), 0);
    EXPECT_EQ(root["uploaded_chunks"][1].asInt(), 2);
}

TEST(MediaPublicDtosTest, WritesUploadAssetResponseWithExistingWireFields)
{
    memochat::media::MediaUploadAssetResponseDto response;
    response.media_key = "media-1";
    response.media_type = "image";
    response.file_name = "photo.png";
    response.mime = "image/png";
    response.size = 2048;
    response.url = "/media/download?asset=media-1";

    const memochat::json::JsonValue root = memochat::media::MediaUploadAssetResponseToJsonValue(response);

    EXPECT_EQ(root["media_key"].asString(), "media-1");
    EXPECT_EQ(root["media_type"].asString(), "image");
    EXPECT_EQ(root["file_name"].asString(), "photo.png");
    EXPECT_EQ(root["mime"].asString(), "image/png");
    EXPECT_EQ(root["size"].asInt64(), 2048);
    EXPECT_EQ(root["url"].asString(), "/media/download?asset=media-1");
}
