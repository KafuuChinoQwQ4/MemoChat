import memochat.account.profile_support_algorithms;

namespace memochat::tests::account::profile_support
{
int ProfileSuccessCode()
{
    return memochat::account::profile_support::modules::ProfileSuccessCode();
}

int ProfileErrorCode()
{
    return memochat::account::profile_support::modules::ProfileErrorCode();
}

const char* MissingRequiredFieldsMessage()
{
    return memochat::account::profile_support::modules::MissingRequiredFieldsMessage();
}

const char* InvalidUidOrNickMessage()
{
    return memochat::account::profile_support::modules::InvalidUidOrNickMessage();
}

const char* ProfileUpdateFailedMessage()
{
    return memochat::account::profile_support::modules::ProfileUpdateFailedMessage();
}

const char* InvalidUidMessage()
{
    return memochat::account::profile_support::modules::InvalidUidMessage();
}

const char* UserNotFoundMessage()
{
    return memochat::account::profile_support::modules::UserNotFoundMessage();
}

const char* UserBaseInfoKeyPrefix()
{
    return memochat::account::profile_support::modules::UserBaseInfoKeyPrefix();
}

const char* UserNameInfoKeyPrefix()
{
    return memochat::account::profile_support::modules::UserNameInfoKeyPrefix();
}

bool IsUidValid(int uid)
{
    return memochat::account::profile_support::modules::IsUidValid(uid);
}
} // namespace memochat::tests::account::profile_support
