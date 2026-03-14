#pragma once

#include "LogicSystem.h"

class ChatSessionServiceRegistrar {
public:
    void Register(LogicSystem& logic, std::map<short, FunCallBack>& callbacks) const;
};

class ChatRelationServiceRegistrar {
public:
    void Register(LogicSystem& logic, std::map<short, FunCallBack>& callbacks) const;
};

class PrivateMessageServiceRegistrar {
public:
    void Register(LogicSystem& logic, std::map<short, FunCallBack>& callbacks) const;
};

class GroupMessageServiceRegistrar {
public:
    void Register(LogicSystem& logic, std::map<short, FunCallBack>& callbacks) const;
};

class AsyncEventDispatcherRegistrar {
public:
    void Register(LogicSystem& logic, std::map<short, FunCallBack>& callbacks) const;
};
