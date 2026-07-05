#pragma once

#include <memory>
#include <string>

class IChatSession;

std::string ExtractOnlineRouteSessionId(const std::shared_ptr<IChatSession>& session);
