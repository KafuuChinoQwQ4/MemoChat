export module memochat.ai.route_module_algorithms;

export namespace memochat::gate::modules::ai::route_algorithms
{
int AtLeast(int value, int minimum)
{
    return value < minimum ? minimum : value;
}

bool StartsWith(const char* value, unsigned long long value_size, const char* prefix, unsigned long long prefix_size)
{
    if (prefix_size == 0)
    {
        return true;
    }
    if (value == nullptr || prefix == nullptr || value_size < prefix_size)
    {
        return false;
    }
    for (unsigned long long index = 0; index < prefix_size; ++index)
    {
        if (value[index] != prefix[index])
        {
            return false;
        }
    }
    return true;
}
} // namespace memochat::gate::modules::ai::route_algorithms
