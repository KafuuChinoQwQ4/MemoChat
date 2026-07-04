import memochat.ai.gateway_service_algorithms;

namespace memochat::tests::ai::gateway_service
{
int SuccessStatusCode()
{
    return memochat::gate::services::ai::service_modules::SuccessStatusCode();
}

const char* JsonContentType()
{
    return memochat::gate::services::ai::service_modules::JsonContentType();
}

int InvalidJsonErrorCode()
{
    return memochat::gate::services::ai::service_modules::InvalidJsonErrorCode();
}

const char* InvalidJsonMessage()
{
    return memochat::gate::services::ai::service_modules::InvalidJsonMessage();
}

int ContentEmptyErrorCode()
{
    return memochat::gate::services::ai::service_modules::ContentEmptyErrorCode();
}

const char* ContentEmptyMessage()
{
    return memochat::gate::services::ai::service_modules::ContentEmptyMessage();
}

int UnauthorizedStatusCode()
{
    return memochat::gate::services::ai::service_modules::UnauthorizedStatusCode();
}

int ForbiddenStatusCode()
{
    return memochat::gate::services::ai::service_modules::ForbiddenStatusCode();
}

int TokenInvalidErrorCode()
{
    return memochat::gate::services::ai::service_modules::TokenInvalidErrorCode();
}

const char* TokenInvalidMessage()
{
    return memochat::gate::services::ai::service_modules::TokenInvalidMessage();
}

const char* ProviderAdminAuthRequiredMessage()
{
    return memochat::gate::services::ai::service_modules::ProviderAdminAuthRequiredMessage();
}

const char* DefaultProviderAdminAuthHeader()
{
    return memochat::gate::services::ai::service_modules::DefaultProviderAdminAuthHeader();
}

const char* DefaultProviderAdminKeyEnv()
{
    return memochat::gate::services::ai::service_modules::DefaultProviderAdminKeyEnv();
}

int DefaultHistoryLimit()
{
    return memochat::gate::services::ai::service_modules::DefaultHistoryLimit();
}

int DefaultHistoryOffset()
{
    return memochat::gate::services::ai::service_modules::DefaultHistoryOffset();
}

int DefaultTaskListLimit()
{
    return memochat::gate::services::ai::service_modules::DefaultTaskListLimit();
}

const char* DefaultModelType()
{
    return memochat::gate::services::ai::service_modules::DefaultModelType();
}

bool ShouldRejectEmptyContent(bool content_empty)
{
    return memochat::gate::services::ai::service_modules::ShouldRejectEmptyContent(content_empty);
}

bool ShouldRejectUserAuth(int uid, bool token_empty)
{
    return memochat::gate::services::ai::service_modules::ShouldRejectUserAuth(uid, token_empty);
}

bool ShouldRejectProviderAdminAuth(bool key_configured, bool supplied_empty, bool token_matches)
{
    return memochat::gate::services::ai::service_modules::ShouldRejectProviderAdminAuth(key_configured,
                                                                                        supplied_empty,
                                                                                        token_matches);
}
} // namespace memochat::tests::ai::gateway_service
