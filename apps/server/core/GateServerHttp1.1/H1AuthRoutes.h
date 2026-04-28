#pragma once

#include "H1LogicSystem.h"

class H1AuthService {
public:
    static void RegisterRoutes(H1LogicSystem& logic);
};

class H1ProfileService {
public:
    static void RegisterRoutes(H1LogicSystem& logic);
};

class H1CallServiceRoutes {
public:
    static void RegisterRoutes(H1LogicSystem& logic);
};
