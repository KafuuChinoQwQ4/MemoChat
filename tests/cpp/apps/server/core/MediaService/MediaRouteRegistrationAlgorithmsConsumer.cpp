import memochat.media.route_registration_algorithms;

namespace memochat::tests::media::route_registration
{
const char* GetMethod()
{
    return memochat::media::route_registration::modules::GetMethod();
}

const char* PostMethod()
{
    return memochat::media::route_registration::modules::PostMethod();
}

const char* UploadInitPath()
{
    return memochat::media::route_registration::modules::UploadInitPath();
}

const char* UploadChunkPath()
{
    return memochat::media::route_registration::modules::UploadChunkPath();
}

const char* UploadStatusPath()
{
    return memochat::media::route_registration::modules::UploadStatusPath();
}

const char* UploadCompletePath()
{
    return memochat::media::route_registration::modules::UploadCompletePath();
}

const char* UploadSimplePath()
{
    return memochat::media::route_registration::modules::UploadSimplePath();
}

const char* MediaDownloadPath()
{
    return memochat::media::route_registration::modules::MediaDownloadPath();
}
} // namespace memochat::tests::media::route_registration
