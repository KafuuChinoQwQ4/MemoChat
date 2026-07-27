# unix/ 目录树

> libpq baseline port 的 Unix 配置、安装和链接适配补丁。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `fix-configure.patch` | 修正 Unix configure 检测和生成行为 |
| `installdirs.patch` | 适配 vcpkg 的 libpq 安装目录 |
| `mingw-install.patch` | 修正 MinGW 安装目标 |
| `no-server-tools.patch` | 排除 libpq package 不需要的服务端工具 |
| `python.patch` | 适配 PostgreSQL Python 构建检测 |
| `single-linkage.patch` | 让构建遵守 vcpkg 的单一静态或动态链接选择 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
