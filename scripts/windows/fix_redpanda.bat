@echo off
REM Fix Redpanda advertised address from host.docker.internal to 127.0.0.1
REM Run this ONCE after starting Docker Desktop

docker stop memochat-redpanda 2>nul

REM Write the fixed config to the data directory
echo Creating fixed redpanda.yaml in data directory...
mkdir D:\docker-data\memochat\redpanda\config 2>nul

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
echo         - address: 127.0.0.1
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
echo         - address: 127.0.0.1
echo           port: 18082
echo schema_registry: {}
) > D:\docker-data\memochat\redpanda\redpanda.yaml

echo Starting Redpanda with fixed config...
docker run --rm -v D:\docker-data\memochat\redpanda\redpanda.yaml:/etc/redpanda/redpanda.yaml:ro ^
    -v D:\docker-data\memochat\redpanda\data:/var/lib/redpanda/data ^
    --network=container:memochat-redpanda ^
    --name memochat-redpanda-temp redpandadata/redpanda:v24.2.6 redpanda start ^
    --rpc-server 0.0.0.0:33145 ^
    --kafka-addr 0.0.0.0:19092 ^
    --advertise-kafka-addr 127.0.0.1:19092 ^
    --admin-addr 0.0.0.0:9644 ^
    --pandaproxy-addr 0.0.0.0:18082 ^
    --advertise-pandaproxy-addr 127.0.0.1:18082 ^
    --developer-mode true ^
    --auto-create-topics-enabled

echo Done.
