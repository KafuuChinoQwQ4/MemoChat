export module memochat.ai.core_algorithms;

export namespace memochat::ai::core::modules
{
int SuccessCode()
{
    return 0;
}

int BadRequestCode()
{
    return 400;
}

int NotFoundCode()
{
    return 404;
}

int UpstreamFailureCode()
{
    return 500;
}

int ServiceUnavailableCode()
{
    return 503;
}

int DefaultHistoryLimit()
{
    return 20;
}

int SelectHistoryLimit(int requested_limit)
{
    return requested_limit > 0 ? requested_limit : DefaultHistoryLimit();
}

const char* OkMessage()
{
    return "ok";
}

const char* UserRole()
{
    return "user";
}

const char* AssistantRole()
{
    return "assistant";
}

const char* EmptyJsonObject()
{
    return "{}";
}

const char* EmptyJsonArray()
{
    return "[]";
}

const char* RequiredSessionUpdateMessage()
{
    return "uid, session_id and title are required";
}

const char* SessionNotFoundMessage()
{
    return "session not found";
}

const char* SessionCreateFailedMessage()
{
    return "session creation failed";
}

const char* DefaultApiProviderAdapter()
{
    return "openai_compatible";
}

bool IsAsciiTrimChar(char ch)
{
    return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
}

bool ShouldCreateSessionFromRequest(bool model_type_empty, bool model_name_empty)
{
    return !model_type_empty && !model_name_empty;
}

bool ShouldRejectSessionUpdate(int uid, bool session_id_empty, bool title_empty)
{
    return uid <= 0 || session_id_empty || title_empty;
}

bool ShouldUseDefaultApiProviderAdapter(bool adapter_empty)
{
    return adapter_empty;
}
} // namespace memochat::ai::core::modules
