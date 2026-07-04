export module memochat.media.route_registration_algorithms;

export namespace memochat::media::route_registration::modules
{
const char* GetMethod()
{
    return "GET";
}

const char* PostMethod()
{
    return "POST";
}

const char* UploadInitPath()
{
    return "/upload_media_init";
}

const char* UploadChunkPath()
{
    return "/upload_media_chunk";
}

const char* UploadStatusPath()
{
    return "/upload_media_status";
}

const char* UploadCompletePath()
{
    return "/upload_media_complete";
}

const char* UploadSimplePath()
{
    return "/upload_media";
}

const char* MediaDownloadPath()
{
    return "/media/download";
}
} // namespace memochat::media::route_registration::modules
