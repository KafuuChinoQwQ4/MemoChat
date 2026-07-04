export module memochat.media.route_schema_algorithms;

export namespace memochat::media::route_schema::modules
{
const char* PostMethod()
{
    return "POST";
}

const char* UploadInitPath()
{
    return "/upload_media_init";
}

const char* UploadInitRouteName()
{
    return "media.upload.init";
}

const char* UploadInitRequestTypeName()
{
    return "MediaUploadInitRequestDto";
}

const char* UploadInitResponseTypeName()
{
    return "MediaUploadInitResponseDto";
}

const char* UploadChunkJsonPath()
{
    return "/upload_media_chunk";
}

const char* UploadChunkJsonRouteName()
{
    return "media.upload.chunk_json";
}

const char* UploadChunkJsonRequestTypeName()
{
    return "MediaUploadChunkJsonRequestDto";
}

const char* UploadChunkJsonResponseTypeName()
{
    return "MediaUploadChunkResponseDto";
}

const char* UploadCompletePath()
{
    return "/upload_media_complete";
}

const char* UploadCompleteRouteName()
{
    return "media.upload.complete";
}

const char* UploadCompleteRequestTypeName()
{
    return "MediaUploadCompleteRequestDto";
}

const char* UploadAssetResponseTypeName()
{
    return "MediaUploadAssetResponseDto";
}

const char* UploadSimplePath()
{
    return "/upload_media";
}

const char* UploadSimpleRouteName()
{
    return "media.upload.simple";
}

const char* UploadSimpleRequestTypeName()
{
    return "MediaUploadSimpleRequestDto";
}
} // namespace memochat::media::route_schema::modules
