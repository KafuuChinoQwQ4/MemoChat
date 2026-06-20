# GateShared/ 目录树

> GateShared 共享路由/runtime 层的 C++ 测试。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `CMakeLists.txt` | 注册 GateShared 测试目标的 CMake 配置。 |
| `RouteSchemaTest.cpp` | 验证路由 DTO schema helper 的反射字段清单、无 body 路由与 snapshot 输出。 |
| `RouteSchemaCatalogTest.cpp` | 验证路由 schema 目录聚合：模块/路由顺序、文本与 JSON 渲染（含转义）及跨模块重复路由检测。 |
| `CrossModuleRouteSchemaCatalogTest.cpp` | 用全部 7 个真实路由模块的 `RouteSchemas()` 构建目录，断言无跨模块 `method+path` 冲突，并用注入的重复路由验证守卫确实生效。 |
| `main.cpp` | GTest 测试入口。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
