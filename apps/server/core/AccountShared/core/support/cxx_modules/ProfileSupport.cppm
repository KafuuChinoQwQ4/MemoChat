export module memochat.account.profile_support_algorithms;

export namespace memochat::account::profile_support::modules
{
// Http2ProfileSupport reports success with error code 0 and validation/handler
// failures with error code 1.
int ProfileSuccessCode()
{
    return 0;
}

int ProfileErrorCode()
{
    return 1;
}

const char* MissingRequiredFieldsMessage()
{
    return "missing required fields";
}

const char* InvalidUidOrNickMessage()
{
    return "invalid uid or nick";
}

const char* ProfileUpdateFailedMessage()
{
    return "profile update failed";
}

const char* InvalidUidMessage()
{
    return "invalid uid";
}

const char* UserNotFoundMessage()
{
    return "user not found";
}

const char* UserBaseInfoKeyPrefix()
{
    return "ubaseinfo_";
}

const char* UserNameInfoKeyPrefix()
{
    return "nameinfo_";
}

// Profile update rejects a non-positive uid; the caller additionally rejects an
// empty nick.
bool IsUidValid(int uid)
{
    return uid > 0;
}
} // namespace memochat::account::profile_support::modules
