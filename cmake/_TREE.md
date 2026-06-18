# cmake/ 目录树

> 项目共用的 CMake 模块，提供 Live2D Cubism 查找、统一格式化目标与第三方包查找重定向。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`package-redirects/`](package-redirects/_TREE.md) | 第三方包 config 查找重定向 |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `FindLive2DCubism.cmake` | 查找 Live2D Cubism SDK 的 CMake find 模块 |
| `MemoChatFormatting.cmake` | 定义代码格式化（clang-format 等）的 CMake 目标 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
