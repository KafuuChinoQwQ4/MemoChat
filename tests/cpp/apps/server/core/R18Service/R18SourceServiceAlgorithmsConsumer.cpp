import memochat.r18.source_service_algorithms;

namespace memochat::tests::r18::source_service
{
unsigned long long ZipMagicMinSize()
{
    return memochat::r18::source_service::modules::ZipMagicMinSize();
}

bool IsZipMagic(char first, char second, unsigned long long size)
{
    return memochat::r18::source_service::modules::IsZipMagic(first, second, size);
}

unsigned long long MaxSourceImportBytes()
{
    return memochat::r18::source_service::modules::MaxSourceImportBytes();
}

bool ShouldRejectSourceImport(bool zip_payload, bool js_payload, unsigned long long size)
{
    return memochat::r18::source_service::modules::ShouldRejectSourceImport(zip_payload, js_payload, size);
}

int NormalizeSearchPage(int page)
{
    return memochat::r18::source_service::modules::NormalizeSearchPage(page);
}

int PreviewChapterCount()
{
    return memochat::r18::source_service::modules::PreviewChapterCount();
}

int PreviewPageCount()
{
    return memochat::r18::source_service::modules::PreviewPageCount();
}

unsigned long long ManifestProbeWindow()
{
    return memochat::r18::source_service::modules::ManifestProbeWindow();
}

bool MatchesJsSourceProbe(bool has_class, bool has_comic_source, bool has_search)
{
    return memochat::r18::source_service::modules::MatchesJsSourceProbe(has_class, has_comic_source, has_search);
}

bool ShouldDispatchSource(bool source_found, bool source_enabled)
{
    return memochat::r18::source_service::modules::ShouldDispatchSource(source_found, source_enabled);
}

const char* GateShellPrefix()
{
    return memochat::r18::source_service::modules::GateShellPrefix();
}

const char* MockSourceId()
{
    return memochat::r18::source_service::modules::MockSourceId();
}

const char* NativeFormat()
{
    return memochat::r18::source_service::modules::NativeFormat();
}

const char* NativeZipFormat()
{
    return memochat::r18::source_service::modules::NativeZipFormat();
}

const char* SourceJsFormat()
{
    return memochat::r18::source_service::modules::SourceJsFormat();
}

const char* OkStatus()
{
    return memochat::r18::source_service::modules::OkStatus();
}

const char* StagedStatus()
{
    return memochat::r18::source_service::modules::StagedStatus();
}

const char* StagedJsStatus()
{
    return memochat::r18::source_service::modules::StagedJsStatus();
}

const char* DefaultVersion()
{
    return memochat::r18::source_service::modules::DefaultVersion();
}

const char* JmSourceVersion()
{
    return memochat::r18::source_service::modules::JmSourceVersion();
}

const char* PicacgSourceVersion()
{
    return memochat::r18::source_service::modules::PicacgSourceVersion();
}

const char* InvalidPackagePayloadMessage()
{
    return memochat::r18::source_service::modules::InvalidPackagePayloadMessage();
}

const char* NativePackageRejectedMessage()
{
    return memochat::r18::source_service::modules::NativePackageRejectedMessage();
}

const char* SourcePackageTooLargeMessage()
{
    return memochat::r18::source_service::modules::SourcePackageTooLargeMessage();
}

const char* SourceDisabledMessage()
{
    return memochat::r18::source_service::modules::SourceDisabledMessage();
}

const char* InvalidManifestMessage()
{
    return memochat::r18::source_service::modules::InvalidManifestMessage();
}

const char* SourceIdEmptyMessage()
{
    return memochat::r18::source_service::modules::SourceIdEmptyMessage();
}

const char* ReservedSourceIdMessage()
{
    return memochat::r18::source_service::modules::ReservedSourceIdMessage();
}

const char* CreateDirFailedMessage()
{
    return memochat::r18::source_service::modules::CreateDirFailedMessage();
}

const char* PersistFailedMessage()
{
    return memochat::r18::source_service::modules::PersistFailedMessage();
}

const char* SourceNotFoundMessage()
{
    return memochat::r18::source_service::modules::SourceNotFoundMessage();
}

const char* CannotDeleteBuiltinMessage()
{
    return memochat::r18::source_service::modules::CannotDeleteBuiltinMessage();
}
} // namespace memochat::tests::r18::source_service
