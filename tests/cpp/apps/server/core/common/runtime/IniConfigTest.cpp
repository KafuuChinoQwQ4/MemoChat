#include "runtime/IniConfig.hpp"

#include <gtest/gtest.h>

namespace memochat::tests::runtime
{
bool HasMultipleEtcdEndpoints(bool comma_found);
unsigned long long EtcdSchemeDelimiterSize();
bool HasEtcdSchemeDelimiter(bool delimiter_found);
bool HasExplicitEtcdPort(bool colon_found);
const char* DefaultEtcdPort();
bool IsEtcdResponseWithKvs(bool kvs_token_found);
bool IsEtcdResponseWithHeader(bool header_token_found);
bool ShouldAttachEtcdLease(bool positive_ttl);
bool ShouldStartEtcdWatchThread(bool watch_thread_joinable);
bool ShouldSkipEtcdConfigWatchStart(bool already_watching);
bool ShouldAcceptEtcdConfigChangeKey(bool section_empty, bool key_empty);
bool HasEtcdConfigPrefix(bool key_starts_with_prefix);
bool HasEtcdConfigSectionSeparator(bool slash_found);
bool ShouldCreateEtcdConfig(bool endpoints_empty);
unsigned long long NormalizeIoContextPoolSize(unsigned long long requested_size);
bool ShouldWrapNextIoContextIndex(unsigned long long next_index, unsigned long long pool_size);
bool ShouldStopIoContextPool(bool already_stopped);
bool ShouldJoinIoContextThread(bool joinable);
} // namespace memochat::tests::runtime

TEST(IniConfigTest, EnvKeyForSanitizesSectionAndKeyThroughRuntimeModule)
{
    EXPECT_EQ(memochat::runtime::IniConfig::EnvKeyFor("SelfServer", "Name"), "MEMOCHAT_SELFSERVER_NAME");
    EXPECT_EQ(memochat::runtime::IniConfig::EnvKeyFor("cluster-mode", "etcd.endpoints"),
              "MEMOCHAT_CLUSTER_MODE_ETCD_ENDPOINTS");
    EXPECT_EQ(memochat::runtime::IniConfig::EnvKeyFor("ai2", "model_1"), "MEMOCHAT_AI2_MODEL_1");
}

TEST(IoContextPoolAlgorithmsTest, DefinesPoolSizeWrapAndShutdownGuards)
{
    EXPECT_EQ(memochat::tests::runtime::NormalizeIoContextPoolSize(0), 1);
    EXPECT_EQ(memochat::tests::runtime::NormalizeIoContextPoolSize(4), 4);
    EXPECT_FALSE(memochat::tests::runtime::ShouldWrapNextIoContextIndex(0, 3));
    EXPECT_FALSE(memochat::tests::runtime::ShouldWrapNextIoContextIndex(2, 3));
    EXPECT_TRUE(memochat::tests::runtime::ShouldWrapNextIoContextIndex(3, 3));
    EXPECT_FALSE(memochat::tests::runtime::ShouldWrapNextIoContextIndex(0, 0));
    EXPECT_TRUE(memochat::tests::runtime::ShouldStopIoContextPool(false));
    EXPECT_FALSE(memochat::tests::runtime::ShouldStopIoContextPool(true));
    EXPECT_TRUE(memochat::tests::runtime::ShouldJoinIoContextThread(true));
    EXPECT_FALSE(memochat::tests::runtime::ShouldJoinIoContextThread(false));
}

TEST(EtcdConfigAlgorithmsTest, DefinesEndpointResponseAndWatchGuards)
{
    EXPECT_TRUE(memochat::tests::runtime::HasMultipleEtcdEndpoints(true));
    EXPECT_FALSE(memochat::tests::runtime::HasMultipleEtcdEndpoints(false));
    EXPECT_EQ(memochat::tests::runtime::EtcdSchemeDelimiterSize(), 3);
    EXPECT_TRUE(memochat::tests::runtime::HasEtcdSchemeDelimiter(true));
    EXPECT_FALSE(memochat::tests::runtime::HasEtcdSchemeDelimiter(false));
    EXPECT_TRUE(memochat::tests::runtime::HasExplicitEtcdPort(true));
    EXPECT_FALSE(memochat::tests::runtime::HasExplicitEtcdPort(false));
    EXPECT_STREQ(memochat::tests::runtime::DefaultEtcdPort(), "2379");
    EXPECT_TRUE(memochat::tests::runtime::IsEtcdResponseWithKvs(true));
    EXPECT_FALSE(memochat::tests::runtime::IsEtcdResponseWithKvs(false));
    EXPECT_TRUE(memochat::tests::runtime::IsEtcdResponseWithHeader(true));
    EXPECT_FALSE(memochat::tests::runtime::IsEtcdResponseWithHeader(false));
    EXPECT_TRUE(memochat::tests::runtime::ShouldAttachEtcdLease(true));
    EXPECT_FALSE(memochat::tests::runtime::ShouldAttachEtcdLease(false));
    EXPECT_TRUE(memochat::tests::runtime::ShouldStartEtcdWatchThread(false));
    EXPECT_FALSE(memochat::tests::runtime::ShouldStartEtcdWatchThread(true));
    EXPECT_TRUE(memochat::tests::runtime::ShouldSkipEtcdConfigWatchStart(true));
    EXPECT_FALSE(memochat::tests::runtime::ShouldSkipEtcdConfigWatchStart(false));
    EXPECT_TRUE(memochat::tests::runtime::ShouldAcceptEtcdConfigChangeKey(false, false));
    EXPECT_FALSE(memochat::tests::runtime::ShouldAcceptEtcdConfigChangeKey(true, false));
    EXPECT_FALSE(memochat::tests::runtime::ShouldAcceptEtcdConfigChangeKey(false, true));
    EXPECT_TRUE(memochat::tests::runtime::HasEtcdConfigPrefix(true));
    EXPECT_FALSE(memochat::tests::runtime::HasEtcdConfigPrefix(false));
    EXPECT_TRUE(memochat::tests::runtime::HasEtcdConfigSectionSeparator(true));
    EXPECT_FALSE(memochat::tests::runtime::HasEtcdConfigSectionSeparator(false));
    EXPECT_TRUE(memochat::tests::runtime::ShouldCreateEtcdConfig(false));
    EXPECT_FALSE(memochat::tests::runtime::ShouldCreateEtcdConfig(true));
}
