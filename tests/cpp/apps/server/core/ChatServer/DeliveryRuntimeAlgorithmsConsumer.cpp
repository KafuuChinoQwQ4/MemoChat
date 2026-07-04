import memochat.chat.delivery_runtime_algorithms;

bool MemoChatTestDeliveryRuntimeInitialStartedExpected()
{
    return memochat::chat::delivery::runtime::modules::InitialStartedExpected();
}

bool MemoChatTestDeliveryRuntimeStopRequestedWhenStarting()
{
    return memochat::chat::delivery::runtime::modules::StopRequestedWhenStarting();
}

bool MemoChatTestDeliveryRuntimeStopRequestedWhenStopping()
{
    return memochat::chat::delivery::runtime::modules::StopRequestedWhenStopping();
}

bool MemoChatTestDeliveryRuntimeStartedAfterStopAndJoin()
{
    return memochat::chat::delivery::runtime::modules::StartedAfterStopAndJoin();
}

bool MemoChatTestDeliveryRuntimeShouldJoinThread(bool joinable)
{
    return memochat::chat::delivery::runtime::modules::ShouldJoinThread(joinable);
}

bool MemoChatTestDeliveryRuntimeShouldRunLoop(bool has_loop)
{
    return memochat::chat::delivery::runtime::modules::ShouldRunLoop(has_loop);
}
