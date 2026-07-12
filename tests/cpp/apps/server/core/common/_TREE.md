# common/ 目录树

> 测试服务端公共库 common/，覆盖认证、数据库、JSON、日志、proto 与 C++26 标准反射基础设施。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`auth/`](auth/_TREE.md) | 公共认证算法测试 |
| [`db/`](db/_TREE.md) | PostgreSQL 与 MongoDB 无异常适配器测试 |
| [`json/`](json/_TREE.md) | Glaze JSON 兼容性测试 |
| [`logging/`](logging/_TREE.md) | 结构化 JSON 日志测试 |
| [`proto/`](proto/_TREE.md) | Protobuf 消息序列化测试 |
| [`reflection/`](reflection/_TREE.md) | C++26 标准反射字段清单测试 |
| [`random/`](random/_TREE.md) | 返回码式安全随机 UUID 测试 |
| [`runtime/`](runtime/_TREE.md) | INI/Etcd 配置与 IO 上下文池算法测试 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
