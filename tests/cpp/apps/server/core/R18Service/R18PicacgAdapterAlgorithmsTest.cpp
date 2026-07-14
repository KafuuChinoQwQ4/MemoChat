#include <gtest/gtest.h>

#include <array>
#include <cstdlib>
#include <cstdint>
#include <string>
#include <string_view>

namespace memochat::tests::r18::picacg_adapter
{
const char* SourceId();
const char* ApiHost();
const char* ApiKey();
const char* HmacKey();
bool HasCredentials(bool api_key_empty, bool hmac_key_empty);
bool HasValidSigningMaterial(unsigned long long api_key_size, unsigned long long hmac_key_size);
const char* MissingCredentialsMessage();
const char* GetMethod();
const char* PostMethod();
int ApiTimeoutSeconds();
int NormalizeSearchPage(int page);
bool ShouldStripLeadingSlash(bool path_empty, bool starts_with_slash);
int SuccessStatusCode();
bool IsSuccessStatus(int status);
bool IsSuccessfulHttpRange(int status);
const char* InvalidJsonResponseMessage();
const char* SearchInvalidJsonMessage();
const char* DefaultEpisodeTitle();
int DefaultEpisodeOrder();
bool ShouldUseFallbackEpisode(bool has_no_episodes);
const char* ImageReferer();
const char* AllowedImageHostsConfigSection();
const char* AllowedImageHostsConfigKey();
const char* ImageTargetPrefix();
const char* RedirectImageHost();
bool IsRedirectImageTarget(const char* target, unsigned long long target_size);
bool IsExactHostInPolicy(const char* host,
                         unsigned long long host_size,
                         const char* policy,
                         unsigned long long policy_size);
bool IsCanonicalAllowedImageUrl(bool scheme_is_https,
                                bool port_is_443,
                                bool has_userinfo,
                                bool has_fragment,
                                bool host_allowed,
                                bool target_allowed);
bool IsPublicIpv4Address(unsigned int address);
bool IsPublicIpv6Address(const unsigned char* bytes, unsigned long long size);
unsigned long long MaxImageBytes();
bool IsAllowedImageContentType(bool jpeg, bool png, bool webp, bool gif, bool avif);
bool IsImageRedirectStatus(int status);
unsigned long long MaxImageRedirects();
const char* ImageUnavailableTitle();
const char* ImageErrorTitle();
bool ShouldUseImagePlaceholder(int status, bool body_empty);
const char* DefaultSearchSort();
const char* NormalizeSearchSort(const char* sort);
} // namespace memochat::tests::r18::picacg_adapter

TEST(R18PicacgAdapterAlgorithmsTest, ExposesStableIdentityAndApiDefaults)
{
    using namespace memochat::tests::r18::picacg_adapter;

    const std::string api_key(29, 'a');
    const std::string hmac_key(63, 'b');
    setenv("MEMOCHAT_R18_PICACG_API_KEY", api_key.c_str(), 1);
    setenv("MEMOCHAT_R18_PICACG_HMAC_KEY", hmac_key.c_str(), 1);

    EXPECT_STREQ(SourceId(), "picacg.official");
    EXPECT_STREQ(ApiHost(), "picaapi.picacomic.com");
    // The test injects canonical-length signing material through the runtime environment.
    EXPECT_FALSE(std::string(ApiKey()).empty());
    EXPECT_FALSE(std::string(HmacKey()).empty());
    EXPECT_FALSE(HasCredentials(true, false));
    EXPECT_FALSE(HasCredentials(false, true));
    EXPECT_TRUE(HasCredentials(false, false));
    EXPECT_FALSE(HasValidSigningMaterial(29, 32));
    EXPECT_FALSE(HasValidSigningMaterial(28, 63));
    EXPECT_TRUE(HasValidSigningMaterial(29, 63));
    EXPECT_STREQ(MissingCredentialsMessage(), "Picacg signing configuration is missing or invalid");
    EXPECT_STREQ(GetMethod(), "GET");
    EXPECT_STREQ(PostMethod(), "POST");
    EXPECT_EQ(ApiTimeoutSeconds(), 8);
}

namespace
{
constexpr std::uint32_t Ipv4(unsigned int a, unsigned int b, unsigned int c, unsigned int d)
{
    return (a << 24U) | (b << 16U) | (c << 8U) | d;
}
} // namespace

TEST(R18PicacgAdapterAlgorithmsTest, ExposesPathStatusAndJsonGuards)
{
    using namespace memochat::tests::r18::picacg_adapter;

    EXPECT_EQ(NormalizeSearchPage(-4), 1);
    EXPECT_EQ(NormalizeSearchPage(0), 1);
    EXPECT_EQ(NormalizeSearchPage(5), 5);
    EXPECT_FALSE(ShouldStripLeadingSlash(true, false));
    EXPECT_FALSE(ShouldStripLeadingSlash(false, false));
    EXPECT_TRUE(ShouldStripLeadingSlash(false, true));

    EXPECT_EQ(SuccessStatusCode(), 200);
    EXPECT_TRUE(IsSuccessStatus(200));
    EXPECT_FALSE(IsSuccessStatus(201));
    EXPECT_TRUE(IsSuccessfulHttpRange(200));
    EXPECT_TRUE(IsSuccessfulHttpRange(299));
    EXPECT_FALSE(IsSuccessfulHttpRange(199));
    EXPECT_FALSE(IsSuccessfulHttpRange(300));
    EXPECT_STREQ(InvalidJsonResponseMessage(), "Picacg invalid JSON response");
    EXPECT_STREQ(SearchInvalidJsonMessage(), "Picacg search invalid JSON");
}

TEST(R18PicacgAdapterAlgorithmsTest, ExposesEpisodeFallbackDefaults)
{
    using namespace memochat::tests::r18::picacg_adapter;

    EXPECT_STREQ(DefaultEpisodeTitle(), "Episode 1");
    EXPECT_EQ(DefaultEpisodeOrder(), 1);
    EXPECT_TRUE(ShouldUseFallbackEpisode(true));
    EXPECT_FALSE(ShouldUseFallbackEpisode(false));
}

TEST(R18PicacgAdapterAlgorithmsTest, ExposesImageFetchGuards)
{
    using namespace memochat::tests::r18::picacg_adapter;

    EXPECT_STREQ(ImageReferer(), "https://manhuabika.com/");
    EXPECT_STREQ(ImageUnavailableTitle(), "Picacg image unavailable");
    EXPECT_STREQ(ImageErrorTitle(), "Picacg image error");
    EXPECT_FALSE(ShouldUseImagePlaceholder(200, false));
    EXPECT_TRUE(ShouldUseImagePlaceholder(199, false));
    EXPECT_TRUE(ShouldUseImagePlaceholder(300, false));
    EXPECT_TRUE(ShouldUseImagePlaceholder(200, true));
    EXPECT_TRUE(IsImageRedirectStatus(301));
    EXPECT_TRUE(IsImageRedirectStatus(302));
    EXPECT_TRUE(IsImageRedirectStatus(307));
    EXPECT_TRUE(IsImageRedirectStatus(308));
    EXPECT_FALSE(IsImageRedirectStatus(200));
    EXPECT_FALSE(IsImageRedirectStatus(404));
    EXPECT_EQ(MaxImageRedirects(), 2U);
}

TEST(R18PicacgAdapterAlgorithmsTest, RequiresCanonicalExactConfiguredMediaOrigin)
{
    using namespace memochat::tests::r18::picacg_adapter;

    const std::string policy = "cdn-one.example, cdn-two.example";
    const std::string exact = "cdn-two.example";
    const std::string suffix_trick = "cdn-two.example.attacker.invalid";
    EXPECT_STREQ(AllowedImageHostsConfigSection(), "R18Picacg");
    EXPECT_STREQ(AllowedImageHostsConfigKey(), "AllowedImageHosts");
    EXPECT_STREQ(ImageTargetPrefix(), "/static/");
    EXPECT_STREQ(RedirectImageHost(), "img.picacomic.com");
    const std::string redirect_image = "/asset/rs:fill:300:400:0/g:sm/cover.jpg";
    const std::string redirect_html = "/asset/index.html";
    const std::string redirect_query = "/asset/cover.jpg?next=https://attacker.invalid";
    EXPECT_TRUE(IsRedirectImageTarget(redirect_image.data(), redirect_image.size()));
    EXPECT_FALSE(IsRedirectImageTarget(redirect_html.data(), redirect_html.size()));
    EXPECT_FALSE(IsRedirectImageTarget(redirect_query.data(), redirect_query.size()));
    EXPECT_FALSE(IsRedirectImageTarget("/", 1));
    EXPECT_TRUE(IsExactHostInPolicy(exact.data(), exact.size(), policy.data(), policy.size()));
    EXPECT_FALSE(IsExactHostInPolicy(suffix_trick.data(), suffix_trick.size(), policy.data(), policy.size()));
    EXPECT_FALSE(IsExactHostInPolicy(exact.data(), exact.size(), "", 0));

    EXPECT_TRUE(IsCanonicalAllowedImageUrl(true, true, false, false, true, true));
    EXPECT_FALSE(IsCanonicalAllowedImageUrl(false, true, false, false, true, true));
    EXPECT_FALSE(IsCanonicalAllowedImageUrl(true, false, false, false, true, true));
    EXPECT_FALSE(IsCanonicalAllowedImageUrl(true, true, true, false, true, true));
    EXPECT_FALSE(IsCanonicalAllowedImageUrl(true, true, false, true, true, true));
    EXPECT_FALSE(IsCanonicalAllowedImageUrl(true, true, false, false, false, true));
    EXPECT_FALSE(IsCanonicalAllowedImageUrl(true, true, false, false, true, false));
}

TEST(R18PicacgAdapterAlgorithmsTest, RejectsNonPublicResolvedAddresses)
{
    using namespace memochat::tests::r18::picacg_adapter;

    EXPECT_TRUE(IsPublicIpv4Address(Ipv4(8, 8, 8, 8)));
    EXPECT_FALSE(IsPublicIpv4Address(Ipv4(0, 0, 0, 0)));
    EXPECT_FALSE(IsPublicIpv4Address(Ipv4(10, 1, 2, 3)));
    EXPECT_FALSE(IsPublicIpv4Address(Ipv4(100, 64, 0, 1)));
    EXPECT_FALSE(IsPublicIpv4Address(Ipv4(127, 0, 0, 1)));
    EXPECT_FALSE(IsPublicIpv4Address(Ipv4(169, 254, 169, 254)));
    EXPECT_FALSE(IsPublicIpv4Address(Ipv4(172, 16, 0, 1)));
    EXPECT_FALSE(IsPublicIpv4Address(Ipv4(192, 168, 1, 1)));
    EXPECT_FALSE(IsPublicIpv4Address(Ipv4(198, 18, 0, 1)));
    EXPECT_FALSE(IsPublicIpv4Address(Ipv4(203, 0, 113, 1)));
    EXPECT_FALSE(IsPublicIpv4Address(Ipv4(224, 0, 0, 1)));
    EXPECT_FALSE(IsPublicIpv4Address(Ipv4(255, 255, 255, 255)));

    const std::array<unsigned char, 16> public_v6{0x26, 0x06, 0x47, 0x00};
    const std::array<unsigned char, 16> loopback_v6{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
    const std::array<unsigned char, 16> unique_local_v6{0xfc, 0};
    const std::array<unsigned char, 16> link_local_v6{0xfe, 0x80};
    const std::array<unsigned char, 16> multicast_v6{0xff, 0x02};
    const std::array<unsigned char, 16> documentation_v6{0x20, 0x01, 0x0d, 0xb8};
    EXPECT_TRUE(IsPublicIpv6Address(public_v6.data(), public_v6.size()));
    EXPECT_FALSE(IsPublicIpv6Address(loopback_v6.data(), loopback_v6.size()));
    EXPECT_FALSE(IsPublicIpv6Address(unique_local_v6.data(), unique_local_v6.size()));
    EXPECT_FALSE(IsPublicIpv6Address(link_local_v6.data(), link_local_v6.size()));
    EXPECT_FALSE(IsPublicIpv6Address(multicast_v6.data(), multicast_v6.size()));
    EXPECT_FALSE(IsPublicIpv6Address(documentation_v6.data(), documentation_v6.size()));
}

TEST(R18PicacgAdapterAlgorithmsTest, BoundsAndTypesImageResponses)
{
    using namespace memochat::tests::r18::picacg_adapter;

    EXPECT_EQ(MaxImageBytes(), 16u * 1024u * 1024u);
    EXPECT_TRUE(IsAllowedImageContentType(true, false, false, false, false));
    EXPECT_TRUE(IsAllowedImageContentType(false, true, false, false, false));
    EXPECT_TRUE(IsAllowedImageContentType(false, false, true, false, false));
    EXPECT_TRUE(IsAllowedImageContentType(false, false, false, true, false));
    EXPECT_TRUE(IsAllowedImageContentType(false, false, false, false, true));
    EXPECT_FALSE(IsAllowedImageContentType(false, false, false, false, false));
}

TEST(R18PicacgAdapterAlgorithmsTest, NormalizesSourceNativeSearchSort)
{
    using namespace memochat::tests::r18::picacg_adapter;

    EXPECT_STREQ(DefaultSearchSort(), "dd");
    EXPECT_STREQ(NormalizeSearchSort(""), "dd");
    EXPECT_STREQ(NormalizeSearchSort("latest"), "dd");
    EXPECT_STREQ(NormalizeSearchSort("oldest"), "da");
    EXPECT_STREQ(NormalizeSearchSort("likes"), "ld");
    EXPECT_STREQ(NormalizeSearchSort("popular"), "vd");
    EXPECT_STREQ(NormalizeSearchSort("nope"), "dd");
}
