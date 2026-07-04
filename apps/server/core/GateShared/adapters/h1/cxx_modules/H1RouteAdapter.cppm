export module memochat.gate.h1_route_adapter_algorithms;

export namespace memochat::gate::adapters::h1::modules
{
bool ShouldReadConnectionFields(bool has_connection)
{
    return has_connection;
}

bool ShouldSetContentType(bool content_type_empty)
{
    return !content_type_empty;
}

bool ShouldApplyFileResponse(bool file_body)
{
    return file_body;
}
} // namespace memochat::gate::adapters::h1::modules
