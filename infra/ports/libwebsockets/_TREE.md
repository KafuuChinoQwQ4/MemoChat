# libwebsockets/ 目录树

> libwebsockets 的 vcpkg overlay 端口，钉住带 HTTP/3 WebTransport API 的上游 commit，供 ChatServer 可选 WebTransport provider 使用。

## 子目录

| 路径 | 作用概括 |
| --- | --- |
| [`patches/`](patches/_TREE.md) | 修补上游 libwebsockets WebTransport/H3 行为的 overlay patch。 |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `portfile.cmake` | 下载并配置启用 GnuTLS、HTTP/3、QUIC 与 WebTransport 角色的 libwebsockets。 |
| `vcpkg.json` | 声明 libwebsockets overlay 端口元数据和构建依赖。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
