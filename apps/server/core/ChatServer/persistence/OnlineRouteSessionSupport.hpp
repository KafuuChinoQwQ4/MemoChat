#pragma once

#include <memory>
#include <string>

class CSession;

std::string ExtractOnlineRouteSessionId(const std::shared_ptr<CSession>& session);
