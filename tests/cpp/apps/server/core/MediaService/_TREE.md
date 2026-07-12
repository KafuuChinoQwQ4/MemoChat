# MediaService/ 目录树

> 测试 MediaService 媒体服务核心逻辑，覆盖分片上传会话与公共上传 DTO 等行为。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `CMakeLists.txt` | 注册该测试目标的 CMake 配置 |
| `main.cpp` | GTest 测试入口（main 函数） |
| `MediaPublicDtosTest.cpp` | 验证媒体公共上传请求/固定成功响应 DTO 的 JSON 兼容性与字段清单 |
| `MediaRouteRegistrationAlgorithmsConsumer.cpp` | 在测试目标中直接 import 媒体路由注册算法 module，暴露 method/path 字面量给 GTest。 |
| `MediaRouteRegistrationAlgorithmsTest.cpp` | 验证媒体路由注册 method 和媒体上传/下载 path 字面量 module-backed helper |
| `MediaRouteSchemaAlgorithmsConsumer.cpp` | 在测试目标中直接 import 媒体路由 schema 算法 module，暴露路由 schema 字面量给 GTest。 |
| `MediaRouteSchemaTest.cpp` | 验证媒体生产路由只读 schema descriptor 与快照输出 |
| `MediaServiceAlgorithmsConsumer.cpp` | 在测试目标中直接 import 媒体 service facade guard 算法 module，暴露响应与上传下载判断给 GTest。 |
| `MediaServiceAlgorithmsTest.cpp` | 验证媒体 service facade module 的响应默认值、上传分片会话与下载 asset guard |
| `MediaStorageTest.cpp` | 验证本地媒体读路径、路径逃逸防护、Ready 状态及不可写根目录的显式失败 |
| `MediaUploadSessionDtoTest.cpp` | 验证媒体上传会话 DTO 的 JSON 编解码与字段清单 |
| `S3MediaStorageAlgorithmsConsumer.cpp` | 在测试目标中直接 import S3 存储算法 module，暴露默认区域/桶、对象名净化字符策略与桶前缀判断给 GTest。 |
| `S3MediaStorageAlgorithmsTest.cpp` | 验证 S3 存储 module 的默认字面量、对象名字符策略、启用判断与桶前缀 guard |
| `LocalMediaStorageAlgorithmsConsumer.cpp` | 在测试目标中直接 import 本地存储算法 module，暴露默认目录/内容类型、文件名净化字符策略与后端判断给 GTest。 |
| `LocalMediaStorageAlgorithmsTest.cpp` | 验证本地存储 module 的默认字面量、文件名字符策略、HTTP URL 与 S3 后端判断 guard |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
