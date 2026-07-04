export module memochat.chat.delivery_runtime_algorithms;

export namespace memochat::chat::delivery::runtime::modules
{
bool InitialStartedExpected()
{
    return false;
}

bool StopRequestedWhenStarting()
{
    return false;
}

bool StopRequestedWhenStopping()
{
    return true;
}

bool StartedAfterStopAndJoin()
{
    return false;
}

bool ShouldJoinThread(bool joinable)
{
    return joinable;
}

bool ShouldRunLoop(bool has_loop)
{
    return has_loop;
}
} // namespace memochat::chat::delivery::runtime::modules
