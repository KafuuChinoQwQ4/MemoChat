#pragma once

#include <memory>
#include <string_view>

class IChatSession;

namespace memochat::chatserver::transport
{

bool PostInboundChatFrame(const std::shared_ptr<IChatSession>& session, short msg_id, std::string_view payload);

} // namespace memochat::chatserver::transport
