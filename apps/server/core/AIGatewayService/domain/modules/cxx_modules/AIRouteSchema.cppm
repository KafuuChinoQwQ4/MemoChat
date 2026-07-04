export module memochat.ai.route_schema_algorithms;

export namespace memochat::ai::route_schema::modules
{
const char* PostMethod()
{
    return "POST";
}

const char* RegisterApiProviderPath()
{
    return "/ai/model/api/register";
}

const char* RegisterApiProviderRouteName()
{
    return "ai.model.api.register";
}

const char* RegisterApiProviderRequestTypeName()
{
    return "AIRegisterApiProviderRequestDto";
}

const char* RegisterApiProviderResponseTypeName()
{
    return "AIRegisterApiProviderResponseDto";
}

const char* DeleteApiProviderPath()
{
    return "/ai/model/api/delete";
}

const char* DeleteApiProviderRouteName()
{
    return "ai.model.api.delete";
}

const char* DeleteApiProviderRequestTypeName()
{
    return "AIDeleteApiProviderRequestDto";
}

const char* DeleteApiProviderResponseTypeName()
{
    return "AIDeleteApiProviderResponseDto";
}

const char* KbUploadPath()
{
    return "/ai/kb/upload";
}

const char* KbUploadRouteName()
{
    return "ai.kb.upload";
}

const char* KbUploadRequestTypeName()
{
    return "AIKbUploadRequestDto";
}

const char* KbUploadResponseTypeName()
{
    return "AIKbUploadResponseDto";
}

const char* KbSearchPath()
{
    return "/ai/kb/search";
}

const char* KbSearchRouteName()
{
    return "ai.kb.search";
}

const char* KbSearchRequestTypeName()
{
    return "AIKbSearchRequestDto";
}

const char* KbSearchResponseTypeName()
{
    return "AIKbSearchResponseDto";
}

const char* KbDeletePath()
{
    return "/ai/kb/delete";
}

const char* KbDeleteRouteName()
{
    return "ai.kb.delete";
}

const char* KbDeleteRequestTypeName()
{
    return "AIKbDeleteRequestDto";
}

const char* SimpleResponseTypeName()
{
    return "AISimpleResponseDto";
}
} // namespace memochat::ai::route_schema::modules
