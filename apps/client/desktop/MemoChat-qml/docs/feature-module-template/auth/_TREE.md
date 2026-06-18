# auth/ 目录树

> 登录/注册/重置流程的 feature-first 模块范例，按 MVVM 分层（view/viewmodel/model/service）并独立管理资源与构建清单。

## 子目录

| 子目录 | 作用概括 |
| --- | --- |
| [`model/`](model/_TREE.md) | 认证状态等数据模型 |
| [`resources/`](resources/_TREE.md) | 模块资源（QML/图片）的 qrc 注册 |
| [`service/`](service/_TREE.md) | 认证业务调用（网络请求封装） |
| [`view/`](view/_TREE.md) | 认证相关 QML 页面 |
| [`viewmodel/`](viewmodel/_TREE.md) | 连接 view 与 service 的视图模型 |

## 文件

| 文件 | 作用概括 |
| --- | --- |
| `README.md` | auth 模块模板布局说明 |
| `sources.cmake` | 模块源文件/头文件/资源清单（模板，不参与当前构建） |

<!-- TREE-DOC: 自动维护。文件夹内容变更时同步更新本表与上面的一句话概括。 -->
