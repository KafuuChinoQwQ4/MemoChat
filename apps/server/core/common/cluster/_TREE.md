# cluster/ 目录树

> 聊天集群节点发现抽象及其 Etcd 实现，供 ChatServer 各节点动态注册与互相寻址。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `ChatClusterDiscovery.h` | 集群节点发现接口抽象 |
| `EtcdClusterDiscovery.cpp` | 基于 Etcd 的节点发现实现 |
| `EtcdClusterDiscovery.h` | Etcd 节点发现声明 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
