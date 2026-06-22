#include <gtest/gtest.h>

#include "services/media/MediaUploadSessionDto.h"
#include "json/GlazeCompat.h"
#include "reflection/StdReflectionIntrospection.h"

#include <array>
#include <string>
#include <string_view>

#if MEMOCHAT_ENABLE_CPP26_REFLECTION
static_assert(memochat::reflection::FieldNamesEqual<memochat::media::MediaUploadSessionDto>(
    std::array<std::string_view, 11>{"uid",
                                     "upload_id",
                                     "media_type",
                                     "file_name",
                                     "mime",
                                     "file_size",
                                     "chunk_size",
                                     "total_chunks",
                                     "created_at",
                                     "expires_at",
                                     "storage_provider"}));
#endif

namespace
{

memochat::media::MediaUploadSessionDto MakeSession()
{
    memochat::media::MediaUploadSessionDto session;
    session.uid = 42;
    session.upload_id = "upload-1";
    session.media_type = "image";
    session.file_name = "photo.png";
    session.mime = "image/png";
    session.file_size = 4096;
    session.chunk_size = 1024;
    session.total_chunks = 4;
    session.created_at = 1000;
    session.expires_at = 2000;
    session.storage_provider = "local";
    return session;
}

} // namespace

TEST(MediaUploadSessionDtoTest, EncodesSessionWithExistingWireFieldNames)
{
    std::string body;
    ASSERT_TRUE(memochat::media::EncodeMediaUploadSession(MakeSession(), &body));

    memochat::json::JsonValue root;
    ASSERT_TRUE(memochat::json::glaze_parse(root, body));
    EXPECT_EQ(root["uid"].asInt(), 42);
    EXPECT_EQ(root["upload_id"].asString(), "upload-1");
    EXPECT_EQ(root["media_type"].asString(), "image");
    EXPECT_EQ(root["file_name"].asString(), "photo.png");
    EXPECT_EQ(root["mime"].asString(), "image/png");
    EXPECT_EQ(root["file_size"].asInt64(), 4096);
    EXPECT_EQ(root["chunk_size"].asInt(), 1024);
    EXPECT_EQ(root["total_chunks"].asInt(), 4);
    EXPECT_EQ(root["created_at"].asInt64(), 1000);
    EXPECT_EQ(root["expires_at"].asInt64(), 2000);
    EXPECT_EQ(root["storage_provider"].asString(), "local");
}

TEST(MediaUploadSessionDtoTest, DecodesFullSession)
{
    memochat::media::MediaUploadSessionDto session;
    ASSERT_TRUE(memochat::media::DecodeMediaUploadSession(
        R"({"uid":7,"upload_id":"upload-2","media_type":"file","file_name":"doc.pdf","mime":"application/pdf","file_size":2048,"chunk_size":1024,"total_chunks":2,"created_at":10,"expires_at":20,"storage_provider":"s3"})",
        &session));

    EXPECT_EQ(session.uid, 7);
    EXPECT_EQ(session.upload_id, "upload-2");
    EXPECT_EQ(session.media_type, "file");
    EXPECT_EQ(session.file_name, "doc.pdf");
    EXPECT_EQ(session.mime, "application/pdf");
    EXPECT_EQ(session.file_size, 2048);
    EXPECT_EQ(session.chunk_size, 1024);
    EXPECT_EQ(session.total_chunks, 2);
    EXPECT_EQ(session.created_at, 10);
    EXPECT_EQ(session.expires_at, 20);
    EXPECT_EQ(session.storage_provider, "s3");
}

TEST(MediaUploadSessionDtoTest, DecodesMissingOptionalFieldsWithDefaults)
{
    memochat::media::MediaUploadSessionDto session;
    ASSERT_TRUE(memochat::media::DecodeMediaUploadSession(
        R"({"uid":7,"upload_id":"upload-3","file_name":"file.bin","file_size":1,"chunk_size":1,"total_chunks":1})",
        &session));

    EXPECT_EQ(session.uid, 7);
    EXPECT_EQ(session.upload_id, "upload-3");
    EXPECT_EQ(session.media_type, "file");
    EXPECT_EQ(session.file_name, "file.bin");
    EXPECT_TRUE(session.mime.empty());
    EXPECT_EQ(session.file_size, 1);
    EXPECT_EQ(session.chunk_size, 1);
    EXPECT_EQ(session.total_chunks, 1);
    EXPECT_EQ(session.created_at, 0);
    EXPECT_EQ(session.expires_at, 0);
    EXPECT_EQ(session.storage_provider, "local");
}

TEST(MediaUploadSessionDtoTest, RejectsInvalidSession)
{
    memochat::media::MediaUploadSessionDto session;

    EXPECT_FALSE(memochat::media::DecodeMediaUploadSession("not-json", &session));
    EXPECT_FALSE(memochat::media::DecodeMediaUploadSession(
        R"({"uid":0,"upload_id":"upload-4","file_name":"file.bin","file_size":1,"chunk_size":1,"total_chunks":1})",
        &session));
    EXPECT_FALSE(memochat::media::DecodeMediaUploadSession(
        R"({"uid":7,"upload_id":"","file_name":"file.bin","file_size":1,"chunk_size":1,"total_chunks":1})",
        &session));
    EXPECT_FALSE(memochat::media::DecodeMediaUploadSession(
        R"({"uid":7,"upload_id":"upload-4","file_size":1,"chunk_size":1,"total_chunks":1})",
        &session));
    EXPECT_FALSE(memochat::media::DecodeMediaUploadSession(
        R"({"uid":7,"upload_id":"upload-4","file_name":"file.bin","file_size":0,"chunk_size":1,"total_chunks":1})",
        &session));
    EXPECT_FALSE(memochat::media::DecodeMediaUploadSession(
        R"({"uid":7,"upload_id":"upload-4","file_name":"file.bin","file_size":1,"chunk_size":0,"total_chunks":1})",
        &session));
    EXPECT_FALSE(memochat::media::DecodeMediaUploadSession(
        R"({"uid":7,"upload_id":"upload-4","file_name":"file.bin","file_size":1,"chunk_size":1,"total_chunks":0})",
        &session));
    EXPECT_FALSE(
        memochat::media::DecodeMediaUploadSession(R"({"uid":7})",
                                                  static_cast<memochat::media::MediaUploadSessionDto*>(nullptr)));
}

TEST(MediaUploadSessionDtoTest, BridgesSessionToAndFromJsonValue)
{
    const memochat::json::JsonValue value = memochat::media::MediaUploadSessionToJsonValue(MakeSession());
    EXPECT_EQ(value["upload_id"].asString(), "upload-1");
    EXPECT_EQ(value["file_name"].asString(), "photo.png");

    memochat::media::MediaUploadSessionDto decoded;
    ASSERT_TRUE(memochat::media::MediaUploadSessionFromJsonValue(value, &decoded));
    EXPECT_EQ(decoded.uid, 42);
    EXPECT_EQ(decoded.upload_id, "upload-1");
    EXPECT_EQ(decoded.media_type, "image");
    EXPECT_EQ(decoded.storage_provider, "local");
}

TEST(MediaUploadSessionDtoTest, ReportsNullEncodeOutput)
{
    std::string error;

    EXPECT_FALSE(memochat::media::EncodeMediaUploadSession(MakeSession(), nullptr, &error));
    EXPECT_EQ(error, "output pointer is null");
}
