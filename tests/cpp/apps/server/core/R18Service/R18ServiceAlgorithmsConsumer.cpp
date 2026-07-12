import memochat.r18.service_algorithms;

namespace memochat::tests::r18::service
{
const char* DefaultSourceId()
{
    return memochat::r18::service::modules::DefaultSourceId();
}

const char* EmptyFieldDefault()
{
    return memochat::r18::service::modules::EmptyFieldDefault();
}

const char* TokenInvalidMessage()
{
    return memochat::r18::service::modules::TokenInvalidMessage();
}

const char* ImportFileNameField()
{
    return memochat::r18::service::modules::ImportFileNameField();
}

const char* ImportDataBase64Field()
{
    return memochat::r18::service::modules::ImportDataBase64Field();
}

const char* ImportManifestJsonField()
{
    return memochat::r18::service::modules::ImportManifestJsonField();
}

const char* DefaultImportFileName()
{
    return memochat::r18::service::modules::DefaultImportFileName();
}

bool ShouldRejectImportPayload(bool encoded_empty, bool decode_ok)
{
    return memochat::r18::service::modules::ShouldRejectImportPayload(encoded_empty, decode_ok);
}

const char* InvalidPluginPackagePayloadMessage()
{
    return memochat::r18::service::modules::InvalidPluginPackagePayloadMessage();
}

const char* DefaultSourceAdminHeader()
{
    return memochat::r18::service::modules::DefaultSourceAdminHeader();
}

const char* SourceAdminRequiredMessage()
{
    return memochat::r18::service::modules::SourceAdminRequiredMessage();
}

bool ShouldRejectSourceAdminAuth(bool key_configured, bool supplied_empty, bool token_matches)
{
    return memochat::r18::service::modules::ShouldRejectSourceAdminAuth(key_configured, supplied_empty, token_matches);
}

int SuccessHttpStatus()
{
    return memochat::r18::service::modules::SuccessHttpStatus();
}

int UnauthorizedHttpStatus()
{
    return memochat::r18::service::modules::UnauthorizedHttpStatus();
}

int ForbiddenHttpStatus()
{
    return memochat::r18::service::modules::ForbiddenHttpStatus();
}

int BadGatewayHttpStatus()
{
    return memochat::r18::service::modules::BadGatewayHttpStatus();
}

const char* GetJsonContentType()
{
    return memochat::r18::service::modules::GetJsonContentType();
}

const char* PostJsonContentType()
{
    return memochat::r18::service::modules::PostJsonContentType();
}

const char* PlainTextContentType()
{
    return memochat::r18::service::modules::PlainTextContentType();
}

const char* ImageFetchFailedPrefix()
{
    return memochat::r18::service::modules::ImageFetchFailedPrefix();
}
} // namespace memochat::tests::r18::service
