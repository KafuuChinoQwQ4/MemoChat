$ErrorActionPreference = "Stop"

$container = "memochat-redpanda"
$topics = @(
    "chat.private.v1",
    "chat.group.v1",
    "dialog.sync.v1",
    "relation.state.v1",
    "user.profile.changed.v1",
    "audit.login.v1",
    "chat.private.dlq.v1",
    "chat.group.dlq.v1"
)

foreach ($topic in $topics) {
    docker exec $container rpk topic create $topic --partitions 8 --replicas 1 | Out-Host
}
