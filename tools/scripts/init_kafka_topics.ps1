$ErrorActionPreference = "Stop"
$DockerCli = Join-Path $PSScriptRoot "docker\arch-docker.ps1"

$container = "memochat-redpanda"
$topics = @(
    "chat.private.v1",
    "chat.group.v1",
    "dialog.sync.v1",
    "relation.state.v1",
    "user.profile.changed.v1",
    "audit.login.v1",
    "ai.agent.task.events.v1",
    "chat.private.dlq.v1",
    "chat.group.dlq.v1"
)

foreach ($topic in $topics) {
    & $DockerCli exec $container rpk topic create $topic --partitions 8 --replicas 1 | Out-Host
}
