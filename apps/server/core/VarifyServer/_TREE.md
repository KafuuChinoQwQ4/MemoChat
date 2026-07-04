# VarifyServer/ 目录树

> 验证码服务，提供 VarifyService gRPC（GetVarifyCode）：生成验证码、限流、写 Redis 并经邮件任务总线异步下发邮件。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`cxx_modules/`](cxx_modules/_TREE.md) | 验证码服务使用的项目自有 C++ module 接口 |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `CMakeLists.txt` | VarifyServer 构建脚本 |
| `ConfigMgr.cpp` | 配置管理实现 |
| `ConfigMgr.h` | 配置管理声明 |
| `EmailDeliveryTaskCodec.cpp` | 邮件投递任务序列化/反序列化实现 |
| `EmailDeliveryTaskCodec.h` | 邮件投递任务编解码声明 |
| `EmailSender.cpp` | 邮件发送实现，导入 module 复用 SMTP 端口/SSL 和状态行判定 |
| `EmailSender.h` | 邮件发送声明 |
| `EmailTaskBus.cpp` | 邮件异步任务总线实现，导入 module 复用 RabbitMQ 默认值和重试边界判定 |
| `EmailTaskBus.h` | 邮件任务总线声明 |
| `RateLimiter.cpp` | 验证码请求限流实现，导入 module 复用 Redis key 前缀和计数判定 |
| `RateLimiter.h` | 限流器声明 |
| `VarifyRedisMgr.cpp` | 验证码 Redis 存取实现，导入 module 复用默认连接参数、reply 类型和哨兵值判定 |
| `VarifyRedisMgr.h` | 验证码 Redis 管理声明 |
| `VarifyServer.cpp` | VarifyServer main 入口，导入 module 复用健康检查/就绪检查运行时字面量 |
| `VarifyServer.h` | VarifyServer 声明 |
| `VarifyServiceImpl.cpp` | VarifyService gRPC 服务实现，导入 module 复用 peer 前缀、日志模块名和 synthetic email 分支判定 |
| `VarifyServiceImpl.h` | VarifyService 服务声明 |
| `VerifyCodePolicy.cpp` | 验证码生成/有效期策略实现，导入 module 做后缀比较和长度归一化 |
| `VerifyCodePolicy.h` | 验证码策略声明 |
| `config.ini` | 服务运行配置 |
| `const.hpp` | 常量定义 |
| `varify2.ini` | 第二节点运行配置 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
