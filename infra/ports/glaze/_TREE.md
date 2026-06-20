# glaze/ 目录树

> Glaze JSON 库的 vcpkg overlay 端口定义，用于钉住带 C++26 P2996 reflection 支持的新版 Glaze。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `portfile.cmake` | Glaze 端口构建脚本，固定从上游 `v7.8.2` tag 拉取源码。 |
| `vcpkg.json` | Glaze 端口清单（版本、依赖和可选 `ssl` feature）。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
