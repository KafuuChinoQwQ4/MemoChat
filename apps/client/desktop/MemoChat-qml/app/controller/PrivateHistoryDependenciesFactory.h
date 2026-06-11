#pragma once

#include "PrivateChatHistoryRequestService.h"

#include <QByteArray>
#include <functional>
#include <memory>
#include <vector>

class ChatFeatureController;
class UserMgr;

namespace memochat::app
{

PrivateHistoryLoadCurrentDependencies
makePrivateHistoryLoadCurrentDependencies(ChatFeatureController& controller,
                                          std::shared_ptr<UserMgr> userMgr,
                                          std::function<void(const QString&)> setCurrentPeerName,
                                          std::function<void(const QString&)> setCurrentPeerIcon,
                                          std::function<void(bool)> setLoading,
                                          std::function<void(bool)> setCanLoadMore,
                                          std::function<void(int, qint64)> requestReadAck);

PrivateHistoryRequestBuildDependencies
makePrivateHistoryRequestBuildDependencies(std::function<void(bool)> setLoading,
                                           std::function<void(int, const QByteArray&)> dispatchPayload);

PrivateHistoryLoadMoreDependencies
makePrivateHistoryLoadMoreDependencies(ChatFeatureController& controller,
                                       std::function<void(bool)> setLoading,
                                       std::function<void(int, const QByteArray&)> dispatchPayload);

PrivateHistoryResponseDependencies
makePrivateHistoryResponseDependencies(ChatFeatureController& controller,
                                       std::shared_ptr<UserMgr> userMgr,
                                       std::function<void(bool)> setLoading,
                                       std::function<void(bool)> setCanLoadMore,
                                       std::function<void(int, qint64)> requestReadAck);

} // namespace memochat::app
