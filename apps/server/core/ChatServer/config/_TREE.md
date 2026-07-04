# config/ 目录树

> ChatServer 各微服务的配置管理类，从 INI/Etcd 解析会话、消息服务、关系服务、关系查询服务的运行参数。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`cxx_modules/`](cxx_modules/_TREE.md) | 配置解析相关的 C++ module 接口 |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `ChatSessionConfig.cpp` | 会话配置实现 |
| `ChatSessionConfig.h` | 会话配置声明 |
| `ConfigMgr.cpp` | 配置管理总入口实现 |
| `ConfigMgr.h` | 配置管理声明 |
| `MessageServiceConfig.cpp` | 消息服务配置实现 |
| `MessageServiceConfig.h` | 消息服务配置声明 |
| `RelationQueryServiceConfig.cpp` | 关系查询服务配置实现 |
| `RelationQueryServiceConfig.h` | 关系查询服务配置声明 |
| `RelationServiceConfig.cpp` | 关系服务配置实现 |
| `RelationServiceConfig.h` | 关系服务配置声明 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
