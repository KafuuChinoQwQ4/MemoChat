# clients/ 目录树

> Gate 对各后端服务的客户端封装：鉴权校验、聊天与验证码服务的 gRPC/RPC 调用。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`cxx_modules/`](cxx_modules/_TREE.md) | 下游客户端相关 C++ module 接口分组。 |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `AuthVerifyClient.cpp` | 鉴权校验服务客户端实现。 |
| `AuthVerifyClient.h` | 鉴权校验客户端接口声明。 |
| `ChatGrpcClient.cpp` | ChatServer gRPC 客户端实现，导入轻量算法 module 处理连接池默认值、span 元数据和失败 guard。 |
| `ChatGrpcClient.h` | Chat gRPC 客户端接口与连接池声明。 |
| `VerifyGrpcClient.cpp` | 验证码服务 gRPC 客户端实现。 |
| `VerifyGrpcClient.h` | 验证码 gRPC 客户端接口声明。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
