# MediaService/ 目录树

> 媒体网关微服务根：负责聊天媒体资产的上传/下载与对象存储接入，独立成 MediaGateway 服务进程。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`app/`](app/_TREE.md) | MediaGateway 服务进程入口 |
| [`core/`](core/_TREE.md) | 媒体存储与 HTTP 支撑等核心基础设施 |
| [`domain/`](domain/_TREE.md) | 媒体业务领域层：路由、profile 与服务 |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `CMakeLists.txt` | MediaService 的构建定义 |
| `mediagateway.ini` | 媒体网关运行期配置（Postgres/Redis/MinIO/鉴权等段） |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
