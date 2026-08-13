# openssl/ 目录树

> OpenSSL 3.6.0 的 Release overlay port，沿用锁定 baseline 并移除产物中的构建机路径。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`unix/`](unix/_TREE.md) | Unix 平台的 OpenSSL 配置、安装和依赖清理逻辑 |
| [`windows/`](windows/_TREE.md) | baseline 中保留的 Windows OpenSSL 构建补丁与入口 |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `aes_cfb128_vaes_encdec_wrapper.diff` | 修复 OpenSSL VAES CFB128 包装器构建问题 |
| `cmake-config.patch` | 调整 OpenSSL CMake package 的安装位置和 vcpkg 查找行为 |
| `command-line-length.patch` | 缩短生成步骤命令行，避免平台命令长度限制 |
| `install-pc-files.cmake` | 为 Windows 构建生成 pkg-config 元数据 |
| `openssl.pc.in` | OpenSSL pkg-config 文件模板 |
| `portfile.cmake` | 获取固定源码、固定 Linux 运行时目录、清理 buildinfo 原路径并分派平台构建流程 |
| `script-prefix.patch` | 修正 OpenSSL 脚本安装前缀 |
| `usage` | vcpkg 安装后的 OpenSSL 使用说明 |
| `vcpkg-cmake-wrapper.cmake.in` | OpenSSL CMake 查找包装器模板 |
| `vcpkg.json` | OpenSSL 3.6.0 Release overlay 元数据与特性声明 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
