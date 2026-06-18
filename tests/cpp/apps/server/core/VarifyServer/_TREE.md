# VarifyServer/ 目录树

> 测试 VarifyServer（验证码/邮件）服务，覆盖邮件投递任务编解码与验证码策略。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `CMakeLists.txt` | 注册该测试目标的 CMake 配置 |
| `EmailDeliveryTaskCodecTest.cpp` | 验证邮件投递任务的序列化/反序列化 |
| `VerifyCodePolicyTest.cpp` | 验证验证码生成/校验策略 |
| `main.cpp` | GTest 测试入口（main 函数） |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
