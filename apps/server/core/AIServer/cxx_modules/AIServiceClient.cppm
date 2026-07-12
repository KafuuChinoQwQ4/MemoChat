export module memochat.ai.client_algorithms;

export namespace memochat::ai::client::modules
{
enum class PositiveIntParseStatus
{
    Configured,
    Empty,
    Invalid,
};

struct PositiveIntParseResult
{
    int value = 0;
    PositiveIntParseStatus status = PositiveIntParseStatus::Invalid;
};

PositiveIntParseResult ParsePositiveIntOr(const char* raw, unsigned long raw_size, int fallback)
{
    if (raw_size == 0)
    {
        return {fallback, PositiveIntParseStatus::Empty};
    }
    if (raw == nullptr)
    {
        return {fallback, PositiveIntParseStatus::Invalid};
    }

    constexpr unsigned long long max_int = 2147483647ULL;
    unsigned long long value = 0;
    for (unsigned long index = 0; index < raw_size; ++index)
    {
        const char ch = raw[index];
        if (ch < '0' || ch > '9')
        {
            return {fallback, PositiveIntParseStatus::Invalid};
        }
        const auto digit = static_cast<unsigned long long>(ch - '0');
        if (value > (max_int - digit) / 10ULL)
        {
            return {fallback, PositiveIntParseStatus::Invalid};
        }
        value = value * 10ULL + digit;
    }
    if (value == 0)
    {
        return {fallback, PositiveIntParseStatus::Invalid};
    }
    return {static_cast<int>(value), PositiveIntParseStatus::Configured};
}

int DefaultTimeoutSec()
{
    return 300;
}

int DefaultAgentTaskLimit()
{
    return 50;
}

int HttpVersion()
{
    return 11;
}

int SuccessCode()
{
    return 0;
}

const char* JsonContentType()
{
    return "application/json";
}

const char* EmptyBody()
{
    return "";
}

const char* EmptyJsonArray()
{
    return "[]";
}

const char* ChatPath()
{
    return "/chat";
}

const char* ChatStreamPath()
{
    return "/chat/stream";
}

const char* SmartPath()
{
    return "/smart";
}

const char* KbUploadPath()
{
    return "/kb/upload";
}

const char* KbSearchPath()
{
    return "/kb/search";
}

const char* ModelsPath()
{
    return "/models";
}

const char* RegisterApiProviderPath()
{
    return "/models/api-provider";
}

const char* DeleteApiProviderPath()
{
    return "/models/api-provider/delete";
}

const char* KbListPathPrefix()
{
    return "/kb/list?uid=";
}

const char* KbPathPrefix()
{
    return "/kb/";
}

const char* UidQueryPrefix()
{
    return "?uid=";
}

const char* AgentMemoryListPathPrefix()
{
    return "/agent/memory?uid=";
}

const char* AgentMemoryPath()
{
    return "/agent/memory";
}

const char* AgentMemoryItemPathPrefix()
{
    return "/agent/memory/";
}

const char* AgentTasksPath()
{
    return "/agent/tasks";
}

const char* AgentTasksListPathPrefix()
{
    return "/agent/tasks?uid=";
}

const char* LimitQueryPrefix()
{
    return "&limit=";
}

const char* CancelSuffix()
{
    return "/cancel";
}

const char* ResumeSuffix()
{
    return "/resume";
}

const char* DefaultDeploymentPreference()
{
    return "any";
}

const char* DefaultApiProviderAdapter()
{
    return "openai_compatible";
}

unsigned long SseDataPrefixSize()
{
    return 6;
}

const char* SseDataPrefix()
{
    return "data: ";
}

const char* SseDoneSentinel()
{
    return "[DONE]";
}

bool EqualsAscii(const char* text, unsigned long text_size, const char* expected)
{
    if (text == nullptr || expected == nullptr)
    {
        return false;
    }

    unsigned long index = 0;
    for (; index < text_size; ++index)
    {
        if (expected[index] == '\0' || text[index] != expected[index])
        {
            return false;
        }
    }

    return expected[index] == '\0';
}

bool StartsWithAscii(const char* text, unsigned long text_size, const char* prefix)
{
    if (text == nullptr || prefix == nullptr)
    {
        return false;
    }

    for (unsigned long index = 0; prefix[index] != '\0'; ++index)
    {
        if (index >= text_size || text[index] != prefix[index])
        {
            return false;
        }
    }

    return true;
}

bool IsSseDataLine(const char* line, unsigned long line_size)
{
    return StartsWithAscii(line, line_size, SseDataPrefix());
}

bool IsSseDoneSentinel(const char* text, unsigned long text_size)
{
    return EqualsAscii(text, text_size, SseDoneSentinel());
}

bool ShouldSetJsonBody(bool has_body)
{
    return has_body;
}

bool ShouldAttachMetadata(bool metadata_empty)
{
    return !metadata_empty;
}

bool ShouldUseParsedMetadata(bool parse_ok, bool metadata_is_object)
{
    return parse_ok && metadata_is_object;
}

bool ShouldKeepUrlByte(bool is_alnum, unsigned char ch)
{
    return is_alnum || ch == '-' || ch == '_' || ch == '.' || ch == '~';
}

bool ShouldUseDefaultDeploymentPreference(bool preference_empty)
{
    return preference_empty;
}

bool ShouldUseDefaultApiProviderAdapter(bool adapter_empty)
{
    return adapter_empty;
}

bool ShouldUpdateCurrentMessageId(bool msg_id_empty)
{
    return !msg_id_empty;
}

bool ShouldUpdateTotalTokens(long long tokens)
{
    return tokens > 0;
}

bool ShouldEmitChunk(bool callback_present)
{
    return callback_present;
}

bool ShouldStoreFinalChunkResult(bool final_flag, bool out_result_present)
{
    return final_flag && out_result_present;
}

bool ShouldFallbackStreamResult(bool saw_final, bool out_result_present)
{
    return !saw_final && out_result_present;
}

bool IsLineTrimChar(char ch)
{
    return ch == ' ' || ch == '\t' || ch == '\r';
}
} // namespace memochat::ai::client::modules
