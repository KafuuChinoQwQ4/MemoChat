export module memochat.chat.group_response_algorithms;

export namespace memochat::chat::message::group_response::modules
{
bool HasPermissionBit(long long permission_bits, long long permission_bit)
{
    return (permission_bits & permission_bit) != 0;
}
} // namespace memochat::chat::message::group_response::modules
