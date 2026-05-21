@echo off
REM Fix Redpanda advertised address for WSL Docker bridge access
REM Current project Docker runs in Arch Linux native Docker.
cd /d "%~dp0..\.."
set "PROJECT_ROOT=%CD%"
set "DOCKER=%PROJECT_ROOT%\tools\scripts\docker\arch-docker.cmd"

"%DOCKER%" stop memochat-redpanda 2>nul

REM Write the fixed config to the data directory
echo Creating fixed redpanda.yaml in data directory...
if not exist "\\wsl.localhost\archlinux\data\docker-data\memochat\redpanda\config" mkdir "\\wsl.localhost\archlinux\data\docker-data\memochat\redpanda\config" 2>nul

(
echo redpanda:
echo     data_directory: /var/lib/redpanda/data
echo     seed_servers: []
echo     rpc_server:
echo         address: 0.0.0.0
echo         port: 33145
echo     kafka_api:
echo         - address: 0.0.0.0
echo           port: 19092
echo     admin:
echo         - address: 0.0.0.0
echo           port: 9644
echo     advertised_rpc_api:
echo         address: 127.0.0.1
echo         port: 33145
echo     advertised_kafka_api:
echo         - address: host.docker.internal
echo           port: 19092
echo     developer_mode: true
echo     auto_create_topics_enabled: true
echo     fetch_reads_debounce_timeout: 10
echo     group_initial_rebalance_delay: 0
echo     group_topic_partitions: 3
echo     log_segment_size_min: 1
echo     storage_min_free_bytes: 10485760
echo     topic_partitions_per_shard: 1000
echo     write_caching_default: "true"
echo rpk:
echo     overprovisioned: true
echo     coredump_dir: /var/lib/redpanda/coredump
echo pandaproxy:
echo     pandaproxy_api:
echo         - address: 0.0.0.0
echo           port: 18082
echo     advertised_pandaproxy_api:
echo         - address: host.docker.internal
echo           port: 18082
echo schema_registry: {}
) > "\\wsl.localhost\archlinux\data\docker-data\memochat\redpanda\redpanda.yaml"

echo Starting Redpanda with fixed config...
"%DOCKER%" run --rm -v /data/docker-data/memochat/redpanda/redpanda.yaml:/etc/redpanda/redpanda.yaml:ro ^
    -v /data/docker-data/memochat/redpanda/data:/var/lib/redpanda/data ^
    --add-host host.docker.internal:host-gateway ^
    --network=container:memochat-redpanda ^
    --name memochat-redpanda-temp redpandadata/redpanda:v24.2.6 redpanda start ^
    --overprovisioned ^
    --smp 1 ^
    --memory 1G ^
    --reserve-memory 0M ^
    --node-id 0 ^
    --check=false ^
    --rpc-server 0.0.0.0:33145 ^
    --kafka-addr 0.0.0.0:19092 ^
    --advertise-kafka-addr host.docker.internal:19092 ^
    --pandaproxy-addr 0.0.0.0:18082 ^
    --advertise-pandaproxy-addr host.docker.internal:18082 ^
    --developer-mode true ^
    --auto-create-topics-enabled

echo Done.
