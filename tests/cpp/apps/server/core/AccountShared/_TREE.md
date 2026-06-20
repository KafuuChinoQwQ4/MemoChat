# AccountShared/ 目录树

> 测试 AccountShared 账号共享库，覆盖登录缓存 DTO 等账号核心支撑逻辑。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `AuthLoginCacheProfileDtoTest.cpp` | 验证登录资料缓存 DTO 的 JSON 编解码与字段清单 |
| `AuthPublicDtosTest.cpp` | 验证账号公开接口请求 DTO 的兼容解析与字段清单 |
| `AuthRouteSchemaTest.cpp` | 验证认证稳定路由只读 schema descriptor 与快照输出 |
| `CMakeLists.txt` | 注册该测试目标的 CMake 配置 |
| `GateAsyncSideEffectDtosTest.cpp` | 验证 Gate 异步副作用 DTO 的 JSON 编解码与字段清单 |
| `main.cpp` | GTest 测试入口（main 函数） |
| `ProfileRouteSchemaTest.cpp` | 验证资料路由只读 schema descriptor 与快照输出 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
