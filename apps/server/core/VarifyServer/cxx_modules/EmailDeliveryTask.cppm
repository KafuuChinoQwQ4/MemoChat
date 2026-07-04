export module memochat.varify.email_delivery_task_algorithms;

export namespace memochat::varify::email_delivery::modules
{
bool HasParseOutput(bool output_present)
{
    return output_present;
}

bool HasRequiredEmailTaskFields(bool email_empty, bool code_empty)
{
    return !email_empty && !code_empty;
}
} // namespace memochat::varify::email_delivery::modules
