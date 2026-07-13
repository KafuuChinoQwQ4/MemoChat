export module memochat.r18.source_service_algorithms;

// Distinct nested namespace (source_service) so these helpers do not collide
// with memochat::r18::modules exported by memochat.r18.source_algorithms, which
// the same production TU (R18SourceService.cpp) also imports.
export namespace memochat::r18::source_service::modules
{
unsigned long long ZipMagicMinSize()
{
    return 4;
}

bool IsZipMagic(char first, char second, unsigned long long size)
{
    return size >= ZipMagicMinSize() && first == 'P' && second == 'K';
}

unsigned long long MaxSourceImportBytes()
{
    return 1024ULL * 1024ULL;
}

bool ShouldRejectSourceImport(bool zip_payload, bool js_payload, unsigned long long size)
{
    return size == 0 || zip_payload || !js_payload || size > MaxSourceImportBytes();
}

int NormalizeSearchPage(int page)
{
    return page < 1 ? 1 : page;
}

int PreviewChapterCount()
{
    return 3;
}

int PreviewPageCount()
{
    return 5;
}

unsigned long long ManifestProbeWindow()
{
    return 4096;
}

bool MatchesJsSourceProbe(bool has_class, bool has_comic_source, bool has_search)
{
    return has_class && (has_comic_source || has_search);
}

bool ShouldDispatchSource(bool source_found, bool source_enabled)
{
    return source_found && source_enabled;
}

const char* GateShellPrefix()
{
    return "gateserver";
}

const char* MockSourceId()
{
    return "mock";
}

const char* NativeFormat()
{
    return "native";
}

const char* NativeZipFormat()
{
    return "native-zip";
}

const char* SourceJsFormat()
{
    return "source-js";
}

const char* OkStatus()
{
    return "ok";
}

const char* StagedStatus()
{
    return "staged";
}

const char* StagedJsStatus()
{
    return "staged-js";
}

const char* DefaultVersion()
{
    return "0.0.0";
}

const char* JmSourceVersion()
{
    return "2.0.16";
}

const char* PicacgSourceVersion()
{
    return "2.2.1";
}

const char* NhentaiSourceVersion()
{
    return "1.0.0";
}

const char* EhentaiSourceVersion()
{
    return "1.0.0";
}

const char* AuthRequiredStatus()
{
    return "auth-required";
}

const char* AuthRequiredMessage()
{
    return "account login required";
}

const char* DirectAccessStatus()
{
    return "direct";
}

const char* InvalidPackagePayloadMessage()
{
    return "source package must be JavaScript";
}

const char* NativePackageRejectedMessage()
{
    return "native source packages are not supported";
}

const char* SourcePackageTooLargeMessage()
{
    return "source package exceeds the 1 MiB limit";
}

const char* SourceDisabledMessage()
{
    return "source is disabled";
}

const char* InvalidManifestMessage()
{
    return "manifest_json is invalid";
}

const char* SourceIdEmptyMessage()
{
    return "source id is empty";
}

const char* ReservedSourceIdMessage()
{
    return "source id is reserved for a built-in adapter";
}

const char* CreateDirFailedMessage()
{
    return "failed to create source directory";
}

const char* PersistFailedMessage()
{
    return "failed to persist source package";
}

const char* SourceNotFoundMessage()
{
    return "source not found";
}

const char* CannotDeleteBuiltinMessage()
{
    return "cannot delete a built-in source";
}
} // namespace memochat::r18::source_service::modules
