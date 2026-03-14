#pragma once

#include "MediaHttpService.h"

class LogicSystem;

class AuthHttpService {
public:
    static void RegisterRoutes(LogicSystem& logic);
};

class ProfileHttpService {
public:
    static void RegisterRoutes(LogicSystem& logic);
};

class CallHttpServiceRoutes {
public:
    static void RegisterRoutes(LogicSystem& logic);
};
