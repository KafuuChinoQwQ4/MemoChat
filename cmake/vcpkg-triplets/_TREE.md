# vcpkg-triplets/ 目录树

> MemoChat 可复现发行构建使用的 vcpkg overlay triplet，负责稳定 ABI 与清除宿主构建路径。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `x64-linux-memochat-release.cmake` | x64 Linux 静态依赖发行 triplet，并将 vcpkg 绝对路径映射为稳定前缀 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
