#pragma once

#include "Singleton.h"
#include <string>
#include <memory>

class GateServerDrogon : public Singleton<GateServerDrogon>
{
    friend class Singleton<GateServerDrogon>;
public:
    void Initialize();
    void Start();

private:
    GateServerDrogon() = default;
    ~GateServerDrogon() = default;
};
