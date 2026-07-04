export module memochat.r18.adapter_utils_algorithms;

export namespace memochat::r18::adapter_utils::modules
{
const char* MissingSchemeMessage()
{
    return "URL missing scheme";
}

const char* MissingHostMessage()
{
    return "URL missing host";
}

const char* UnsupportedSchemeMessage()
{
    return "unsupported URL scheme";
}

const char* DefaultTarget()
{
    return "/";
}

const char* DefaultHttpsPort()
{
    return "443";
}

const char* DefaultHttpPort()
{
    return "80";
}

const char* SelectDefaultPort(bool scheme_equals_https)
{
    return scheme_equals_https ? DefaultHttpsPort() : DefaultHttpPort();
}

bool IsHttpsScheme(bool scheme_equals_https)
{
    return scheme_equals_https;
}

bool IsHttpScheme(bool scheme_equals_http)
{
    return scheme_equals_http;
}

bool ShouldUseDefaultTarget(bool path_missing)
{
    return path_missing;
}

bool ShouldThrowMissingHost(bool host_empty)
{
    return host_empty;
}

const char* DefaultUserAgent()
{
    return "Mozilla/5.0 (Linux; Android 10; K; wv) AppleWebKit/537.36 "
           "(KHTML, like Gecko) Version/4.0 Chrome/130.0.0.0 Mobile Safari/537.36";
}

bool IsAsciiAlphaNum(unsigned char ch)
{
    return (ch >= static_cast<unsigned char>('0') && ch <= static_cast<unsigned char>('9')) ||
           (ch >= static_cast<unsigned char>('A') && ch <= static_cast<unsigned char>('Z')) ||
           (ch >= static_cast<unsigned char>('a') && ch <= static_cast<unsigned char>('z'));
}

bool IsUrlEncodeUnreserved(unsigned char ch)
{
    return IsAsciiAlphaNum(ch) || ch == static_cast<unsigned char>('-') || ch == static_cast<unsigned char>('_') ||
           ch == static_cast<unsigned char>('.') || ch == static_cast<unsigned char>('~');
}

const char* XmlEscapeText(char ch)
{
    switch (ch)
    {
        case '&':
            return "&amp;";
        case '<':
            return "&lt;";
        case '>':
            return "&gt;";
        case '"':
            return "&quot;";
        case '\'':
            return "&apos;";
        default:
            return nullptr;
    }
}

bool ShouldSkipEmptyTag(bool tag_empty)
{
    return tag_empty;
}

const char* PlaceholderContentType()
{
    return "image/svg+xml";
}

const char* DefaultCachedImageContentType()
{
    return "image/jpeg";
}

bool ShouldReadCachedImageBody(bool body_file_open)
{
    return body_file_open;
}

bool HasCachedImageBody(bool body_empty)
{
    return !body_empty;
}

bool ShouldUseDefaultCachedImageContentType(bool content_type_empty)
{
    return content_type_empty;
}

unsigned char Base64InvalidMarker()
{
    return static_cast<unsigned char>(255);
}

bool ShouldSkipBase64Whitespace(unsigned char ch)
{
    return ch == static_cast<unsigned char>(' ') || ch == static_cast<unsigned char>('\t') ||
           ch == static_cast<unsigned char>('\n') || ch == static_cast<unsigned char>('\r') ||
           ch == static_cast<unsigned char>('\f') || ch == static_cast<unsigned char>('\v');
}

bool IsBase64Padding(unsigned char ch)
{
    return ch == static_cast<unsigned char>('=');
}

bool HasInvalidBase64Value(unsigned char value, unsigned char invalid_marker)
{
    return value == invalid_marker;
}
} // namespace memochat::r18::adapter_utils::modules
