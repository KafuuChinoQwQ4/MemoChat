#!/usr/bin/env bash

# Local runtime service topology shared by deploy/start/stop scripts.
#
# Row fields:
# group|runtime_dir|executable|source_executable|config_source|display_name|tcp_wait_port|udp_wait_ports|instance_name|stop_tcp_ports|stop_udp_ports|log_dir|telemetry_service_name|telemetry_namespace

readonly MEMOCHAT_TOPOLOGY_GROUP_WORKER="worker"
readonly MEMOCHAT_TOPOLOGY_GROUP_RELATION_QUERY="relation_query"
readonly MEMOCHAT_TOPOLOGY_GROUP_RELATION_SERVICE="relation_service"
readonly MEMOCHAT_TOPOLOGY_GROUP_MESSAGE_SERVICE="message_service"
readonly MEMOCHAT_TOPOLOGY_GROUP_VARIFY="varify"
readonly MEMOCHAT_TOPOLOGY_GROUP_CHAT="chat"
readonly MEMOCHAT_TOPOLOGY_GROUP_AI="ai"
readonly MEMOCHAT_TOPOLOGY_GROUP_GATE="gate"

readonly -a MEMOCHAT_RUNTIME_SERVICE_TOPOLOGY=(
    "gate|GateServer1|GateServer|GateServer|GateServer/config.ini|GateServer-1|8080|||8080 8082|8081|../../artifacts/logs/services/GateServer1|GateServer1|memochat"
    "gate|GateServer2|GateServer|GateServer|GateServer/gate2.ini|GateServer-2|8084|||8084|8085|../../artifacts/logs/services/GateServer2|GateServer2|memochat"
    "chat|chatserver1|ChatServer|ChatServer|ChatServer/chatserver1.ini|ChatServer-1|8090|8190||8090 50055|8190|../../artifacts/logs/services/chatserver1|ChatServer|memochat"
    "chat|chatserver2|ChatServer|ChatServer|ChatServer/chatserver2.ini|ChatServer-2|8091|8191||8091 50056|8191|../../artifacts/logs/services/chatserver2|ChatServer|memochat"
    "worker|ChatDeliveryWorker1|ChatDeliveryWorker|ChatDeliveryWorker|ChatServer/chatdeliveryworker1.ini|ChatDeliveryWorker-1|||chatdeliveryworker1|||../../artifacts/logs/services/chatdeliveryworker1|ChatDeliveryWorker|memochat"
    "relation_query|ChatRelationQueryService1|ChatRelationQueryService|ChatRelationQueryService|ChatServer/chatrelationquery1.ini|ChatRelationQueryService-1|50090||chatrelationquery1|50090||../../artifacts/logs/services/chatrelationquery1|ChatRelationQueryService|memochat"
    "relation_service|ChatRelationServiceWorker1|ChatRelationServiceWorker|ChatRelationServiceWorker|ChatServer/chatrelationservice1.ini|ChatRelationServiceWorker-1|50091||chatrelationservice1|50091||../../artifacts/logs/services/chatrelationservice1|ChatRelationServiceWorker|memochat"
    "message_service|ChatMessageService1|ChatMessageService|ChatMessageService|ChatServer/chatmessageservice1.ini|ChatMessageService-1|50092||chatmessageservice1|50092||../../artifacts/logs/services/chatmessageservice1|ChatMessageService|memochat"
    "ai|AIServer|AIServer|AIServer|AIServer/config.ini|AIServer|8095|||8095||../../artifacts/logs/services/AIServer|AIServer|memochat"
    "varify|VarifyServer1|VarifyServer|VarifyServer|VarifyServer/config.ini|VarifyServer-1|50051|||50051 8083||../../artifacts/logs/services/VarifyServer1|VarifyServer1|memochat"
    "varify|VarifyServer2|VarifyServer|VarifyServer|VarifyServer/varify2.ini|VarifyServer-2|48083|||48083 8087||../../artifacts/logs/services/VarifyServer2|VarifyServer2|memochat"
)

readonly -a MEMOCHAT_CORE_START_GROUPS=(
    "varify"
    "chat"
    "ai"
    "gate"
)

readonly -a MEMOCHAT_STOP_PORT_GROUP_ORDER=(
    "gate|GateServer|tcp udp"
    "ai|AIServer|tcp"
    "message_service|ChatMessageService|tcp"
    "relation_service|ChatRelationServiceWorker|tcp"
    "relation_query|ChatRelationQueryService|tcp"
    "chat|ChatServer|tcp udp"
    "varify|VarifyServer|tcp"
)

readonly -a MEMOCHAT_STOP_PID_ORDER=(
    "GateServer-2"
    "GateServer-1"
    "AIServer"
    "ChatDeliveryWorker-1"
    "ChatMessageService-1"
    "ChatRelationServiceWorker-1"
    "ChatRelationQueryService-1"
    "ChatServer-2"
    "ChatServer-1"
    "VarifyServer-2"
    "VarifyServer-1"
)

readonly -a MEMOCHAT_STOP_EXECUTABLE_ORDER=(
    "GateServer"
    "AIServer"
    "ChatDeliveryWorker"
    "ChatMessageService"
    "ChatRelationServiceWorker"
    "ChatRelationQueryService"
    "ChatServer"
    "VarifyServer"
)

memochat_topology_group_label() {
    case "$1" in
        "$MEMOCHAT_TOPOLOGY_GROUP_VARIFY")
            printf 'VarifyServer'
            ;;
        "$MEMOCHAT_TOPOLOGY_GROUP_CHAT")
            printf 'ChatServer'
            ;;
        "$MEMOCHAT_TOPOLOGY_GROUP_AI")
            printf 'AIServer'
            ;;
        "$MEMOCHAT_TOPOLOGY_GROUP_GATE")
            printf 'GateServer'
            ;;
        "$MEMOCHAT_TOPOLOGY_GROUP_WORKER")
            printf 'ChatDeliveryWorker'
            ;;
        "$MEMOCHAT_TOPOLOGY_GROUP_RELATION_QUERY")
            printf 'ChatRelationQueryService'
            ;;
        "$MEMOCHAT_TOPOLOGY_GROUP_RELATION_SERVICE")
            printf 'ChatRelationServiceWorker'
            ;;
        "$MEMOCHAT_TOPOLOGY_GROUP_MESSAGE_SERVICE")
            printf 'ChatMessageService'
            ;;
        *)
            printf '%s' "$1"
            ;;
    esac
}
