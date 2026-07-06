# 媒体与文件契约

媒体链路由客户端、MediaGatewayServer、MinIO 和访问控制共同组成。

## 上传

- Web 前端媒体能力位于 `apps/web/src/shared/media/`。
- QML 客户端媒体能力位于 `apps/client/desktop/MemoChat-qml/core/media`、`shared/media` 和相关 feature。
- 服务端媒体入口由 `MediaGatewayServer` 负责。
- 对象存储使用 MinIO，本地默认由 `memochat-minio` 提供。

## 下载和访问控制

媒体访问控制设计以 `apps/server/core/docs/media-access-control.md` 为准。不要在客户端直接假设对象存储公开可读；下载 URL、鉴权、过期策略和资源归属应由服务端契约决定。

## 凭据

MinIO access key 和 secret key 生产环境必须通过环境变量或 Secret 注入。本地 Docker 可以使用 compose 中的 dev 默认值，但不要把本地默认值复制到生产部署。
