#include "modules/media/MediaRouteModule.h"

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

const char* ExpectedMediaRouteSchemaSnapshot()
{
    return "route: media.upload.init\n"
           "method: POST\n"
           "path: /upload_media_init\n"
           "request: MediaUploadInitRequestDto\n"
           "  - uid\n"
           "  - token\n"
           "  - media_type\n"
           "  - file_name\n"
           "  - mime\n"
           "  - file_size\n"
           "response: MediaUploadInitResponseDto\n"
           "  - upload_id\n"
           "  - chunk_size\n"
           "  - total_chunks\n"
           "  - uploaded_chunks\n"
           "\n"
           "route: media.upload.chunk_json\n"
           "method: POST\n"
           "path: /upload_media_chunk\n"
           "request: MediaUploadChunkJsonRequestDto\n"
           "  - uid\n"
           "  - token\n"
           "  - upload_id\n"
           "  - index\n"
           "  - data_base64\n"
           "response: MediaUploadChunkResponseDto\n"
           "  - upload_id\n"
           "  - index\n"
           "  - size\n"
           "\n"
           "route: media.upload.complete\n"
           "method: POST\n"
           "path: /upload_media_complete\n"
           "request: MediaUploadCompleteRequestDto\n"
           "  - uid\n"
           "  - token\n"
           "  - upload_id\n"
           "response: MediaUploadAssetResponseDto\n"
           "  - media_key\n"
           "  - media_type\n"
           "  - file_name\n"
           "  - mime\n"
           "  - size\n"
           "  - url\n"
           "\n"
           "route: media.upload.simple\n"
           "method: POST\n"
           "path: /upload_media\n"
           "request: MediaUploadSimpleRequestDto\n"
           "  - uid\n"
           "  - token\n"
           "  - media_type\n"
           "  - file_name\n"
           "  - mime\n"
           "  - data_base64\n"
           "response: MediaUploadAssetResponseDto\n"
           "  - media_key\n"
           "  - media_type\n"
           "  - file_name\n"
           "  - mime\n"
           "  - size\n"
           "  - url\n"
           "\n";
}

} // namespace

TEST(MediaRouteSchemaTest, ListsOnlySchemaEligibleMediaJsonBodyRoutes)
{
    const auto schemas = memochat::gate::modules::media::MediaRouteModule::RouteSchemas();

    ASSERT_EQ(schemas.size(), 4U);
    EXPECT_EQ(schemas[0].name, "media.upload.init");
    EXPECT_EQ(schemas[0].method, "POST");
    EXPECT_EQ(schemas[0].path, "/upload_media_init");
    EXPECT_EQ(schemas[0].request.type_name, "MediaUploadInitRequestDto");
    EXPECT_EQ(schemas[0].response.type_name, "MediaUploadInitResponseDto");

    EXPECT_EQ(schemas[1].name, "media.upload.chunk_json");
    EXPECT_EQ(schemas[1].method, "POST");
    EXPECT_EQ(schemas[1].path, "/upload_media_chunk");
    EXPECT_EQ(schemas[1].request.type_name, "MediaUploadChunkJsonRequestDto");
    EXPECT_EQ(schemas[1].response.type_name, "MediaUploadChunkResponseDto");

    EXPECT_EQ(schemas[2].name, "media.upload.complete");
    EXPECT_EQ(schemas[2].method, "POST");
    EXPECT_EQ(schemas[2].path, "/upload_media_complete");
    EXPECT_EQ(schemas[2].request.type_name, "MediaUploadCompleteRequestDto");
    EXPECT_EQ(schemas[2].response.type_name, "MediaUploadAssetResponseDto");

    EXPECT_EQ(schemas[3].name, "media.upload.simple");
    EXPECT_EQ(schemas[3].method, "POST");
    EXPECT_EQ(schemas[3].path, "/upload_media");
    EXPECT_EQ(schemas[3].request.type_name, "MediaUploadSimpleRequestDto");
    EXPECT_EQ(schemas[3].response.type_name, "MediaUploadAssetResponseDto");
}

TEST(MediaRouteSchemaTest, BuildsFieldInventoriesFromMediaDtos)
{
    const auto schemas = memochat::gate::modules::media::MediaRouteModule::RouteSchemas();
    ASSERT_EQ(schemas.size(), 4U);

    ExpectFields(schemas[0].request, {"uid", "token", "media_type", "file_name", "mime", "file_size"});
    ExpectFields(schemas[0].response, {"upload_id", "chunk_size", "total_chunks", "uploaded_chunks"});

    ExpectFields(schemas[1].request, {"uid", "token", "upload_id", "index", "data_base64"});
    ExpectFields(schemas[1].response, {"upload_id", "index", "size"});

    ExpectFields(schemas[2].request, {"uid", "token", "upload_id"});
    ExpectFields(schemas[2].response, {"media_key", "media_type", "file_name", "mime", "size", "url"});

    ExpectFields(schemas[3].request, {"uid", "token", "media_type", "file_name", "mime", "data_base64"});
    ExpectFields(schemas[3].response, {"media_key", "media_type", "file_name", "mime", "size", "url"});
}

TEST(MediaRouteSchemaTest, RendersDeterministicSnapshotForReview)
{
    const auto schemas = memochat::gate::modules::media::MediaRouteModule::RouteSchemas();
    const std::string snapshot = memochat::gate::routing::RenderRouteSchemaSnapshot(schemas);

    EXPECT_EQ(snapshot, ExpectedMediaRouteSchemaSnapshot());
}
