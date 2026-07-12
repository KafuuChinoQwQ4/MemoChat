# async/ 目录树

> Gate 账号域的 Kafka 事件发布器，承载登录审计与用户资料变更事件。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`cxx_modules/`](cxx_modules/_TREE.md) | Kafka topic、事件类型与错误字面量的项目自有 C++ module 接口 |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `GateAsyncSideEffectDtos.cpp` | 账号 Kafka 事件 payload/envelope DTO 编解码实现 |
| `GateAsyncSideEffectDtos.hpp` | 账号 Kafka 事件 payload/envelope DTO 声明 |
| `GateAsyncSideEffects.cpp` | 账号 Kafka 事件发布实现 |
| `GateAsyncSideEffects.hpp` | 账号 Kafka 事件发布声明 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
