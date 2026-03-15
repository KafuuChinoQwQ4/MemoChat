$ErrorActionPreference = "Stop"

$container = "memochat-rabbitmq"

docker exec $container rabbitmqadmin --username=guest --password=guest declare exchange name=memochat.direct type=direct durable=true | Out-Host
docker exec $container rabbitmqadmin --username=guest --password=guest declare exchange name=memochat.dlx type=direct durable=true | Out-Host

$queues = @(
    @{ Name = "chat.delivery.retry.q"; RoutingKey = "chat.delivery.retry" },
    @{ Name = "chat.notify.offline.q"; RoutingKey = "chat.notify.offline" },
    @{ Name = "relation.notify.q"; RoutingKey = "relation.notify" },
    @{ Name = "chat.outbox.relay.retry.q"; RoutingKey = "chat.outbox.relay.retry" },
    @{ Name = "verify.email.delivery.q"; RoutingKey = "verify.email.delivery" },
    @{ Name = "status.presence.refresh.q"; RoutingKey = "status.presence.refresh" },
    @{ Name = "gate.cache.invalidate.q"; RoutingKey = "gate.cache.invalidate" }
)

foreach ($queue in $queues) {
    docker exec $container rabbitmqadmin --username=guest --password=guest declare queue name=$($queue.Name) durable=true | Out-Host
    docker exec $container rabbitmqadmin --username=guest --password=guest declare binding source=memochat.direct destination_type=queue destination=$($queue.Name) routing_key=$($queue.RoutingKey) | Out-Host
}
