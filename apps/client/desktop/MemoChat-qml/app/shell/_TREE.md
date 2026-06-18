# shell/ 目录树

> 应用外壳层，维护页面/标签页与当前用户等外壳级状态，并通过视图模型暴露给 QML 界面。

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `AppShellStateController.cpp` | 外壳状态控制器实现（页面、标签、当前用户、加载/引导状态） |
| `AppShellStateController.h` | 外壳状态控制器接口声明 |
| `ShellViewModel.cpp` | 外壳视图模型实现 |
| `ShellViewModel.h` | 外壳视图模型声明，向 QML 暴露页面与当前用户属性 |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
