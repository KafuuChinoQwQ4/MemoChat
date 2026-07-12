# tests/ 目录树

> 全仓库测试根：按语言拆分为 C++ GTest（cpp/）与 Python 契约/结构测试（python/），共享数据放 fixtures/，整体镜像主项目相对路径。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`cpp/`](cpp/_TREE.md) | C++ GTest 测试树，镜像主项目源码路径 |
| [`fixtures/`](fixtures/_TREE.md) | 跨测试共享的静态数据（如 Live2D 最小模型） |
| [`python/`](python/_TREE.md) | Python 结构/契约测试树，镜像主项目路径 |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `CMakeLists.txt` | 注册 C++ 测试子目录及无异常 Python 契约门禁的 CMake 入口 |
| `__init__.py` | 将 tests 标记为 Python 包，支持包内导入 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
