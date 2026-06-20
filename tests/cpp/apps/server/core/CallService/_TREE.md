# CallService/ 目录树

> 测试 CallService 通话服务核心支撑逻辑，覆盖通话请求与会话缓存 DTO 等行为。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `CallPublicDtosTest.cpp` | 验证通话公共请求/固定成功响应 DTO 的 JSON 兼容性与字段清单 |
| `CallRouteSchemaTest.cpp` | 验证通话生产路由只读 schema descriptor 与快照输出 |
| `CallSessionCacheDtoTest.cpp` | 验证通话会话缓存 DTO 的 JSON 编解码与字段清单 |
| `CMakeLists.txt` | 注册该测试目标的 CMake 配置 |
| `main.cpp` | GTest 测试入口（main 函数） |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
