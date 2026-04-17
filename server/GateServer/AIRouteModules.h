#pragma once

#include "MediaHttpService.h"

class LogicSystem;
class GrpcClientPool;

class AIHttpServiceRoutes {
public:
    static void RegisterRoutes(LogicSystem& logic);
};
