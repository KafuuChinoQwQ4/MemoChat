# windows/ 目录树

> libpq baseline port 的 Windows 编译器、工具和依赖适配补丁。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `macro-def.patch` | 修正 Windows 宏定义冲突 |
| `msbuild.patch` | 适配 vcpkg 的 MSBuild 参数与输出 |
| `spin_delay.patch` | 修正 Windows 自旋等待实现 |
| `tcl-9.0-alpha.patch` | 适配 Tcl 9 的构建变化 |
| `win_bison_flex.patch` | 适配 Windows 的 Bison/Flex 工具路径 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
