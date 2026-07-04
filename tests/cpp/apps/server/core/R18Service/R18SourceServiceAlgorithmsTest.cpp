#include <gtest/gtest.h>

namespace memochat::tests::r18::source_service
{
unsigned long long ZipMagicMinSize();
bool IsZipMagic(char first, char second, unsigned long long size);
int NormalizeSearchPage(int page);
int PreviewChapterCount();
int PreviewPageCount();
unsigned long long ManifestProbeWindow();
bool MatchesJsSourceProbe(bool has_class, bool has_comic_source, bool has_search);
const char* GateShellPrefix();
const char* MockSourceId();
const char* NativeFormat();
const char* NativeZipFormat();
const char* SourceJsFormat();
const char* OkStatus();
const char* StagedStatus();
const char* StagedJsStatus();
const char* DefaultVersion();
const char* JmSourceVersion();
const char* PicacgSourceVersion();
const char* InvalidPackagePayloadMessage();
const char* InvalidManifestMessage();
const char* SourceIdEmptyMessage();
const char* ReservedSourceIdMessage();
const char* CreateDirFailedMessage();
const char* PersistFailedMessage();
const char* SourceNotFoundMessage();
const char* CannotDeleteBuiltinMessage();
} // namespace memochat::tests::r18::source_service

TEST(R18SourceServiceAlgorithmsTest, PinsZipMagicDetection)
{
    using namespace memochat::tests::r18::source_service;

    EXPECT_EQ(ZipMagicMinSize(), 4u);
    // Real payload check: needs at least 4 bytes AND the first two are 'P','K'.
    EXPECT_TRUE(IsZipMagic('P', 'K', 4));
    EXPECT_TRUE(IsZipMagic('P', 'K', 100));
    EXPECT_FALSE(IsZipMagic('P', 'K', 3));
    EXPECT_FALSE(IsZipMagic('P', 'Q', 100));
    EXPECT_FALSE(IsZipMagic('Q', 'K', 100));
    EXPECT_FALSE(IsZipMagic('\0', '\0', 0));
}

TEST(R18SourceServiceAlgorithmsTest, PinsSearchPageClamp)
{
    using namespace memochat::tests::r18::source_service;

    EXPECT_EQ(NormalizeSearchPage(0), 1);
    EXPECT_EQ(NormalizeSearchPage(-5), 1);
    EXPECT_EQ(NormalizeSearchPage(1), 1);
    EXPECT_EQ(NormalizeSearchPage(7), 7);
}

TEST(R18SourceServiceAlgorithmsTest, PinsPreviewCountsAndProbeWindow)
{
    using namespace memochat::tests::r18::source_service;

    EXPECT_EQ(PreviewChapterCount(), 3);
    EXPECT_EQ(PreviewPageCount(), 5);
    EXPECT_EQ(ManifestProbeWindow(), 4096u);
}

TEST(R18SourceServiceAlgorithmsTest, PinsJsSourceProbeDecision)
{
    using namespace memochat::tests::r18::source_service;

    // "class " plus either "ComicSource" or "search".
    EXPECT_TRUE(MatchesJsSourceProbe(true, true, false));
    EXPECT_TRUE(MatchesJsSourceProbe(true, false, true));
    EXPECT_TRUE(MatchesJsSourceProbe(true, true, true));
    EXPECT_FALSE(MatchesJsSourceProbe(true, false, false));
    EXPECT_FALSE(MatchesJsSourceProbe(false, true, true));
    EXPECT_FALSE(MatchesJsSourceProbe(false, false, false));
}

TEST(R18SourceServiceAlgorithmsTest, PinsSourceLiterals)
{
    using namespace memochat::tests::r18::source_service;

    EXPECT_STREQ(GateShellPrefix(), "gateserver");
    EXPECT_STREQ(MockSourceId(), "mock");
    EXPECT_STREQ(NativeFormat(), "native");
    EXPECT_STREQ(NativeZipFormat(), "native-zip");
    EXPECT_STREQ(SourceJsFormat(), "source-js");
    EXPECT_STREQ(OkStatus(), "ok");
    EXPECT_STREQ(StagedStatus(), "staged");
    EXPECT_STREQ(StagedJsStatus(), "staged-js");
    EXPECT_STREQ(DefaultVersion(), "0.0.0");
    EXPECT_STREQ(JmSourceVersion(), "2.0.16");
    EXPECT_STREQ(PicacgSourceVersion(), "2.2.1");
}

TEST(R18SourceServiceAlgorithmsTest, PinsErrorMessages)
{
    using namespace memochat::tests::r18::source_service;

    EXPECT_STREQ(InvalidPackagePayloadMessage(), "plugin package must be a zip file or JavaScript source");
    EXPECT_STREQ(InvalidManifestMessage(), "manifest_json is invalid");
    EXPECT_STREQ(SourceIdEmptyMessage(), "source id is empty");
    EXPECT_STREQ(ReservedSourceIdMessage(), "source id is reserved for a built-in adapter");
    EXPECT_STREQ(CreateDirFailedMessage(), "failed to create source directory");
    EXPECT_STREQ(PersistFailedMessage(), "failed to persist source package");
    EXPECT_STREQ(SourceNotFoundMessage(), "source not found");
    EXPECT_STREQ(CannotDeleteBuiltinMessage(), "cannot delete a built-in source");
}
