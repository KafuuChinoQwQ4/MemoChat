# cmake/ 目录树

> 项目共用的 CMake 模块，提供 Live2D Cubism 查找、统一格式化目标与第三方包查找重定向。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`package-redirects/`](package-redirects/_TREE.md) | 第三方包 config 查找重定向 |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `EmbedLuaScripts.cmake` | 将 Lua 脚本嵌入为生成的 C++ `std::string_view` 头文件 |
| `FindLive2DCubism.cmake` | 查找 Live2D Cubism SDK 的 CMake find 模块 |
| `MemoChatFormatting.cmake` | 定义代码格式化（clang-format 等）的 CMake 目标 |
| `MemoChatModules.cmake` | 定义 GNU C++ modules 的显式 CMI 构建与消费 helper |
| `UpdateModuleStamp.cmake` | 仅在 CMI 内容哈希变化时更新 module stamp，避免 consumer 因 `.gcm` 时间戳重编 |
| `gcc-modules-depfile-filter.sh` | 清理 GCC modules depfile 中会污染 Ninja deps log 的 CMI 与 phantom module 依赖 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
