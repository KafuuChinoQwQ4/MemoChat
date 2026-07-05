#pragma once

#include "IChatSession.hpp"
#include "MsgNode.hpp"

#include <memory>
#include <utility>

class LogicSystem;

class LogicNode
{
    friend class LogicSystem;

public:
    LogicNode(std::shared_ptr<IChatSession> session, std::shared_ptr<RecvNode> recvnode)
        : _session(std::move(session))
        , _recvnode(std::move(recvnode))
    {
    }

private:
    std::shared_ptr<IChatSession> _session;
    std::shared_ptr<RecvNode> _recvnode;
};
