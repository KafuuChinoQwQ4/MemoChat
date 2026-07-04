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
    return "source.zip";
}

bool ShouldRejectImportPayload(bool encoded_empty, bool decode_ok)
{
    return encoded_empty || !decode_ok;
}

const char* InvalidPluginPackagePayloadMessage()
{
    return "invalid plugin package payload";
}

int SuccessHttpStatus()
{
    return 200;
}

int UnauthorizedHttpStatus()
{
    return 401;
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
