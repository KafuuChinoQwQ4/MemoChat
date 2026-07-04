export module memochat.ai.route_registration_algorithms;

export namespace memochat::ai::route_registration::modules
{
const char* GetMethod()
{
    return "GET";
}

const char* PostMethod()
{
    return "POST";
}

const char* ChatPath()
{
    return "/ai/chat";
}

const char* SmartPath()
{
    return "/ai/smart";
}

const char* HistoryPath()
{
    return "/ai/history";
}

const char* SessionPath()
{
    return "/ai/session";
}

const char* SessionListPath()
{
    return "/ai/session/list";
}

const char* SessionDeletePath()
{
    return "/ai/session/delete";
}

const char* SessionUpdatePath()
{
    return "/ai/session/update";
}

const char* ModelListPath()
{
    return "/ai/model/list";
}

const char* ModelApiRegisterPath()
{
    return "/ai/model/api/register";
}

const char* ModelApiDeletePath()
{
    return "/ai/model/api/delete";
}

const char* KbUploadPath()
{
    return "/ai/kb/upload";
}

const char* KbSearchPath()
{
    return "/ai/kb/search";
}

const char* KbListPath()
{
    return "/ai/kb/list";
}

const char* KbDeletePath()
{
    return "/ai/kb/delete";
}

const char* MemoryListPath()
{
    return "/ai/memory/list";
}

const char* MemoryPath()
{
    return "/ai/memory";
}

const char* MemoryDeletePath()
{
    return "/ai/memory/delete";
}

const char* TasksPath()
{
    return "/ai/tasks";
}

const char* TaskDetailPath()
{
    return "/ai/tasks/detail";
}

const char* TaskCancelPath()
{
    return "/ai/tasks/cancel";
}

const char* TaskResumePath()
{
    return "/ai/tasks/resume";
}
} // namespace memochat::ai::route_registration::modules
