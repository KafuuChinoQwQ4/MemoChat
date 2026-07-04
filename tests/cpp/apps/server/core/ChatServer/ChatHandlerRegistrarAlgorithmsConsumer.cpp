import memochat.chat.handler_registrar_algorithms;

namespace modules = memochat::chat::orchestration::handler_registrar::modules;

unsigned MemoChatTestChatSessionHandlerCount()
{
    return modules::ChatSessionHandlerCount();
}

unsigned MemoChatTestChatRelationHandlerCount()
{
    return modules::ChatRelationHandlerCount();
}

unsigned MemoChatTestPrivateMessageHandlerCount()
{
    return modules::PrivateMessageHandlerCount();
}

unsigned MemoChatTestGroupMessageHandlerCount()
{
    return modules::GroupMessageHandlerCount();
}

unsigned MemoChatTestAsyncEventDispatcherHandlerCount()
{
    return modules::AsyncEventDispatcherHandlerCount();
}

unsigned MemoChatTestExpectedTotalHandlerCount()
{
    return modules::ExpectedTotalHandlerCount();
}

bool MemoChatTestShouldRegisterAsyncEventDispatcherHandlers()
{
    return modules::ShouldRegisterAsyncEventDispatcherHandlers();
}

bool MemoChatTestDidRegistrarAddExpectedHandlers(unsigned long long before_size,
                                                 unsigned long long after_size,
                                                 unsigned expected_count)
{
    return modules::DidRegistrarAddExpectedHandlers(before_size, after_size, expected_count);
}
