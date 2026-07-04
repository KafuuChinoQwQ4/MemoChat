import memochat.chat.postgres_mgr_algorithms;

namespace postgres_mgr_modules = memochat::chat::persistence::postgres_mgr::modules;

bool MemoChatTestPostgresMgrShouldResetDao(bool dao_exists)
{
    return postgres_mgr_modules::ShouldResetDao(dao_exists);
}

bool MemoChatTestPostgresMgrShouldInitializeDao(bool dao_exists)
{
    return postgres_mgr_modules::ShouldInitializeDao(dao_exists);
}

const char* MemoChatTestPostgresMgrInitFailureEvent()
{
    return postgres_mgr_modules::InitFailureEvent();
}

const char* MemoChatTestPostgresMgrInitFailureMessage()
{
    return postgres_mgr_modules::InitFailureMessage();
}
