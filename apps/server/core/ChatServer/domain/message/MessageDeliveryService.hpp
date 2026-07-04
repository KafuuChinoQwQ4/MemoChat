#pragma once

#include "json/GlazeCompat.hpp"
#include "ports/IDeliveryGateway.hpp"
#include "ports/IDeliveryTaskPublisher.hpp"
#include "ports/IOnlineRouteStore.hpp"
#include "ports/ISessionRegistry.hpp"

#include <string>
#include <vector>

class MessageDeliveryService : public IDeliveryGateway
{
public:
    explicit MessageDeliveryService(IDeliveryTaskPublisher* task_publisher = nullptr,
                                    ISessionRegistry* session_registry = nullptr,
                                    IOnlineRouteStore* online_route_store = nullptr);
    void SetTaskPublisher(IDeliveryTaskPublisher* task_publisher);
    void SetRouteDependencies(ISessionRegistry* session_registry, IOnlineRouteStore* online_route_store);

    void PushPayload(const std::vector<int>& recipients,
                     short msgid,
                     const memochat::json::JsonValue& payload,
                     int exclude_uid = 0) override;
    bool TryPushPayload(const std::vector<int>& recipients,
                        short msgid,
                        const memochat::json::JsonValue& payload,
                        int exclude_uid = 0,
                        bool enqueue_on_failure = false) override;

private:
    IDeliveryTaskPublisher* _task_publisher = nullptr;
    ISessionRegistry* _session_registry = nullptr;
    IOnlineRouteStore* _online_route_store = nullptr;
};
