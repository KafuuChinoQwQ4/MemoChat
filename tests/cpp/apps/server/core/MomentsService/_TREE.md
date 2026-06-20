# MomentsService/ 目录树

> 测试 MomentsService 朋友圈服务核心逻辑，覆盖公开请求/响应 DTO 的 JSON 兼容性与字段清单。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `CMakeLists.txt` | 注册该测试目标的 CMake 配置 |
| `main.cpp` | GTest 测试入口（main 函数） |
| `MomentsOutputDtosTest.cpp` | 验证朋友圈公开请求/响应 DTO 的 JSON 字段、兼容解析、空数组和反射字段清单 |
| `MomentsRouteSchemaTest.cpp` | 验证朋友圈稳定路由只读 schema descriptor 与快照输出 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
