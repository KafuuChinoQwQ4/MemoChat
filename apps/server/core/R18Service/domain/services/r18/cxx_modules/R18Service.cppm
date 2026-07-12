export module memochat.r18.service_algorithms;

export namespace memochat::r18::service::modules
{
const char* DefaultSourceId()
{
    return "";
}

const char* EmptyFieldDefault()
{
    return "";
}

const char* TokenInvalidMessage()
{
    return "token invalid";
}

const char* ImportFileNameField()
{
    return "file_name";
}

const char* ImportDataBase64Field()
{
    return "data_base64";
}

const char* ImportManifestJsonField()
{
    return "manifest_json";
}

const char* DefaultImportFileName()
{
    return "source.js";
}

bool ShouldRejectImportPayload(bool encoded_empty, bool decode_ok)
{
    return encoded_empty || !decode_ok;
}

const char* InvalidPluginPackagePayloadMessage()
{
    return "invalid plugin package payload";
}

const char* DefaultSourceAdminHeader()
{
    return "X-MemoChat-R18-Source-Admin-Key";
}

const char* SourceAdminRequiredMessage()
{
    return "R18 source administrator authorization required";
}

bool ShouldRejectSourceAdminAuth(bool key_configured, bool supplied_empty, bool token_matches)
{
    return !key_configured || supplied_empty || !token_matches;
}

int SuccessHttpStatus()
{
    return 200;
}

int UnauthorizedHttpStatus()
{
    return 401;
}

int ForbiddenHttpStatus()
{
    return 403;
}

int BadGatewayHttpStatus()
{
    return 502;
}

const char* GetJsonContentType()
{
    return "text/json";
}

const char* PostJsonContentType()
{
    return "application/json";
}

const char* PlainTextContentType()
{
    return "text/plain";
}

const char* ImageFetchFailedPrefix()
{
    return "image fetch failed: ";
}
} // namespace memochat::r18::service::modules
