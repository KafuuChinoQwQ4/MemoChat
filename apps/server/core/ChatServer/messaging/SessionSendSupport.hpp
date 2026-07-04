#pragma once

#include <memory>
#include <string>

class CSession;

void SendChatSessionPayload(const std::shared_ptr<CSession>& session, std::string payload, short msg_id);
