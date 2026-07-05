export module memochat.account.auth_public_dto_algorithms;

export namespace memochat::account::auth_public::modules
{
bool HasAuthEmailRequiredShape(bool has_email)
{
    return has_email;
}

bool HasProfileUpdateRequiredShape(bool has_token, bool has_nick, bool has_desc, bool has_icon)
{
    return has_token && has_nick && has_desc && has_icon;
}
} // namespace memochat::account::auth_public::modules
