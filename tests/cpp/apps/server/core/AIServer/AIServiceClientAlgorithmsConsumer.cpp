import memochat.ai.client_algorithms;

namespace memochat::tests::ai::client
{
int ParsePositiveIntValue(const char* raw, unsigned long raw_size, int fallback)
{
    return memochat::ai::client::modules::ParsePositiveIntOr(raw, raw_size, fallback).value;
}

bool PositiveIntWasConfigured(const char* raw, unsigned long raw_size, int fallback)
{
    return memochat::ai::client::modules::ParsePositiveIntOr(raw, raw_size, fallback).status ==
           memochat::ai::client::modules::PositiveIntParseStatus::Configured;
}

bool PositiveIntWasEmpty(const char* raw, unsigned long raw_size, int fallback)
{
    return memochat::ai::client::modules::ParsePositiveIntOr(raw, raw_size, fallback).status ==
           memochat::ai::client::modules::PositiveIntParseStatus::Empty;
}

bool PositiveIntWasInvalid(const char* raw, unsigned long raw_size, int fallback)
{
    return memochat::ai::client::modules::ParsePositiveIntOr(raw, raw_size, fallback).status ==
           memochat::ai::client::modules::PositiveIntParseStatus::Invalid;
}

int DefaultTimeoutSec()
{
    return memochat::ai::client::modules::DefaultTimeoutSec();
}

int DefaultAgentTaskLimit()
{
    return memochat::ai::client::modules::DefaultAgentTaskLimit();
}

int HttpVersion()
{
    return memochat::ai::client::modules::HttpVersion();
}

int SuccessCode()
{
    return memochat::ai::client::modules::SuccessCode();
}

const char* JsonContentType()
{
    return memochat::ai::client::modules::JsonContentType();
}

const char* EmptyBody()
{
    return memochat::ai::client::modules::EmptyBody();
}

const char* EmptyJsonArray()
{
    return memochat::ai::client::modules::EmptyJsonArray();
}

const char* ChatPath()
{
    return memochat::ai::client::modules::ChatPath();
}

const char* ChatStreamPath()
{
    return memochat::ai::client::modules::ChatStreamPath();
}

const char* SmartPath()
{
    return memochat::ai::client::modules::SmartPath();
}

const char* KbUploadPath()
{
    return memochat::ai::client::modules::KbUploadPath();
}

const char* KbSearchPath()
{
    return memochat::ai::client::modules::KbSearchPath();
}

const char* ModelsPath()
{
    return memochat::ai::client::modules::ModelsPath();
}

const char* RegisterApiProviderPath()
{
    return memochat::ai::client::modules::RegisterApiProviderPath();
}

const char* DeleteApiProviderPath()
{
    return memochat::ai::client::modules::DeleteApiProviderPath();
}

const char* KbListPathPrefix()
{
    return memochat::ai::client::modules::KbListPathPrefix();
}

const char* KbPathPrefix()
{
    return memochat::ai::client::modules::KbPathPrefix();
}

const char* UidQueryPrefix()
{
    return memochat::ai::client::modules::UidQueryPrefix();
}

const char* AgentMemoryListPathPrefix()
{
    return memochat::ai::client::modules::AgentMemoryListPathPrefix();
}

const char* AgentMemoryPath()
{
    return memochat::ai::client::modules::AgentMemoryPath();
}

const char* AgentMemoryItemPathPrefix()
{
    return memochat::ai::client::modules::AgentMemoryItemPathPrefix();
}

const char* AgentTasksPath()
{
    return memochat::ai::client::modules::AgentTasksPath();
}

const char* AgentTasksListPathPrefix()
{
    return memochat::ai::client::modules::AgentTasksListPathPrefix();
}

const char* LimitQueryPrefix()
{
    return memochat::ai::client::modules::LimitQueryPrefix();
}

const char* CancelSuffix()
{
    return memochat::ai::client::modules::CancelSuffix();
}

const char* ResumeSuffix()
{
    return memochat::ai::client::modules::ResumeSuffix();
}

const char* DefaultDeploymentPreference()
{
    return memochat::ai::client::modules::DefaultDeploymentPreference();
}

const char* DefaultApiProviderAdapter()
{
    return memochat::ai::client::modules::DefaultApiProviderAdapter();
}

unsigned long SseDataPrefixSize()
{
    return memochat::ai::client::modules::SseDataPrefixSize();
}

bool IsSseDataLine(const char* line, unsigned long line_size)
{
    return memochat::ai::client::modules::IsSseDataLine(line, line_size);
}

bool IsSseDoneSentinel(const char* text, unsigned long text_size)
{
    return memochat::ai::client::modules::IsSseDoneSentinel(text, text_size);
}

bool ShouldSetJsonBody(bool has_body)
{
    return memochat::ai::client::modules::ShouldSetJsonBody(has_body);
}

bool ShouldAttachMetadata(bool metadata_empty)
{
    return memochat::ai::client::modules::ShouldAttachMetadata(metadata_empty);
}

bool ShouldUseParsedMetadata(bool parse_ok, bool metadata_is_object)
{
    return memochat::ai::client::modules::ShouldUseParsedMetadata(parse_ok, metadata_is_object);
}

bool ShouldKeepUrlByte(bool is_alnum, unsigned char ch)
{
    return memochat::ai::client::modules::ShouldKeepUrlByte(is_alnum, ch);
}

bool ShouldUseDefaultDeploymentPreference(bool preference_empty)
{
    return memochat::ai::client::modules::ShouldUseDefaultDeploymentPreference(preference_empty);
}

bool ShouldUseDefaultApiProviderAdapter(bool adapter_empty)
{
    return memochat::ai::client::modules::ShouldUseDefaultApiProviderAdapter(adapter_empty);
}

bool ShouldUpdateCurrentMessageId(bool msg_id_empty)
{
    return memochat::ai::client::modules::ShouldUpdateCurrentMessageId(msg_id_empty);
}

bool ShouldUpdateTotalTokens(long long tokens)
{
    return memochat::ai::client::modules::ShouldUpdateTotalTokens(tokens);
}

bool ShouldEmitChunk(bool callback_present)
{
    return memochat::ai::client::modules::ShouldEmitChunk(callback_present);
}

bool ShouldStoreFinalChunkResult(bool final_flag, bool out_result_present)
{
    return memochat::ai::client::modules::ShouldStoreFinalChunkResult(final_flag, out_result_present);
}

bool ShouldFallbackStreamResult(bool saw_final, bool out_result_present)
{
    return memochat::ai::client::modules::ShouldFallbackStreamResult(saw_final, out_result_present);
}

bool IsLineTrimChar(char ch)
{
    return memochat::ai::client::modules::IsLineTrimChar(ch);
}
} // namespace memochat::tests::ai::client
