# db/ 目录树

> 数据库访问的无异常适配层，使用 libpq C API 提供显式状态和错误文本。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `MongoCCompat.hpp` | 封装 libmongoc 初始化、客户端池和无异常 lease 生命周期。 |
| `PqxxCompat.hpp` | 基于 libpq 的无异常 PostgreSQL 连接、事务、参数和结果适配器。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
