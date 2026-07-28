# services/ 目录树

> 各服务专用容器镜像 Dockerfile。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `cpp-service.Dockerfile` | 从已校验的服务 bundle 和固定 Ubuntu digest 构建非 root、固定运行时依赖与法律文件的 C++ 后端镜像，并验证运行 UID 可读取系统 CA。 |
| `memo-ops.Dockerfile` | MemoOps 运维平台镜像构建文件。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
