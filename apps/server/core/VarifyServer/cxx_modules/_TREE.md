# cxx_modules/ 目录树

> VarifyServer 的项目自有 C++ module interface 文件。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `EmailDeliveryTask.cppm` | 邮件投递任务编解码使用的输出指针和必填字段判定算法 module |
| `EmailSender.cppm` | 邮件发送器使用的默认 SMTP 端口、隐式 SSL 和 SMTP 状态行判定算法 module |
| `EmailTaskBus.cppm` | 邮件任务总线使用的 RabbitMQ 默认值、重试参数归一化和重试边界判定算法 module |
| `RateLimiter.cppm` | 验证码限流器使用的 Redis key 前缀和计数结果判定算法 module |
| `VarifyRedis.cppm` | 验证码 Redis 管理器使用的默认端口/池大小、reply 类型和哨兵值判定算法 module |
| `VarifyServerRuntime.cppm` | VarifyServer 健康检查/就绪检查入口使用的路径、响应元数据和分支 guard module |
| `VarifyService.cppm` | VarifyService gRPC 处理路径使用的 peer 前缀、日志模块名和 synthetic email 分支判定算法 module |
| `VerifyCode.cppm` | 验证码策略使用的 ASCII 后缀比较和验证码长度归一化算法 module |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
