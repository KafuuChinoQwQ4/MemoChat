import memochat.r18.adapter_utils_algorithms;

namespace memochat::tests::r18::adapter_utils
{
const char* MissingSchemeMessage()
{
    return memochat::r18::adapter_utils::modules::MissingSchemeMessage();
}

const char* MissingHostMessage()
{
    return memochat::r18::adapter_utils::modules::MissingHostMessage();
}

const char* UnsupportedSchemeMessage()
{
    return memochat::r18::adapter_utils::modules::UnsupportedSchemeMessage();
}

const char* DefaultTarget()
{
    return memochat::r18::adapter_utils::modules::DefaultTarget();
}

const char* DefaultHttpsPort()
{
    return memochat::r18::adapter_utils::modules::DefaultHttpsPort();
}

const char* DefaultHttpPort()
{
    return memochat::r18::adapter_utils::modules::DefaultHttpPort();
}

const char* SelectDefaultPort(bool scheme_equals_https)
{
    return memochat::r18::adapter_utils::modules::SelectDefaultPort(scheme_equals_https);
}

bool IsHttpsScheme(bool scheme_equals_https)
{
    return memochat::r18::adapter_utils::modules::IsHttpsScheme(scheme_equals_https);
}

bool IsHttpScheme(bool scheme_equals_http)
{
    return memochat::r18::adapter_utils::modules::IsHttpScheme(scheme_equals_http);
}

bool ShouldUseDefaultTarget(bool path_missing)
{
    return memochat::r18::adapter_utils::modules::ShouldUseDefaultTarget(path_missing);
}

bool ShouldThrowMissingHost(bool host_empty)
{
    return memochat::r18::adapter_utils::modules::ShouldThrowMissingHost(host_empty);
}

const char* DefaultUserAgent()
{
    return memochat::r18::adapter_utils::modules::DefaultUserAgent();
}

bool IsUrlEncodeUnreserved(unsigned char ch)
{
    return memochat::r18::adapter_utils::modules::IsUrlEncodeUnreserved(ch);
}

const char* XmlEscapeText(char ch)
{
    return memochat::r18::adapter_utils::modules::XmlEscapeText(ch);
}

bool ShouldSkipEmptyTag(bool tag_empty)
{
    return memochat::r18::adapter_utils::modules::ShouldSkipEmptyTag(tag_empty);
}

const char* PlaceholderContentType()
{
    return memochat::r18::adapter_utils::modules::PlaceholderContentType();
}

const char* DefaultCachedImageContentType()
{
    return memochat::r18::adapter_utils::modules::DefaultCachedImageContentType();
}

bool ShouldReadCachedImageBody(bool body_file_open)
{
    return memochat::r18::adapter_utils::modules::ShouldReadCachedImageBody(body_file_open);
}

bool HasCachedImageBody(bool body_empty)
{
    return memochat::r18::adapter_utils::modules::HasCachedImageBody(body_empty);
}

bool ShouldUseDefaultCachedImageContentType(bool content_type_empty)
{
    return memochat::r18::adapter_utils::modules::ShouldUseDefaultCachedImageContentType(content_type_empty);
}

unsigned char Base64InvalidMarker()
{
    return memochat::r18::adapter_utils::modules::Base64InvalidMarker();
}

bool ShouldSkipBase64Whitespace(unsigned char ch)
{
    return memochat::r18::adapter_utils::modules::ShouldSkipBase64Whitespace(ch);
}

bool IsBase64Padding(unsigned char ch)
{
    return memochat::r18::adapter_utils::modules::IsBase64Padding(ch);
}

bool HasInvalidBase64Value(unsigned char value, unsigned char invalid_marker)
{
    return memochat::r18::adapter_utils::modules::HasInvalidBase64Value(value, invalid_marker);
}
} // namespace memochat::tests::r18::adapter_utils
