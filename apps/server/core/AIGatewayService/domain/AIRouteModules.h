#pragma once

class LogicSystem;
class GrpcClientPool;

class AIHttpServiceRoutes
{
public:
    static void RegisterRoutes(LogicSystem& logic);
};
