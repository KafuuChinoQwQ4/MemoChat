# infra/ 目录树

> 全平台基础设施总目录：自研运维平台、部署编排、容器镜像、可观测性栈与第三方端口移植。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`Memo_ops/`](Memo_ops/_TREE.md) | 自研运维监控平台（采集、分析、服务端 + QML 客户端 + k8s 清单）。 |
| [`deploy/`](deploy/_TREE.md) | 部署编排：容器镜像、Helm/k8s 清单、本地 compose 与可观测性配置。 |
| [`docker/`](docker/_TREE.md) | 通用服务端 Dockerfile。 |
| [`observability/`](observability/_TREE.md) | 可观测性栈配置（ELK 日志栈 + OTel 指标/链路栈）。 |
| [`ports/`](ports/_TREE.md) | vcpkg 自定义端口与 overlay ports（Glaze、libwebsockets、nghttp2、stdexec 等）。 |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `README.md` | infra 目录总体说明。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
