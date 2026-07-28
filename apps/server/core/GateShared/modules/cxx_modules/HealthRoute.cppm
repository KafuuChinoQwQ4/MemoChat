export module memochat.gate.health_route_algorithms;

export namespace memochat::gate::health::modules
{
const char* HealthPath()
{
    return "/healthz";
}

const char* ReadinessPath()
{
    return "/readyz";
}

const char* HealthStatusText()
{
    return "ok";
}

const char* ReadinessStatusText()
{
    return "ready";
}

const char* JsonContentType()
{
    return "application/json";
}

int SuccessfulProbeStatus()
{
    return 200;
}

int ReadinessFailureStatus()
{
    return 503;
}

const char* NotReadyStatusText()
{
    return "not_ready";
}
} // namespace memochat::gate::health::modules
