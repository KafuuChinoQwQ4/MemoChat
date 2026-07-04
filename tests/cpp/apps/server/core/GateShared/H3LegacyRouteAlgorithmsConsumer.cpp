import memochat.gate.h3_legacy_route_algorithms;

namespace memochat::tests::gate::h3::legacy
{
int GetRouteCount()
{
    return memochat::gate::h3::legacy::modules::GetRouteCount();
}

const char* GetRoutePathAt(int index)
{
    return memochat::gate::h3::legacy::modules::GetRoutePathAt(index);
}

int PostRouteCount()
{
    return memochat::gate::h3::legacy::modules::PostRouteCount();
}

const char* PostRoutePathAt(int index)
{
    return memochat::gate::h3::legacy::modules::PostRoutePathAt(index);
}

int RouteNotFoundStatus()
{
    return memochat::gate::h3::legacy::modules::RouteNotFoundStatus();
}

const char* RouteNotFoundBody()
{
    return memochat::gate::h3::legacy::modules::RouteNotFoundBody();
}

const char* JsonContentType()
{
    return memochat::gate::h3::legacy::modules::JsonContentType();
}
} // namespace memochat::tests::gate::h3::legacy
