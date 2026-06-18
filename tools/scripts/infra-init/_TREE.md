# infra-init/ 目录树

> 基础设施初始化脚本：Kafka/Redpanda 主题、RabbitMQ 拓扑与 Chat 配置补丁。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `fix_redpanda_config.py` | 修复 Redpanda 配置的脚本 |
| `init_kafka_topics.ps1` | 初始化 Kafka 主题 |
| `init_rabbitmq_topology.ps1` | 初始化 RabbitMQ 交换机/队列拓扑 |
| `patch_chat_configs.ps1` | 批量打补丁修正 Chat 服务配置 |
| `redpanda_fixed.yaml` | 修正后的 Redpanda 配置样例 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
