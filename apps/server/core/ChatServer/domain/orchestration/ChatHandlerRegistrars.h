#pragma once

#include "LogicSystem.h"

class ChatRuntimeComposition;

class ChatSessionServiceRegistrar
{
public:
    void Register(ChatRuntimeComposition& composition, std::map<short, FunCallBack>& callbacks) const;
};

class ChatRelationServiceRegistrar
{
public:
    void Register(ChatRuntimeComposition& composition, std::map<short, FunCallBack>& callbacks) const;
};

class PrivateMessageServiceRegistrar
{
public:
    void Register(ChatRuntimeComposition& composition, std::map<short, FunCallBack>& callbacks) const;
};

class GroupMessageServiceRegistrar
{
public:
    void Register(ChatRuntimeComposition& composition, std::map<short, FunCallBack>& callbacks) const;
};

class AsyncEventDispatcherRegistrar
{
public:
    void Register(ChatRuntimeComposition& composition, std::map<short, FunCallBack>& callbacks) const;
};
