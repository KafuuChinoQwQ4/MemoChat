export module memochat.gate.routing_algorithms;

export namespace memochat::gate::routing::modules
{
void NormalizeMethodAscii(char* data, unsigned long long size)
{
    for (unsigned long long index = 0; index < size; ++index)
    {
        const auto ch = static_cast<unsigned char>(data[index]);
        if (ch >= static_cast<unsigned char>('a') && ch <= static_cast<unsigned char>('z'))
        {
            data[index] = static_cast<char>(ch - static_cast<unsigned char>('a') + static_cast<unsigned char>('A'));
        }
    }
}

bool MatchesRoutePrefix(const char* path,
                        unsigned long long path_size,
                        const char* prefix,
                        unsigned long long prefix_size)
{
    if (path_size == prefix_size)
    {
        for (unsigned long long index = 0; index < prefix_size; ++index)
        {
            if (path[index] != prefix[index])
            {
                return false;
            }
        }
        return true;
    }

    if (path_size <= prefix_size)
    {
        return false;
    }

    for (unsigned long long index = 0; index < prefix_size; ++index)
    {
        if (path[index] != prefix[index])
        {
            return false;
        }
    }

    const char next = path[prefix_size];
    return next == '/' || next == '?';
}
} // namespace memochat::gate::routing::modules
