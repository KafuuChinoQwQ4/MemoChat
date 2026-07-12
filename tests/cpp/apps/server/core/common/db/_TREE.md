# db/ 目录树

> 验证 PostgreSQL 与 MongoDB 无异常 C API 适配器的参数和失败状态契约。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `CMakeLists.txt` | 注册数据库无异常适配器 GTest。 |
| `NoExceptionsDbAdaptersTest.cpp` | 验证参数序列化及无效连接通过显式状态失败。 |
| `main.cpp` | GTest 入口。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
