#pragma once

class LogicSystem;
class GateHttp3Connection;

class GateHttp3Service {
public:
    static void RegisterRoutes(LogicSystem& logic);
};
