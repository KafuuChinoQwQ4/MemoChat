# stdexec/ 目录树

> stdexec（NVIDIA 的 std::execution / P2300 sender-receiver 参考实现）的 vcpkg overlay 端口定义。钉到上游 main 修复 commit（633c873，2026-06-18），因为 vcpkg 内置的 2026-02-26 版本在 gcc-16 下编不过（见 portfile 注释与 NVIDIA/stdexec#1747）。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `portfile.cmake` | stdexec 端口构建脚本，从上游 main commit 633c873 拉取，启用 asio/tbb/taskflow feature；并用 `vcpkg_replace_string` 修正 execution.bs Revision 正则解析。 |
| `vcpkg.json` | stdexec 端口清单（version-date、host 工具依赖、asio/tbb/taskflow feature）。 |
| `fix-boost-asio-dependency.patch` | 让 asio feature 用 vcpkg 的 Boost 而非自行下载（上游原 patch）。 |
| `fix-tbb-dependency.patch` | 让 tbb feature 用 vcpkg 的 TBB（上游原 patch）。 |
| `fix-taskflow-dependency.patch` | 让 taskflow feature 用 vcpkg 的 taskflow（上游原 patch）。 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
