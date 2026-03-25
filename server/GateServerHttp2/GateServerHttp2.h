#pragma once

#include "Singleton.h"
#include <string>
#include <memory>

class GateServerHttp2 : public Singleton<GateServerHttp2>
{
    friend class Singleton<GateServerHttp2>;
public:
    void Initialize();
    void Run();
    void Stop();

    GateServerHttp2() = default;
    ~GateServerHttp2() = default;
};
