# ports/ 目录树

> vcpkg 自定义端口（overlay ports）：为依赖库提供本仓库定制的构建定义。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`glaze/`](glaze/_TREE.md) | Glaze JSON 库的 vcpkg overlay 端口定义。 |
| [`h2o/`](h2o/_TREE.md) | h2o HTTP 库的 vcpkg 自定义端口。 |
| [`libwebsockets/`](libwebsockets/_TREE.md) | 带 HTTP/3 WebTransport API 的 libwebsockets overlay 端口，供 ChatServer 可选 WebTransport provider 使用。 |
| [`nghttp2/`](nghttp2/_TREE.md) | nghttp2 库的 vcpkg 自定义端口。 |
| [`stdexec/`](stdexec/_TREE.md) | stdexec（std::execution 参考实现）的 vcpkg overlay 端口，钉到上游 main 修复 commit 以兼容 gcc-16。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
