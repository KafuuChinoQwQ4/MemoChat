# compose/ 目录树

> 按组件拆分的 Docker Compose 片段：数据存储、消息队列、LiveKit、可观测性与 Envoy 负载均衡配置/证书。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `datastores.yml` | Redis/PostgreSQL/MongoDB 数据存储 compose。 |
| `envoy-lb.yaml` | Envoy 负载均衡监听/集群配置。 |
| `envoy-lb.yml` | Envoy 负载均衡 compose 服务定义。 |
| `envoy.yaml` | Envoy 网关配置。 |
| `generate-envoy-certs.sh` | 生成 Envoy TLS 证书的脚本。 |
| `kafka.yml` | Redpanda/Kafka compose。 |
| `livekit.yml` | LiveKit/Redis/coturn 音视频 compose。 |
| `observability.yml` | 可观测性栈 compose（Prometheus/Grafana/Loki/Tempo/OTel 等）。 |
| `rabbitmq.yml` | RabbitMQ compose。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
