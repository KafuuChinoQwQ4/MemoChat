# mongo-c-driver/ 目录树

> 固定 mongo-c-driver 1.30.6 baseline，并移除 MongoDB 握手元数据中的发行构建机编译路径。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `disable-dynamic-when-static.patch` | 沿用 baseline 的静态链接行为修正 |
| `fix-dependencies.patch` | 沿用 baseline 的依赖发现修正 |
| `fix-include-directory.patch` | 沿用 baseline 的安装头文件路径修正 |
| `fix-mingw.patch` | 沿用 baseline 的 MinGW 构建修正 |
| `portfile.cmake` | 构建固定版本并应用依赖、安装和握手元数据补丁 |
| `redact-build-flags.patch` | 清空会随 MongoDB 握手上报的 C/C++ 构建参数，避免泄露绝对路径 |
| `remove_abs_patch.cmake` | 沿用 baseline 的绝对安装路径清理规则 |
| `usage` | vcpkg 安装后的 CMake target 使用说明 |
| `vcpkg.json` | 固定版本、feature 与构建依赖元数据 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
