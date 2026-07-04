#include <gtest/gtest.h>

#include <array>
#include <string_view>

namespace memochat::tests::r18::adapter_utils
{
const char* MissingSchemeMessage();
const char* MissingHostMessage();
const char* UnsupportedSchemeMessage();
const char* DefaultTarget();
const char* DefaultHttpsPort();
const char* DefaultHttpPort();
const char* SelectDefaultPort(bool scheme_equals_https);
bool IsHttpsScheme(bool scheme_equals_https);
bool IsHttpScheme(bool scheme_equals_http);
bool ShouldUseDefaultTarget(bool path_missing);
bool ShouldThrowMissingHost(bool host_empty);
const char* DefaultUserAgent();
bool IsUrlEncodeUnreserved(unsigned char ch);
const char* XmlEscapeText(char ch);
bool ShouldSkipEmptyTag(bool tag_empty);
const char* PlaceholderContentType();
const char* DefaultCachedImageContentType();
bool ShouldReadCachedImageBody(bool body_file_open);
bool HasCachedImageBody(bool body_empty);
bool ShouldUseDefaultCachedImageContentType(bool content_type_empty);
unsigned char Base64InvalidMarker();
bool ShouldSkipBase64Whitespace(unsigned char ch);
bool IsBase64Padding(unsigned char ch);
bool HasInvalidBase64Value(unsigned char value, unsigned char invalid_marker);
} // namespace memochat::tests::r18::adapter_utils

TEST(R18AdapterUtilsAlgorithmsTest, ExposesStableUrlDefaultsAndErrors)
{
    using namespace memochat::tests::r18::adapter_utils;

    EXPECT_STREQ(MissingSchemeMessage(), "URL missing scheme");
    EXPECT_STREQ(MissingHostMessage(), "URL missing host");
    EXPECT_STREQ(UnsupportedSchemeMessage(), "unsupported URL scheme");
    EXPECT_STREQ(DefaultTarget(), "/");
    EXPECT_STREQ(DefaultHttpsPort(), "443");
    EXPECT_STREQ(DefaultHttpPort(), "80");
    EXPECT_STREQ(SelectDefaultPort(true), "443");
    EXPECT_STREQ(SelectDefaultPort(false), "80");
    EXPECT_TRUE(IsHttpsScheme(true));
    EXPECT_FALSE(IsHttpsScheme(false));
    EXPECT_TRUE(IsHttpScheme(true));
    EXPECT_FALSE(IsHttpScheme(false));
    EXPECT_TRUE(ShouldUseDefaultTarget(true));
    EXPECT_FALSE(ShouldUseDefaultTarget(false));
    EXPECT_TRUE(ShouldThrowMissingHost(true));
    EXPECT_FALSE(ShouldThrowMissingHost(false));
}

TEST(R18AdapterUtilsAlgorithmsTest, ExposesUserAgentAndEncodingGuards)
{
    using namespace memochat::tests::r18::adapter_utils;

    EXPECT_NE(std::string_view(DefaultUserAgent()).find("Mobile Safari/537.36"), std::string_view::npos);
    for (const unsigned char ch : std::array<unsigned char, 8>{'A', 'z', '0', '9', '-', '_', '.', '~'})
        EXPECT_TRUE(IsUrlEncodeUnreserved(ch));
    for (const unsigned char ch : std::array<unsigned char, 5>{' ', '%', '/', '?', '&'})
        EXPECT_FALSE(IsUrlEncodeUnreserved(ch));
}

TEST(R18AdapterUtilsAlgorithmsTest, ExposesXmlAndPayloadDefaults)
{
    using namespace memochat::tests::r18::adapter_utils;

    EXPECT_STREQ(XmlEscapeText('&'), "&amp;");
    EXPECT_STREQ(XmlEscapeText('<'), "&lt;");
    EXPECT_STREQ(XmlEscapeText('>'), "&gt;");
    EXPECT_STREQ(XmlEscapeText('"'), "&quot;");
    EXPECT_STREQ(XmlEscapeText('\''), "&apos;");
    EXPECT_EQ(XmlEscapeText('x'), nullptr);

    EXPECT_TRUE(ShouldSkipEmptyTag(true));
    EXPECT_FALSE(ShouldSkipEmptyTag(false));
    EXPECT_STREQ(PlaceholderContentType(), "image/svg+xml");
    EXPECT_STREQ(DefaultCachedImageContentType(), "image/jpeg");
    EXPECT_TRUE(ShouldReadCachedImageBody(true));
    EXPECT_FALSE(ShouldReadCachedImageBody(false));
    EXPECT_FALSE(HasCachedImageBody(true));
    EXPECT_TRUE(HasCachedImageBody(false));
    EXPECT_TRUE(ShouldUseDefaultCachedImageContentType(true));
    EXPECT_FALSE(ShouldUseDefaultCachedImageContentType(false));
}

TEST(R18AdapterUtilsAlgorithmsTest, ExposesBase64PrimitiveGuards)
{
    using namespace memochat::tests::r18::adapter_utils;

    const unsigned char invalid = Base64InvalidMarker();
    EXPECT_EQ(invalid, static_cast<unsigned char>(255));
    EXPECT_TRUE(ShouldSkipBase64Whitespace(static_cast<unsigned char>(' ')));
    EXPECT_TRUE(ShouldSkipBase64Whitespace(static_cast<unsigned char>('\n')));
    EXPECT_FALSE(ShouldSkipBase64Whitespace(static_cast<unsigned char>('A')));
    EXPECT_TRUE(IsBase64Padding(static_cast<unsigned char>('=')));
    EXPECT_FALSE(IsBase64Padding(static_cast<unsigned char>('A')));
    EXPECT_TRUE(HasInvalidBase64Value(invalid, invalid));
    EXPECT_FALSE(HasInvalidBase64Value(static_cast<unsigned char>(63), invalid));
}
