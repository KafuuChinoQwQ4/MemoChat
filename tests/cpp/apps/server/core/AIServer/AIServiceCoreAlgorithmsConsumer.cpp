import memochat.ai.core_algorithms;

namespace memochat::tests::ai::core
{
int SuccessCode()
{
    return memochat::ai::core::modules::SuccessCode();
}

int BadRequestCode()
{
    return memochat::ai::core::modules::BadRequestCode();
}

int NotFoundCode()
{
    return memochat::ai::core::modules::NotFoundCode();
}

int UpstreamFailureCode()
{
    return memochat::ai::core::modules::UpstreamFailureCode();
}

int ServiceUnavailableCode()
{
    return memochat::ai::core::modules::ServiceUnavailableCode();
}

int DefaultHistoryLimit()
{
    return memochat::ai::core::modules::DefaultHistoryLimit();
}

int SelectHistoryLimit(int requested_limit)
{
    return memochat::ai::core::modules::SelectHistoryLimit(requested_limit);
}

const char* OkMessage()
{
    return memochat::ai::core::modules::OkMessage();
}

const char* UserRole()
{
    return memochat::ai::core::modules::UserRole();
}

const char* AssistantRole()
{
    return memochat::ai::core::modules::AssistantRole();
}

const char* EmptyJsonObject()
{
    return memochat::ai::core::modules::EmptyJsonObject();
}

const char* EmptyJsonArray()
{
    return memochat::ai::core::modules::EmptyJsonArray();
}

const char* RequiredSessionUpdateMessage()
{
    return memochat::ai::core::modules::RequiredSessionUpdateMessage();
}

const char* SessionNotFoundMessage()
{
    return memochat::ai::core::modules::SessionNotFoundMessage();
}

const char* DefaultApiProviderAdapter()
{
    return memochat::ai::core::modules::DefaultApiProviderAdapter();
}

bool IsAsciiTrimChar(char ch)
{
    return memochat::ai::core::modules::IsAsciiTrimChar(ch);
}

bool ShouldCreateSessionFromRequest(bool model_type_empty, bool model_name_empty)
{
    return memochat::ai::core::modules::ShouldCreateSessionFromRequest(model_type_empty, model_name_empty);
}

bool ShouldRejectSessionUpdate(int uid, bool session_id_empty, bool title_empty)
{
    return memochat::ai::core::modules::ShouldRejectSessionUpdate(uid, session_id_empty, title_empty);
}

bool ShouldUseDefaultApiProviderAdapter(bool adapter_empty)
{
    return memochat::ai::core::modules::ShouldUseDefaultApiProviderAdapter(adapter_empty);
}
} // namespace memochat::tests::ai::core
