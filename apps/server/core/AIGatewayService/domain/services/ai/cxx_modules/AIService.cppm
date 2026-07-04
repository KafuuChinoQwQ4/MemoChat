export module memochat.ai.gateway_service_algorithms;

export namespace memochat::gate::services::ai::service_modules
{
int SuccessStatusCode()
{
    return 200;
}

const char* JsonContentType()
{
    return "application/json";
}

int InvalidJsonErrorCode()
{
    return 1;
}

const char* InvalidJsonMessage()
{
    return "invalid json";
}

int ContentEmptyErrorCode()
{
    return 1;
}

const char* ContentEmptyMessage()
{
    return "content is empty";
}

int UnauthorizedStatusCode()
{
    return 401;
}

int ForbiddenStatusCode()
{
    return 403;
}

int TokenInvalidErrorCode()
{
    return 401;
}

const char* TokenInvalidMessage()
{
    return "token invalid or missing";
}

const char* ProviderAdminAuthRequiredMessage()
{
    return "provider admin auth required";
}

const char* DefaultProviderAdminAuthHeader()
{
    return "X-MemoChat-AI-Provider-Admin-Key";
}

const char* DefaultProviderAdminKeyEnv()
{
    return "MEMOCHAT_AI_PROVIDER_ADMIN_KEY";
}

int DefaultHistoryLimit()
{
    return 20;
}

int DefaultHistoryOffset()
{
    return 0;
}

int DefaultTaskListLimit()
{
    return 50;
}

const char* DefaultModelType()
{
    return "ollama";
}

bool ShouldRejectEmptyContent(bool content_empty)
{
    return content_empty;
}

bool ShouldRejectUserAuth(int uid, bool token_empty)
{
    return uid <= 0 || token_empty;
}

bool ShouldRejectProviderAdminAuth(bool key_configured, bool supplied_empty, bool token_matches)
{
    return !key_configured || supplied_empty || !token_matches;
}
} // namespace memochat::gate::services::ai::service_modules
