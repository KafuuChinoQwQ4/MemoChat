# VarifyServer/ 目录树

> 测试 VarifyServer（验证码/邮件）服务，覆盖邮件投递任务编解码与验证码策略。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `CMakeLists.txt` | 注册该测试目标的 CMake 配置 |
| `EmailDeliveryTaskCodecTest.cpp` | 验证邮件投递任务的序列化/反序列化 |
| `EmailSenderAlgorithmsConsumer.cpp` | 在测试目标中直接 import 邮件发送算法 module，暴露 SMTP 端口/SSL 和状态行判定给 GTest |
| `EmailSenderAlgorithmsTest.cpp` | 验证邮件发送算法 module 的默认 SMTP 端口、隐式 SSL 和 SMTP 状态码边界 |
| `EmailTaskBusAlgorithmsConsumer.cpp` | 在测试目标中直接 import 邮件任务总线算法 module，暴露 RabbitMQ 默认值和重试判定给 GTest |
| `EmailTaskBusAlgorithmsTest.cpp` | 验证邮件任务总线算法 module 的默认 RabbitMQ 参数、重试参数归一化和重试边界 |
| `RateLimiterAlgorithmsConsumer.cpp` | 在测试目标中直接 import 验证码限流算法 module，暴露 key 前缀和计数判定给 GTest |
| `RateLimiterAlgorithmsTest.cpp` | 验证验证码限流算法 module 的 Redis key 前缀、错误计数和限流边界 |
| `VarifyRedisAlgorithmsConsumer.cpp` | 在测试目标中直接 import 验证码 Redis 算法 module，暴露默认连接参数、reply 判定和哨兵值给 GTest |
| `VarifyRedisAlgorithmsTest.cpp` | 验证验证码 Redis 算法 module 的默认连接参数、reply guard、哨兵值和首次递增边界 |
| `VarifyServerRuntimeAlgorithmsConsumer.cpp` | 在测试目标中直接 import VarifyServer runtime 算法 module，暴露健康检查路径和响应元数据给 GTest |
| `VarifyServerRuntimeAlgorithmsTest.cpp` | 验证 VarifyServer runtime 算法 module 的健康检查路径、响应体和分支 guard |
| `VarifyServiceAlgorithmsConsumer.cpp` | 在测试目标中直接 import VarifyService 算法 module，暴露 peer 前缀、日志模块名和 synthetic email 分支判定给 GTest |
| `VarifyServiceAlgorithmsTest.cpp` | 验证 VarifyService 算法 module 的 gRPC peer 前缀、日志模块名和 synthetic email 分支边界 |
| `VerifyCodePolicyTest.cpp` | 验证验证码生成/校验策略 |
| `main.cpp` | GTest 测试入口（main 函数） |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
