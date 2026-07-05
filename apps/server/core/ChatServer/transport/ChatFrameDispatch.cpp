#include "ChatFrameDispatch.hpp"

#include "IChatSession.hpp"
#include "LogicNode.hpp"
#include "LogicSystem.hpp"
#include "MsgNode.hpp"
#include "const.hpp"
#include "logging/Logger.hpp"

#include <cstring>
#include <limits>
#include <string>

namespace memochat::chatserver::transport
{

bool PostInboundChatFrame(const std::shared_ptr<IChatSession>& session, short msg_id, std::string_view payload)
{
    if (!session || msg_id < 0 || payload.size() > static_cast<std::size_t>(MAX_LENGTH) ||
        payload.size() > static_cast<std::size_t>(std::numeric_limits<short>::max()))
    {
        memolog::LogWarn("chat.frame_dispatch.rejected",
                         "inbound chat frame rejected",
                         {{"msg_id", std::to_string(msg_id)},
                          {"transport", session ? session->transportName() : std::string()},
                          {"payload_size", std::to_string(payload.size())}});
        return false;
    }

    auto recv_node = std::make_shared<RecvNode>(static_cast<short>(payload.size()), msg_id);
    if (!payload.empty())
    {
        std::memcpy(recv_node->_data, payload.data(), payload.size());
    }
    recv_node->_cur_len = static_cast<short>(payload.size());
    recv_node->_data[recv_node->_total_len] = '\0';

    session->updateHeartbeat();
    LogicSystem::GetInstance()->PostMsgToQue(std::make_shared<LogicNode>(session, recv_node));
    return true;
}

} // namespace memochat::chatserver::transport
