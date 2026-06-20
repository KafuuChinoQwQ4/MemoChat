# R18Service/ 目录树

> 测试 R18Service 内容源核心逻辑，覆盖 source record DTO 编解码等行为。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `CMakeLists.txt` | 注册该测试目标的 CMake 配置 |
| `main.cpp` | GTest 测试入口（main 函数） |
| `R18PublicDtosTest.cpp` | 验证 R18 公开接口请求 DTO 的兼容解析与字段清单 |
| `R18RouteSchemaTest.cpp` | 验证 R18 稳定 action 路由只读 schema descriptor 与快照输出 |
| `R18SourceRecordCodecTest.cpp` | 验证 R18 内容源记录 DTO 的 JSON 编解码与字段清单 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
