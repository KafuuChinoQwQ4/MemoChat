export module memochat.chat.handler_registrar_algorithms;

export namespace memochat::chat::orchestration::handler_registrar::modules
{
constexpr unsigned ChatSessionHandlerCount()
{
    return 3U;
}

constexpr unsigned ChatRelationHandlerCount()
{
    return 7U;
}

constexpr unsigned PrivateMessageHandlerCount()
{
    return 6U;
}

constexpr unsigned GroupMessageHandlerCount()
{
    return 18U;
}

constexpr unsigned AsyncEventDispatcherHandlerCount()
{
    return 0U;
}

constexpr unsigned ExpectedTotalHandlerCount()
{
    return ChatSessionHandlerCount() + ChatRelationHandlerCount() + PrivateMessageHandlerCount() +
           GroupMessageHandlerCount() + AsyncEventDispatcherHandlerCount();
}

constexpr bool ShouldRegisterAsyncEventDispatcherHandlers()
{
    return AsyncEventDispatcherHandlerCount() > 0U;
}

constexpr bool DidRegistrarAddExpectedHandlers(unsigned long long before_size,
                                               unsigned long long after_size,
                                               unsigned expected_count)
{
    return after_size >= before_size && after_size - before_size == expected_count;
}
} // namespace memochat::chat::orchestration::handler_registrar::modules
