import memochat.gate.h1_route_adapter_algorithms;

namespace memochat::tests::gate::h1
{
bool ShouldReadConnectionFields(bool has_connection)
{
    return memochat::gate::adapters::h1::modules::ShouldReadConnectionFields(has_connection);
}

bool ShouldSetContentType(bool content_type_empty)
{
    return memochat::gate::adapters::h1::modules::ShouldSetContentType(content_type_empty);
}

bool ShouldApplyFileResponse(bool file_body)
{
    return memochat::gate::adapters::h1::modules::ShouldApplyFileResponse(file_body);
}
} // namespace memochat::tests::gate::h1
