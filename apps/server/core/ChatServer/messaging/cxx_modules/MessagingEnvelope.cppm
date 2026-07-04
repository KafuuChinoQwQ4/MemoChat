export module memochat.chat.messaging_envelope_algorithms;

export namespace memochat::chat::messaging::envelope_modules
{
int AtLeast(int value, int minimum)
{
    return value < minimum ? minimum : value;
}

int NonNegative(int value)
{
    return AtLeast(value, 0);
}

int MinInt(int lhs, int rhs)
{
    return lhs < rhs ? lhs : rhs;
}

int MaxInt(int lhs, int rhs)
{
    return lhs < rhs ? rhs : lhs;
}
} // namespace memochat::chat::messaging::envelope_modules
