#include "PrivateHistoryDependenciesFactory.h"

#include "ChatFeatureController.h"

#include <utility>

namespace memochat::app
{

PrivateHistoryLoadCurrentDependencies
makePrivateHistoryLoadCurrentDependencies(ChatFeatureController& controller,
                                          std::shared_ptr<UserMgr> userMgr,
                                          std::function<void(const QString&)> setCurrentPeerName,
                                          std::function<void(const QString&)> setCurrentPeerIcon,
                                          std::function<void(bool)> setLoading,
                                          std::function<void(bool)> setCanLoadMore,
                                          std::function<void(int, qint64)> requestReadAck)
{
    return controller.privateHistoryLoadCurrentDependencies(std::move(userMgr),
                                                            std::move(setCurrentPeerName),
                                                            std::move(setCurrentPeerIcon),
                                                            std::move(setLoading),
                                                            std::move(setCanLoadMore),
                                                            std::move(requestReadAck));
}

PrivateHistoryRequestBuildDependencies
makePrivateHistoryRequestBuildDependencies(std::function<void(bool)> setLoading,
                                           std::function<void(int, const QByteArray&)> dispatchPayload)
{
    PrivateHistoryRequestBuildDependencies dependencies;
    dependencies.setLoading = std::move(setLoading);
    dependencies.dispatchPayload = std::move(dispatchPayload);
    return dependencies;
}

PrivateHistoryLoadMoreDependencies
makePrivateHistoryLoadMoreDependencies(ChatFeatureController& controller,
                                       std::function<void(bool)> setLoading,
                                       std::function<void(int, const QByteArray&)> dispatchPayload)
{
    PrivateHistoryLoadMoreDependencies dependencies;
    dependencies.messageModel = &controller.messageModel();
    dependencies.setLoading = std::move(setLoading);
    dependencies.dispatchPayload = std::move(dispatchPayload);
    return dependencies;
}

PrivateHistoryResponseDependencies
makePrivateHistoryResponseDependencies(ChatFeatureController& controller,
                                       std::shared_ptr<UserMgr> userMgr,
                                       std::function<void(bool)> setLoading,
                                       std::function<void(bool)> setCanLoadMore,
                                       std::function<void(int, qint64)> requestReadAck)
{
    return controller.privateHistoryResponseDependencies(std::move(userMgr),
                                                         std::move(setLoading),
                                                         std::move(setCanLoadMore),
                                                         std::move(requestReadAck));
}

} // namespace memochat::app
