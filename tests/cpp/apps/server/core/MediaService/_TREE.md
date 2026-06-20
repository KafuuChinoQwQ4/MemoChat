# MediaService/ 目录树

> 测试 MediaService 媒体服务核心逻辑，覆盖分片上传会话与公共上传 DTO 等行为。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `CMakeLists.txt` | 注册该测试目标的 CMake 配置 |
| `main.cpp` | GTest 测试入口（main 函数） |
| `MediaPublicDtosTest.cpp` | 验证媒体公共上传请求/固定成功响应 DTO 的 JSON 兼容性与字段清单 |
| `MediaRouteSchemaTest.cpp` | 验证媒体生产路由只读 schema descriptor 与快照输出 |
| `MediaUploadSessionDtoTest.cpp` | 验证媒体上传会话 DTO 的 JSON 编解码与字段清单 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
