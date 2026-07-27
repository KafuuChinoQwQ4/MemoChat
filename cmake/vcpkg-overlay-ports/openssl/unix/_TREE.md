# unix/ 目录树

> OpenSSL baseline port 的 Unix 配置与安装适配文件。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `android-cc.patch` | 修正 Android 编译器选择 |
| `configure` | 过滤并转交 vcpkg 生成的 OpenSSL Configure 参数 |
| `move-openssldir.patch` | 将 OpenSSL 配置目录移动到 vcpkg package 根下 |
| `no-empty-dirs.patch` | 避免为未构建的 engine/provider 创建空目录 |
| `no-static-libs-for-shared.patch` | 动态构建时避免安装静态库 |
| `portfile.cmake` | Unix OpenSSL 编译、安装和 package 清理流程 |
| `remove-deps.cmake` | 清除生成 Makefile 中不可重定位的依赖尾部 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
