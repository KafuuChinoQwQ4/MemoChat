#pragma once

#include <memory>
#include <string>

class IChatSession;

void SendChatSessionPayload(const std::shared_ptr<IChatSession>& session, std::string payload, short msg_id);
void SendChatSessionOfflineNotification(const std::shared_ptr<IChatSession>& session, int uid);
