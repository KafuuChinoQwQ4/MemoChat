# libpq/ 目录树

> libpq 16.9 的 Release overlay port，沿用锁定 baseline 并固定系统配置目录。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`android/`](android/_TREE.md) | Android 共享库命名适配 |
| [`unix/`](unix/_TREE.md) | Unix 平台的 libpq 配置和安装补丁 |
| [`windows/`](windows/_TREE.md) | Windows 平台的 libpq 工具链补丁 |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `Makefile` | vcpkg 仅构建 libpq 与所需客户端组件的入口 |
| `build-msvc.cmake` | MSVC 平台的 libpq 构建流程 |
| `libpq.props.in` | MSBuild 的 libpq 属性模板 |
| `portfile.cmake` | 获取固定源码、应用补丁并将系统配置目录固定为 `/etc/postgresql` |
| `usage` | vcpkg 安装后的 libpq 使用说明 |
| `vcpkg-cmake-wrapper.cmake` | PostgreSQL/libpq CMake 查找包装器 |
| `vcpkg-libs.props.in` | MSBuild 的 libpq 依赖库属性模板 |
| `vcpkg.json` | libpq 16.9 Release overlay 元数据与特性声明 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
