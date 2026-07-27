# msquic/ 目录树

> 基于项目锁定的 vcpkg msquic port，仅将内置 OpenSSL 的模块目录改为稳定的 Linux 系统路径。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `portfile.cmake` | 下载、配置、构建并安装 msquic，同时消除临时构建路径泄露 |
| `vcpkg.json` | msquic port 的版本、特性与依赖元数据 |
| `*.patch`, `*.diff` | 与锁定 vcpkg 基线一致的 msquic 上游构建修补 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
