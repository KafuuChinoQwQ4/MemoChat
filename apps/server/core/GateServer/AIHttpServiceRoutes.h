#pragma once
#include "MediaHttpService.h"

class LogicSystem;
class ChatTransportContext;

class AIHttpServiceRoutes {
public:
    static void RegisterRoutes(LogicSystem& logic);
};
