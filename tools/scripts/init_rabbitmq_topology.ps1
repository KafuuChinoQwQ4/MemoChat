$ErrorActionPreference = "Stop"
$DockerCli = Join-Path $PSScriptRoot "docker\arch-docker.ps1"

$container = "memochat-rabbitmq"
$rabbitUser = if ($env:MEMOCHAT_RABBITMQ_USER) { $env:MEMOCHAT_RABBITMQ_USER } else { "memochat" }
$rabbitPassword = if ($env:MEMOCHAT_RABBITMQ_PASSWORD) { $env:MEMOCHAT_RABBITMQ_PASSWORD } else { "123456" }

& $DockerCli exec $container rabbitmqadmin --username=$rabbitUser --password=$rabbitPassword declare exchange name=memochat.direct type=direct durable=true | Out-Host
& $DockerCli exec $container rabbitmqadmin --username=$rabbitUser --password=$rabbitPassword declare exchange name=memochat.dlx type=direct durable=true | Out-Host

$queues = @(
    @{ Name = "chat.delivery.retry.q"; RoutingKey = "chat.delivery.retry" },
    @{ Name = "chat.notify.offline.q"; RoutingKey = "chat.notify.offline" },
    @{ Name = "relation.notify.q"; RoutingKey = "relation.notify" },
    @{ Name = "chat.outbox.relay.retry.q"; RoutingKey = "chat.outbox.relay.retry" },
    @{ Name = "verify.email.delivery.q"; RoutingKey = "verify.email.delivery" },
    @{ Name = "status.presence.refresh.q"; RoutingKey = "status.presence.refresh" },
    @{ Name = "gate.cache.invalidate.q"; RoutingKey = "gate.cache.invalidate" },
    @{ Name = "ai.agent.tasks.q"; RoutingKey = "ai.agent.task.run" }
)

foreach ($queue in $queues) {
    & $DockerCli exec $container rabbitmqadmin --username=$rabbitUser --password=$rabbitPassword declare queue name=$($queue.Name) durable=true | Out-Host
    & $DockerCli exec $container rabbitmqadmin --username=$rabbitUser --password=$rabbitPassword declare binding source=memochat.direct destination_type=queue destination=$($queue.Name) routing_key=$($queue.RoutingKey) | Out-Host
}
