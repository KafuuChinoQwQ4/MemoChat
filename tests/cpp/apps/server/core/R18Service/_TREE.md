# R18Service/ 目录树

> 测试 R18Service 内容源核心逻辑，覆盖 adapter 工具算法、官方 JM adapter 算法、source record DTO 编解码等行为。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `CMakeLists.txt` | 注册该测试目标的 CMake 配置 |
| `main.cpp` | GTest 测试入口（main 函数） |
| `R18AdapterUtilsAlgorithmsConsumer.cpp` | 直接 import R18 adapter utils module，向测试暴露 URL、编码、缓存和 Base64 primitive helper |
| `R18AdapterUtilsAlgorithmsTest.cpp` | 验证 R18 adapter utils module 导出的默认值、编码/转义和缓存/Base64 guard |
| `R18JmAdapterAlgorithmsConsumer.cpp` | 直接 import R18 JM adapter module，向测试暴露源标识、API 默认值、分页和图片 guard |
| `R18JmAdapterAlgorithmsTest.cpp` | 验证 R18 JM adapter module 导出的官方源默认值、host 轮换、payload/page 和图片 guard |
| `R18PicacgAdapterAlgorithmsConsumer.cpp` | 直接 import R18 Picacg adapter module，向测试暴露源/API 默认值、环境凭据和图片 guard |
| `R18PicacgAdapterAlgorithmsTest.cpp` | 验证 R18 Picacg adapter module 导出的官方源默认值、环境凭据、分页、章节和图片 guard |
| `R18PublicDtosTest.cpp` | 验证 R18 公开接口请求 DTO 的兼容解析与字段清单 |
| `R18RouteRegistrationAlgorithmsConsumer.cpp` | 直接 import R18 route registration module，向测试暴露 HTTP 方法和路径常量 |
| `R18RouteRegistrationAlgorithmsTest.cpp` | 验证 R18 网关 route registration module 导出的稳定方法和路径常量 |
| `R18RouteSchemaAlgorithmsConsumer.cpp` | 直接 import R18 route schema module，向测试暴露常量 helper |
| `R18RouteSchemaTest.cpp` | 验证 R18 稳定 action 路由只读 schema descriptor 与快照输出 |
| `R18ServiceAlgorithmsConsumer.cpp` | 直接 import R18 service facade module，向测试暴露认证、导入 payload 和响应 metadata helper |
| `R18ServiceAlgorithmsTest.cpp` | 验证 R18 service facade module 导出的错误消息、导入 payload guard、状态码和 content-type |
| `R18SourceRecordCodecTest.cpp` | 验证 R18 内容源记录 DTO 的 JSON 编解码与字段清单 |
| `R18SourceServiceAlgorithmsConsumer.cpp` | 直接 import R18 source service module，向测试暴露 zip 魔数、搜索分页、预览计数、JS 探测和源默认字面量 helper |
| `R18SourceServiceAlgorithmsTest.cpp` | 验证 R18 source service module 导出的 zip 魔数检测、搜索分页归一化、预览章节/页数、JS 探测判定、源字面量和错误消息 |
| `R18SourceServiceTest.cpp` | 验证 R18 内容源服务通过 C++ module 算法归一化导入源 ID 和版本路径段 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
