export module memochat.runtime.ini_config_algorithms;

export namespace memochat::runtime::modules
{
char SanitizeEnvTokenChar(unsigned char ch)
{
    if (ch >= static_cast<unsigned char>('a') && ch <= static_cast<unsigned char>('z'))
    {
        return static_cast<char>(ch - static_cast<unsigned char>('a') + static_cast<unsigned char>('A'));
    }
    if ((ch >= static_cast<unsigned char>('A') && ch <= static_cast<unsigned char>('Z')) ||
        (ch >= static_cast<unsigned char>('0') && ch <= static_cast<unsigned char>('9')))
    {
        return static_cast<char>(ch);
    }
    return '_';
}
} // namespace memochat::runtime::modules
