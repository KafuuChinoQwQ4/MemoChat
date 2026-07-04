import memochat.gate.health_route_algorithms;

namespace memochat::tests::gate::health
{
const char* HealthPath()
{
    return memochat::gate::health::modules::HealthPath();
}

const char* ReadinessPath()
{
    return memochat::gate::health::modules::ReadinessPath();
}

const char* HealthStatusText()
{
    return memochat::gate::health::modules::HealthStatusText();
}

const char* ReadinessStatusText()
{
    return memochat::gate::health::modules::ReadinessStatusText();
}

const char* JsonContentType()
{
    return memochat::gate::health::modules::JsonContentType();
}

int SuccessfulProbeStatus()
{
    return memochat::gate::health::modules::SuccessfulProbeStatus();
}
} // namespace memochat::tests::gate::health
