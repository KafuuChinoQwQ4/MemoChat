#include <string>

import memochat.gate.h3_route_adapter_algorithms;

namespace memochat::tests::gate::h3_adapter
{
bool ShouldProcessConnection(bool has_connection)
{
    return memochat::gate::adapters::h3::modules::ShouldProcessConnection(has_connection);
}

bool ShouldAppendQueryToTarget(bool query_empty)
{
    return memochat::gate::adapters::h3::modules::ShouldAppendQueryToTarget(query_empty);
}

int FileBodyNotFoundStatus()
{
    return memochat::gate::adapters::h3::modules::FileBodyNotFoundStatus();
}

std::string FileBodyNotFoundBody()
{
    return memochat::gate::adapters::h3::modules::FileBodyNotFoundBody();
}

std::string FileBodyNotFoundContentType()
{
    return memochat::gate::adapters::h3::modules::FileBodyNotFoundContentType();
}

std::string DefaultFileContentType()
{
    return memochat::gate::adapters::h3::modules::DefaultFileContentType();
}

std::string DefaultInlineContentType()
{
    return memochat::gate::adapters::h3::modules::DefaultInlineContentType();
}

std::string
ResolveContentType(bool content_type_empty, const char* default_content_type, const char* route_content_type)
{
    return memochat::gate::adapters::h3::modules::ResolveContentType(content_type_empty,
                                                                     default_content_type,
                                                                     route_content_type);
}
} // namespace memochat::tests::gate::h3_adapter
